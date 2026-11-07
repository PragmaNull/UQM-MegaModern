/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "libs/graphics/dcqueue.h"

#include "core/log/log.h"
#include "core/stl/chrono.h"
#include "core/stl/condition_variable.h"
#include "core/stl/optional.h"
#include "core/stl/thread.h"
#include "core/string/StringUtils.h"
#include "core/thread/Mutex.h"
#include "libs/graphics/bbox.h"
#include "libs/graphics/context.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/font.h"
#include "libs/graphics/gfx_common.h"
#include "libs/time/timelib.h"
#include "options.h"
#include "port.h"
#include "uqm/setup.h"

namespace
{

// The DCQ lock. A thread can lock it more than once.
uqm::RecursiveMutex s_dcqMutex {};
// The number of times that the owner has locked s_dcqMutex.
int s_dcqDepth UQM_GUARDED_BY(s_dcqMutex) {};
// The renderer signals this when the queue is empty.
uqstl::condition_variable_any s_renderingDone {};

void lockDcq() UQM_ACQUIRE(s_dcqMutex)
{
	s_dcqMutex.lock();
	++s_dcqDepth;
}

void unlockDcq() UQM_RELEASE(s_dcqMutex)
{
	--s_dcqDepth;
	s_dcqMutex.unlock();
}

// Releases all the levels of the DCQ lock while a thread waits, and then restores them.
class DcqFullRelease
{
public:
	void unlock() UQM_NO_THREAD_SAFETY_ANALYSIS
	{
		m_depth = s_dcqDepth;
		s_dcqDepth = 0;
		for (int i {}; i < m_depth; ++i)
		{
			s_dcqMutex.unlock();
		}
	}

	void lock() UQM_NO_THREAD_SAFETY_ANALYSIS
	{
		for (int i {}; i < m_depth; ++i)
		{
			s_dcqMutex.lock();
		}
		s_dcqDepth = m_depth;
	}

private:
	int m_depth {};
};

} // namespace

static uqstl::array<tfb::DrawCommand, DcqMax> DCQ UQM_GUARDED_BY(s_dcqMutex);

static tfb::DrawCommandQueue DrawCommandQueue UQM_GUARDED_BY(s_dcqMutex);

static inline constexpr uint32_t DCQueueFPS {20};
static inline constexpr auto DCQueueTicksForFps {getTicksForFramerate(DCQueueFPS)};
int RenderedFrames = 0;


static bool DCQ_HasSpace(int slots) UQM_REQUIRES(s_dcqMutex)
{
	return DrawCommandQueue.FullSize < DcqMax - slots;
}

// Wait for the queue to be emptied. Always have the DCQ locked when calling this.
static void TFB_WaitForSpace(int requested_slots) UQM_REQUIRES(s_dcqMutex)
{
	uqm::log::debug("DCQ overload (Size = {}, FullSize = {}, "
					"Requested = {}).  Sleeping until renderer is done.",
					DrawCommandQueue.Size, DrawCommandQueue.FullSize,
					requested_slots);
	TFB_BatchReset();
	DcqFullRelease release {};
	while (!DCQ_HasSpace(requested_slots))
	{
		s_renderingDone.wait(release);
	}
	uqm::log::debug("DCQ clear (Size = {}, FullSize = {}).  Continuing.",
					DrawCommandQueue.Size, DrawCommandQueue.FullSize);
}

// Locks the DCQ, and waits until it has space for the slots.
static void lockDcqWithSpace(int slots) UQM_ACQUIRE(s_dcqMutex)
{
	lockDcq();
	if (!DCQ_HasSpace(slots))
	{
		TFB_WaitForSpace(slots);
	}
}

// The number of commands that the renderer can take now.
static int DCQ_Size(void)
{
	const uqm::LockGuard lock {s_dcqMutex};
	return DrawCommandQueue.Size;
}

// The number of commands in the queue, with the batched ones.
static int DCQ_FullSize(void)
{
	const uqm::LockGuard lock {s_dcqMutex};
	return DrawCommandQueue.FullSize;
}

namespace
{

// Holds the DCQ lock from construction to destruction.
class [[nodiscard]] UQM_SCOPED_CAPABILITY DcqGuard
{
public:
	DcqGuard() UQM_ACQUIRE(s_dcqMutex)
	{
		lockDcq();
	}

	// Waits until the queue has space for 'slots' commands.
	explicit DcqGuard(int slots) UQM_ACQUIRE(s_dcqMutex)
	{
		lockDcqWithSpace(slots);
	}

	~DcqGuard() UQM_RELEASE()
	{
		unlockDcq();
	}

	DcqGuard(const DcqGuard&) = delete;
	DcqGuard& operator=(const DcqGuard&) = delete;
};

} // namespace

DcqLock::DcqLock(int slots)
	UQM_NO_THREAD_SAFETY_ANALYSIS
{
	lockDcqWithSpace(slots);
}

DcqLock::~DcqLock()
	UQM_NO_THREAD_SAFETY_ANALYSIS
{
	unlockDcq();
}

// Always have the DCQ locked when calling this.
static void Synchronize_DCQ(void) UQM_REQUIRES(s_dcqMutex)
{
	if (!DrawCommandQueue.Batching)
	{
		const int front = DrawCommandQueue.Front;
		const int back = DrawCommandQueue.InsertionPoint;
		DrawCommandQueue.Back = DrawCommandQueue.InsertionPoint;
		if (front <= back)
		{
			DrawCommandQueue.Size = (back - front);
		}
		else
		{
			DrawCommandQueue.Size = (back + DcqMax - front);
		}
		DrawCommandQueue.FullSize = DrawCommandQueue.Size;
	}
}

void TFB_BatchGraphics(void)
{
	const DcqGuard lock {};
	DrawCommandQueue.Batching++;
}

void TFB_UnbatchGraphics(void)
{
	const DcqGuard lock {};
	if (DrawCommandQueue.Batching)
	{
		DrawCommandQueue.Batching--;
	}
	Synchronize_DCQ();
}

// Cancel all pending batch operations, making them unbatched.  This will
// cause a small amount of flicker when invoked, but prevents
// batching problems from freezing the game.
void TFB_BatchReset(void)
{
	const DcqGuard lock {};
	DrawCommandQueue.Batching = 0;
	Synchronize_DCQ();
}


// Draw Command Queue Stuff

void Init_DrawCommandQueue(void)
{
	{
		const DcqGuard lock {};
		DrawCommandQueue.Back = 0;
		DrawCommandQueue.Front = 0;
		DrawCommandQueue.InsertionPoint = 0;
		DrawCommandQueue.Batching = 0;
		DrawCommandQueue.FullSize = 0;
		DrawCommandQueue.Size = 0;
	}

	TFB_BBox_Init(CanvasWidth, CanvasHeight);
}

void Uninit_DrawCommandQueue(void)
{
}

void TFB_DrawCommandQueue_Push(tfb::DrawCommand* Command)
{
	const DcqGuard lock {1};
	DCQ[DrawCommandQueue.InsertionPoint] = *Command;
	DrawCommandQueue.InsertionPoint = (DrawCommandQueue.InsertionPoint + 1)
									% DcqMax;
	DrawCommandQueue.FullSize++;
	Synchronize_DCQ();
}

int TFB_DrawCommandQueue_Pop(tfb::DrawCommand* target)
{
	const DcqGuard lock {};

	if (DrawCommandQueue.Size == 0)
	{
		return 0;
	}

	if (DrawCommandQueue.Front == DrawCommandQueue.Back && DrawCommandQueue.Size != DcqMax)
	{
		uqm::log::debug("Augh!  Assertion failure in DCQ!  "
						"Front == Back, Size != DCQ_MAX");
		DrawCommandQueue.Size = 0;
		return 0;
	}

	*target = DCQ[DrawCommandQueue.Front];
	DrawCommandQueue.Front = (DrawCommandQueue.Front + 1) % DcqMax;

	DrawCommandQueue.Size--;
	DrawCommandQueue.FullSize--;
	return 1;
}

void TFB_DrawCommandQueue_Clear()
{
	const DcqGuard lock {};
	DrawCommandQueue.Size = 0;
	DrawCommandQueue.Front = 0;
	DrawCommandQueue.Back = 0;
	DrawCommandQueue.Batching = 0;
	DrawCommandQueue.FullSize = 0;
	DrawCommandQueue.InsertionPoint = 0;
}

static void checkExclusiveThread(tfb::DrawCommand* DrawCommand)
{
#ifdef DEBUG_DCQ_THREADS
	static uqstl::thread::id exclusiveThreadId {};

	// Only one thread is currently allowed to enqueue commands
	// This is not a technical limitation but rather a semantical one atm.
	if (DrawCommand->Type == DrawCommandType::ReinitVideo)
	{ // TFB_DRAWCOMMANDTYPE_REINITVIDEO is an exception
		// It is queued from the main() thread, which is safe to do
		return;
	}

	if (exclusiveThreadId == uqstl::thread::id {})
	{
		exclusiveThreadId = uqstl::this_thread::get_id();
	}
	else
	{
		assert(uqstl::this_thread::get_id() == exclusiveThreadId);
	}
#else
	(void)DrawCommand; // suppress unused warning
#endif
}

void TFB_EnqueueDrawCommand(tfb::DrawCommand* DrawCommand)
{
	if (TFB_DEBUG_HALT)
	{
		return;
	}

	checkExclusiveThread(DrawCommand);

	if (DrawCommand->Type <= DrawCommandType::CopyToImage
		&& _CurFramePtr->Type == ScreenDrawable)
	{
		static GfxRect scissor_rect;

		// Set the clipping region.
		// We allow drawing with no current context set, so the whole screen
		if ((_pCurContext && !rectsEqual(scissor_rect, _pCurContext->ClipRect))
			|| (!_pCurContext && scissor_rect.extent.width != 0))
		{
			// Enqueue command to set the glScissor spec
			tfb::DrawCommand DC;

			if (_pCurContext)
			{
				scissor_rect = _pCurContext->ClipRect;
			}
			else
			{
				scissor_rect.extent.width = 0;
			}

			if (scissor_rect.extent.width)
			{
				DC.Type = DrawCommandType::ScissorEnable;
				DC.data.scissor.rect = scissor_rect;
			}
			else
			{
				DC.Type = DrawCommandType::ScissorDisable;
			}

			TFB_EnqueueDrawCommand(&DC);
		}
	}

	TFB_DrawCommandQueue_Push(DrawCommand);
}

// Time spent on rendering, and the frames rendered in that time, since the last FPS update.
static uqstl::chrono::steady_clock::duration s_renderTime {};
static int s_renderFrames {};

// Adds the time since start to the render time, for one rendered frame.
static void addRenderTime(uqstl::chrono::steady_clock::time_point start)
{
	s_renderTime += uqstl::chrono::steady_clock::now() - start;
	++s_renderFrames;
}

// fps is the rate of the frames that the game draws. renderFps is the rate
// that the rendering could reach if it did not wait for the game.
static void computeFPS(int* fps, int* renderFps)
{
	static TimeCount last_time;
	static TimePeriod fps_counter;
	TimeCount current_time;
	TimePeriod delta_time;

	current_time = GetTimeCounter();
	delta_time = current_time - last_time;
	last_time = current_time;

	fps_counter += delta_time;
	if (fps_counter > DCQueueTicksForFps)
	{
		*fps = static_cast<int>(RenderedFrames / uqstl::chrono::duration<double>(fps_counter).count());
		const double renderSeconds {uqstl::chrono::duration<double>(s_renderTime).count()};
		*renderFps = renderSeconds > 0.0 ? static_cast<int>(s_renderFrames / renderSeconds) : 0;
		uqm::log::info("fps {:.2}, effective {}, render {}",
					   1.0 / uqstl::chrono::duration<double>(delta_time).count(), *fps, *renderFps);

		fps_counter = TimePeriod {};
		RenderedFrames = 0;
		s_renderTime = {};
		s_renderFrames = 0;
	}
}

// Draws text on the FPS canvas. The text starts at x.
static void drawFPSText(uqstl::string_view text, int x, int y, Color color)
{
	GfxRect tr;
	int16_t w {};
	int16_t h {};

	GetFontDims(&w, &h);
	tfb::Image* img = TFB_DrawImage_CreateForScreen(w, h, true);
	tr.corner.x = tr.corner.y = 0;
	tr.extent.width = w;
	tr.extent.height = h;
	TFB_DrawImage_Rect(&tr, color, DrawReplaceMode, img);

	for (const char c : text)
	{
		tfb::Char* ch = GetFrameForFPS(static_cast<unsigned char>(c));
		if (ch && ch->disp.width)
		{
			TFB_DrawCanvas_FontChar(ch, img, x, y, MAKE_DRAW_MODE(DrawKind::Alpha, 0xff), TFB_GetFPSCanvas());
			x += ch->disp.width + GetFontCharSpace();
		}
		else
		{
			x += w / 2;
		}
	}
	TFB_DrawImage_Delete(img);
}

static void RenderFPS(int fps, int renderFps)
{
	static int prevFPS = 0;
	static int prevRenderFPS = 0;
	if (GoodToGoFPS() && (prevFPS != fps || prevRenderFPS != renderFps))
	{
		prevFPS = fps;
		prevRenderFPS = renderFps;

		TFB_ClearFPSCanvas();

		const int x = 2 << resolutionFactor;
		drawFPSText(fmt::format("CPU: {}", fps), x, 7 << resolutionFactor, BuildColorRgba(0x00, 0xFF, 0x00, 0xFF));
		drawFPSText(fmt::format("GPU: {}", renderFps), x, 16 << resolutionFactor, BuildColorRgba(0x00, 0xFF, 0xFF, 0xFF));
	}
}

// Only call from main() thread!!
void TFB_FlushGraphics(void)
{
	static int fps = 0;
	static int renderFps = 0;
	const bool showFps {testFlag(g_gfxFlags, uqm::GfxFlags::ShowFPS)};
	const uqstl::chrono::steady_clock::time_point start {uqstl::chrono::steady_clock::now()};
	const auto notifyDone {uqgsl::finally([] {
		s_renderingDone.notify_all();
	})};

	if (DCQ_Size() == 0)
	{
		static int last_fade = 255;
		static int last_transition = 255;
		const int current_fade = GetFadeAmount();
		const int current_transition = TransitionAmount;

		if ((current_fade != 255 && current_fade != last_fade) || (current_transition != 255 && current_transition != last_transition) || (current_fade == 255 && last_fade != 255) || (current_transition == 255 && last_transition != 255))
		{
			TFB_SwapBuffers(tfb::Redraw::Fading);
			// if fading, redraw every frame
			if (showFps)
			{
				addRenderTime(start);
			}
		}
		else if (TFB_ImGuiVisible())
		{
			// The overlay draws at most once for each frame of a 60 Hz display.
			static uqstl::chrono::steady_clock::time_point s_lastOverlayFrame {};
			if (start - s_lastOverlayFrame >= uqstl::chrono::microseconds {16'667})
			{
				s_lastOverlayFrame = start;
				TFB_SwapBuffers(tfb::Redraw::Expose);
			}
			TaskSwitch();
		}
		else
		{
			TaskSwitch();
		}

		last_fade = current_fade;
		last_transition = current_transition;
		return;
	}

	if (showFps)
	{
		computeFPS(&fps, &renderFps);
	}

	int commands_handled {0};
	// Holds the queue lock while the renderer empties the queue.
	uqstl::optional<DcqLock> livelockLock {};

	if (DCQ_FullSize() > DcqForceBreakSize)
	{
		TFB_BatchReset();
	}

	if (DCQ_Size() > DcqForceSlowdownSize)
	{
		livelockLock.emplace(-1);
	}

	TFB_BBox_Reset();

	for (;;)
	{
		tfb::DrawCommand DC;

		if (!TFB_DrawCommandQueue_Pop(&DC))
		{
			// the Queue is now empty.
			break;
		}

		++commands_handled;
		if (!livelockLock && commands_handled + DCQ_Size() > DcqLivelockMax)
		{
			livelockLock.emplace(-1);
		}

		switch (DC.Type)
		{
			case DrawCommandType::SetMipmap:
				{
					const tfb::DrawCommandSetMipmap* cmd = &DC.data.setmipmap;
					TFB_DrawImage_SetMipmap(cmd->image, cmd->mipmap,
											cmd->hotx, cmd->hoty);
					break;
				}

			case DrawCommandType::Image:
				{
					const tfb::DrawCommandImage* cmd = &DC.data.image;
					tfb::Image* DC_image = cmd->image;
					const int x = cmd->x;
					const int y = cmd->y;

					TFB_DrawCanvas_Image(DC_image, x, y,
										 cmd->scale, cmd->scaleMode, cmd->colormap,
										 cmd->drawMode,
										 TFB_GetScreenCanvas(cmd->destBuffer));

					if (cmd->destBuffer == ScreenBuffer::Main)
					{
						const uqm::LockGuard imageLock {DC_image->mutex};
						if (cmd->scale)
						{
							TFB_BBox_RegisterCanvas(DC_image->ScaledImg,
													x - DC_image->last_scale_hs.x,
													y - DC_image->last_scale_hs.y);
						}
						else
						{
							TFB_BBox_RegisterCanvas(DC_image->NormalImg,
													x - DC_image->NormalHs.x,
													y - DC_image->NormalHs.y);
						}
					}

					break;
				}

			case DrawCommandType::FilledImage:
				{
					const tfb::DrawCommandFilledImage* cmd = &DC.data.filledimage;
					tfb::Image* DC_image = cmd->image;
					const int x = cmd->x;
					const int y = cmd->y;

					TFB_DrawCanvas_FilledImage(DC_image, x, y,
											   cmd->scale, cmd->scaleMode, cmd->color,
											   cmd->drawMode,
											   TFB_GetScreenCanvas(cmd->destBuffer));

					if (cmd->destBuffer == ScreenBuffer::Main)
					{
						const uqm::LockGuard imageLock {DC_image->mutex};
						if (cmd->scale)
						{
							TFB_BBox_RegisterCanvas(DC_image->ScaledImg,
													x - DC_image->last_scale_hs.x,
													y - DC_image->last_scale_hs.y);
						}
						else
						{
							TFB_BBox_RegisterCanvas(DC_image->NormalImg,
													x - DC_image->NormalHs.x,
													y - DC_image->NormalHs.y);
						}
					}

					break;
				}

			case DrawCommandType::FontChar:
				{
					const tfb::DrawCommandFontChar* cmd = &DC.data.fontchar;
					tfb::Char* DC_char = cmd->fontchar;
					const int x = cmd->x;
					const int y = cmd->y;

					TFB_DrawCanvas_FontChar(DC_char, cmd->backing, x, y,
											cmd->drawMode, TFB_GetScreenCanvas(cmd->destBuffer));

					if (cmd->destBuffer == ScreenBuffer::Main)
					{
						GfxRect r;

						r.corner.x = x - DC_char->hotSpot.x;
						r.corner.y = y - DC_char->hotSpot.y;
						r.extent.width = DC_char->extent.width;
						r.extent.height = DC_char->extent.height;

						TFB_BBox_RegisterRect(&r);
					}

					break;
				}

			case DrawCommandType::Line:
				{
					const tfb::DrawCommandLine* cmd = &DC.data.line;

					if (cmd->destBuffer == ScreenBuffer::Main)
					{
						GfxRect r;
						r.corner.x = uqstl::min(cmd->x1, cmd->x2);
						r.corner.y = uqstl::min(cmd->y1, cmd->y2);
						r.extent.width = abs(cmd->x1 - cmd->x2) + cmd->thickness;
						r.extent.height = abs(cmd->y1 - cmd->y2) + cmd->thickness;

						TFB_BBox_RegisterRect(&r);
					}
					TFB_DrawCanvas_Line(cmd->x1, cmd->y1, cmd->x2, cmd->y2,
										cmd->color, cmd->drawMode,
										TFB_GetScreenCanvas(cmd->destBuffer),
										cmd->thickness);
					break;
				}

			case DrawCommandType::Rectangle:
				{
					tfb::DrawCommandRect* cmd = &DC.data.rect;

					if (cmd->destBuffer == ScreenBuffer::Main)
					{
						TFB_BBox_RegisterRect(&cmd->rect);
					}
					TFB_DrawCanvas_Rect(&cmd->rect, cmd->color, cmd->drawMode,
										TFB_GetScreenCanvas(cmd->destBuffer));

					break;
				}

			case DrawCommandType::ScissorEnable:
				{
					const tfb::DrawCommandScissor* cmd = &DC.data.scissor;

					TFB_DrawCanvas_SetClipRect(
						TFB_GetScreenCanvas(ScreenBuffer::Main), &cmd->rect);
					TFB_BBox_SetClipRect(&DC.data.scissor.rect);
					break;
				}

			case DrawCommandType::ScissorDisable:
				TFB_DrawCanvas_SetClipRect(
					TFB_GetScreenCanvas(ScreenBuffer::Main), nullptr);
				TFB_BBox_SetClipRect(nullptr);
				break;

			case DrawCommandType::CopyToImage:
				{
					const tfb::DrawCommandCopyToImage* cmd = &DC.data.copytoimage;
					tfb::Image* DC_image = cmd->image;
					const GfxPoint dstPt = {0, 0};

					if (DC_image == 0)
					{
						uqm::log::debug("DCQ ERROR: COPYTOIMAGE passed null "
										"image ptr");
						break;
					}
					const uqm::LockGuard imageLock {DC_image->mutex};
					TFB_DrawCanvas_CopyRect(
						TFB_GetScreenCanvas(cmd->srcBuffer), &cmd->rect,
						DC_image->NormalImg, dstPt);
					break;
				}

			case DrawCommandType::Copy:
				{
					const tfb::DrawCommandCopy* cmd = &DC.data.copy;
					const GfxRect r = cmd->rect;

					if (cmd->destBuffer == ScreenBuffer::Main)
					{
						TFB_BBox_RegisterRect(&cmd->rect);
					}

					TFB_DrawCanvas_CopyRect(
						TFB_GetScreenCanvas(cmd->srcBuffer), &r,
						TFB_GetScreenCanvas(cmd->destBuffer), r.corner);
					break;
				}

			case DrawCommandType::DeleteImage:
				{
					tfb::Image* DC_image = DC.data.deleteimage.image;
					TFB_DrawImage_Delete(DC_image);
					break;
				}

			case DrawCommandType::DeleteData:
				{
					delete[] DC.data.deletedata.data;
					break;
				}

			case DrawCommandType::SendSignal:
				DC.data.sendsignal.sem->release();
				break;

			case DrawCommandType::ReinitVideo:
				{
					const tfb::DrawCommandReinitVideo* cmd = &DC.data.reinitvideo;
					const uqm::GfxDriver oldDriver = GraphicsDriver;
					const uqm::GfxFlags oldFlags = g_gfxFlags;
					const int oldWidth = WindowWidth;
					const int oldHeight = WindowHeight;
					unsigned int windowType {static_cast<unsigned int>(optWindowType)};
					if (TFB_ReInitGraphics(cmd->driver, cmd->flags,
										   cmd->width, cmd->height, &resolutionFactor,
										   &windowType))
					{
						uqm::log::error("Could not provide requested mode: "
										"reverting to last known driver.");
						// We don't know what exactly failed, so roll it all back
						if (TFB_ReInitGraphics(oldDriver, oldFlags,
											   oldWidth, oldHeight, &resolutionFactor,
											   &windowType))
						{
							uqm::log::critical(
								"Couldn't reinit at that point either. "
								"Your video has been somehow tied in knots.");
							exit(EXIT_FAILURE);
						}
					}
					TFB_SwapBuffers(tfb::Redraw::Yes);
					break;
				}

			case DrawCommandType::Callback:
				{
					DC.data.callback.callback(DC.data.callback.arg);
					break;
				}
		}
	}

	livelockLock.reset();

	if (showFps)
	{
		RenderFPS(fps, renderFps);
	}

	TFB_SwapBuffers(tfb::Redraw::No);
	RenderedFrames++;
	if (showFps)
	{
		addRenderTime(start);
	}
}

void TFB_PurgeDanglingGraphics(void)
{
	const DcqLock lock {-1};

	for (;;)
	{
		tfb::DrawCommand DC;

		if (!TFB_DrawCommandQueue_Pop(&DC))
		{
			// the Queue is now empty.
			break;
		}

		switch (DC.Type)
		{
			case DrawCommandType::DeleteImage:
				{
					tfb::Image* DC_image = DC.data.deleteimage.image;
					TFB_DrawImage_Delete(DC_image);
					break;
				}
			case DrawCommandType::DeleteData:
				{
					delete[] DC.data.deletedata.data;
					break;
				}
			case DrawCommandType::Image:
				{
					tfb::ColorMap* cmap = DC.data.image.colormap;
					if (cmap)
					{
						TFB_ReturnColorMap(cmap);
					}
					break;
				}
			case DrawCommandType::SendSignal:
				{
					DC.data.sendsignal.sem->release();
					break;
				}
			default:
				break;
		}
	}
}
