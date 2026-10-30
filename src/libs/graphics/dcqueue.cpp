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

#include "port.h"
#include "core/string/StringUtils.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/context.h"
#include "libs/graphics/dcqueue.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/bbox.h"
#include "libs/time/timelib.h"
#include "core/log/log.h"
#include "options.h"
#include "libs/graphics/font.h"
#include "uqm/setup.h"

#include "core/thread/Mutex.h"

#include <condition_variable>
#include <thread>

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

static TFB_DrawCommand DCQ[DCQ_MAX] UQM_GUARDED_BY(s_dcqMutex);

static TFB_DrawCommandQueue DrawCommandQueue UQM_GUARDED_BY(s_dcqMutex);

static inline constexpr uint32_t DCQueueFPS {20};
static inline constexpr auto DCQueueTicksForFps {getTicksForFramerate(DCQueueFPS)};
int RenderedFrames = 0;


static bool
DCQ_HasSpace(int slots) UQM_REQUIRES(s_dcqMutex)
{
	return DrawCommandQueue.FullSize < DCQ_MAX - slots;
}

// Wait for the queue to be emptied. Always have the DCQ locked when calling this.
static void
TFB_WaitForSpace(int requested_slots) UQM_REQUIRES(s_dcqMutex)
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
static void
lockDcqWithSpace(int slots) UQM_ACQUIRE(s_dcqMutex)
{
	lockDcq();
	if (!DCQ_HasSpace(slots))
	{
		TFB_WaitForSpace(slots);
	}
}

// The number of commands that the renderer can take now.
static int
DCQ_Size(void)
{
	const uqm::LockGuard lock {s_dcqMutex};
	return DrawCommandQueue.Size;
}

// The number of commands in the queue, with the batched ones.
static int
DCQ_FullSize(void)
{
	const uqm::LockGuard lock {s_dcqMutex};
	return DrawCommandQueue.FullSize;
}

// Holds the DCQ lock until Unlock_DCQ, so the analysis cannot check it.
void Lock_DCQ(int slots) UQM_NO_THREAD_SAFETY_ANALYSIS
{
	lockDcqWithSpace(slots);
}

void Unlock_DCQ(void) UQM_NO_THREAD_SAFETY_ANALYSIS
{
	unlockDcq();
}

// Always have the DCQ locked when calling this.
static void
Synchronize_DCQ(void) UQM_REQUIRES(s_dcqMutex)
{
	if (!DrawCommandQueue.Batching)
	{
		int front = DrawCommandQueue.Front;
		int back = DrawCommandQueue.InsertionPoint;
		DrawCommandQueue.Back = DrawCommandQueue.InsertionPoint;
		if (front <= back)
		{
			DrawCommandQueue.Size = (back - front);
		}
		else
		{
			DrawCommandQueue.Size = (back + DCQ_MAX - front);
		}
		DrawCommandQueue.FullSize = DrawCommandQueue.Size;
	}
}

void TFB_BatchGraphics(void)
{
	lockDcq();
	DrawCommandQueue.Batching++;
	unlockDcq();
}

void TFB_UnbatchGraphics(void)
{
	lockDcq();
	if (DrawCommandQueue.Batching)
	{
		DrawCommandQueue.Batching--;
	}
	Synchronize_DCQ();
	unlockDcq();
}

// Cancel all pending batch operations, making them unbatched.  This will
// cause a small amount of flicker when invoked, but prevents
// batching problems from freezing the game.
void TFB_BatchReset(void)
{
	lockDcq();
	DrawCommandQueue.Batching = 0;
	Synchronize_DCQ();
	unlockDcq();
}


// Draw Command Queue Stuff

void Init_DrawCommandQueue(void)
{
	lockDcq();
	DrawCommandQueue.Back = 0;
	DrawCommandQueue.Front = 0;
	DrawCommandQueue.InsertionPoint = 0;
	DrawCommandQueue.Batching = 0;
	DrawCommandQueue.FullSize = 0;
	DrawCommandQueue.Size = 0;
	unlockDcq();

	TFB_BBox_Init(CanvasWidth, CanvasHeight);
}

void Uninit_DrawCommandQueue(void)
{
}

void TFB_DrawCommandQueue_Push(TFB_DrawCommand* Command)
{
	lockDcqWithSpace(1);
	DCQ[DrawCommandQueue.InsertionPoint] = *Command;
	DrawCommandQueue.InsertionPoint = (DrawCommandQueue.InsertionPoint + 1)
									% DCQ_MAX;
	DrawCommandQueue.FullSize++;
	Synchronize_DCQ();
	unlockDcq();
}

int TFB_DrawCommandQueue_Pop(TFB_DrawCommand* target)
{
	lockDcq();

	if (DrawCommandQueue.Size == 0)
	{
		unlockDcq();
		return (0);
	}

	if (DrawCommandQueue.Front == DrawCommandQueue.Back && DrawCommandQueue.Size != DCQ_MAX)
	{
		uqm::log::debug("Augh!  Assertion failure in DCQ!  "
						"Front == Back, Size != DCQ_MAX");
		DrawCommandQueue.Size = 0;
		unlockDcq();
		return (0);
	}

	*target = DCQ[DrawCommandQueue.Front];
	DrawCommandQueue.Front = (DrawCommandQueue.Front + 1) % DCQ_MAX;

	DrawCommandQueue.Size--;
	DrawCommandQueue.FullSize--;
	unlockDcq();

	return 1;
}

void TFB_DrawCommandQueue_Clear()
{
	lockDcq();
	DrawCommandQueue.Size = 0;
	DrawCommandQueue.Front = 0;
	DrawCommandQueue.Back = 0;
	DrawCommandQueue.Batching = 0;
	DrawCommandQueue.FullSize = 0;
	DrawCommandQueue.InsertionPoint = 0;
	unlockDcq();
}

static void
checkExclusiveThread(TFB_DrawCommand* DrawCommand)
{
#ifdef DEBUG_DCQ_THREADS
	static uqstl::thread::id exclusiveThreadId {};

	// Only one thread is currently allowed to enqueue commands
	// This is not a technical limitation but rather a semantical one atm.
	if (DrawCommand->Type == TFB_DRAWCOMMANDTYPE_REINITVIDEO)
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

void TFB_EnqueueDrawCommand(TFB_DrawCommand* DrawCommand)
{
	if (TFB_DEBUG_HALT)
	{
		return;
	}

	checkExclusiveThread(DrawCommand);

	if (DrawCommand->Type <= TFB_DRAWCOMMANDTYPE_COPYTOIMAGE
		&& _CurFramePtr->Type == SCREEN_DRAWABLE)
	{
		static GFXRECT scissor_rect;

		// Set the clipping region.
		// We allow drawing with no current context set, so the whole screen
		if ((_pCurContext && !rectsEqual(scissor_rect, _pCurContext->ClipRect))
			|| (!_pCurContext && scissor_rect.extent.width != 0))
		{
			// Enqueue command to set the glScissor spec
			TFB_DrawCommand DC;

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
				DC.Type = TFB_DRAWCOMMANDTYPE_SCISSORENABLE;
				DC.data.scissor.rect = scissor_rect;
			}
			else
			{
				DC.Type = TFB_DRAWCOMMANDTYPE_SCISSORDISABLE;
			}

			TFB_EnqueueDrawCommand(&DC);
		}
	}

	TFB_DrawCommandQueue_Push(DrawCommand);
}

static void
computeFPS(int* fps)
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
		*fps = (int)((float)DCQueueTicksForFps * RenderedFrames / fps_counter);
		uqm::log::info("fps {:.2}, effective {}",
					   (float)DCQueueTicksForFps / delta_time, *fps);

		fps_counter = 0;
		RenderedFrames = 0;
	}
}

static void
RenderFPS(int* fps)
{
	static int prevFPS = 0;
	if (GoodToGoFPS() && (prevFPS != *fps))
	{
		GFXRECT tr;
		int16_t w, h;
		int i;
		int max;
		int step = 6;
		TFB_Char* ch;
		TFB_Image* img;

		prevFPS = *fps;

		TFB_ClearFPSCanvas();

		int x = 14 << resolutionFactor;
		int y = 7 << resolutionFactor;

		char buf[8] {};
		const auto fmtResult = fmt::format_to_sz_n(buf, "{}", *fps);

		max = fmtResult.size; //(uint16_t) utf8StringCount(buf);

		GetFontDims(&w, &h);
		img = TFB_DrawImage_CreateForScreen(w, h, true);
		tr.corner.x = tr.corner.y = 0;
		tr.extent.width = w;
		tr.extent.height = h;
		TFB_DrawImage_Rect(&tr, BUILD_COLOR_RGBA(0x00, 0xFF, 0x00, 0xFF), DRAW_REPLACE_MODE, img);

		for (i = max - 1; i >= 0; i--)
		{
			ch = GetFrameForFPS(buf[i]);
			if (ch)
			{
				TFB_DrawCanvas_FontChar(ch, img, x, y, MAKE_DRAW_MODE(DRAW_ALPHA, 0xff), TFB_GetFPSCanvas());
				if ((i > 0) && (buf[i - 1] == '1'))
				{
					step = 5;
				}
				else
				{
					step = 6;
				}
				x -= step << resolutionFactor;
			}
		}
		TFB_DrawImage_Delete(img);
	}
}

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

// Only call from main() thread!!
void TFB_FlushGraphics(void)
{
	int commands_handled;
	static int fps = 0;
	bool livelock_deterrence;

	if (DCQ_Size() == 0)
	{
		static int last_fade = 255;
		static int last_transition = 255;
		int current_fade = GetFadeAmount();
		int current_transition = TransitionAmount;

		if ((current_fade != 255 && current_fade != last_fade) || (current_transition != 255 && current_transition != last_transition) || (current_fade == 255 && last_fade != 255) || (current_transition == 255 && last_transition != 255))
		{
			TFB_SwapBuffers(uqm::TFBRedraw::Fading);
			// if fading, redraw every frame
		}
		else
		{
			TaskSwitch();
		}

		last_fade = current_fade;
		last_transition = current_transition;
		s_renderingDone.notify_all();
		return;
	}

	if (testFlag(g_gfxFlags, uqm::GfxFlags::ShowFPS))
	{
		computeFPS(&fps);
	}

	commands_handled = 0;
	livelock_deterrence = false;

	if (DCQ_FullSize() > DCQ_FORCE_BREAK_SIZE)
	{
		TFB_BatchReset();
	}

	if (DCQ_Size() > DCQ_FORCE_SLOWDOWN_SIZE)
	{
		Lock_DCQ(-1);
		livelock_deterrence = true;
	}

	TFB_BBox_Reset();

	for (;;)
	{
		TFB_DrawCommand DC;

		if (!TFB_DrawCommandQueue_Pop(&DC))
		{
			// the Queue is now empty.
			break;
		}

		++commands_handled;
		if (!livelock_deterrence && commands_handled + DCQ_Size() > DCQ_LIVELOCK_MAX)
		{
			// log_add (log_Debug, "Initiating livelock deterrence!");
			livelock_deterrence = true;

			Lock_DCQ(-1);
		}

		switch (DC.Type)
		{
			case TFB_DRAWCOMMANDTYPE_SETMIPMAP:
				{
					TFB_DrawCommand_SetMipmap* cmd = &DC.data.setmipmap;
					TFB_DrawImage_SetMipmap(cmd->image, cmd->mipmap,
											cmd->hotx, cmd->hoty);
					break;
				}

			case TFB_DRAWCOMMANDTYPE_IMAGE:
				{
					TFB_DrawCommand_Image* cmd = &DC.data.image;
					TFB_Image* DC_image = cmd->image;
					const int x = cmd->x;
					const int y = cmd->y;

					TFB_DrawCanvas_Image(DC_image, x, y,
										 cmd->scale, cmd->scaleMode, cmd->colormap,
										 cmd->drawMode,
										 TFB_GetScreenCanvas(cmd->destBuffer));

					if (cmd->destBuffer == TFB_SCREEN_MAIN)
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

			case TFB_DRAWCOMMANDTYPE_FILLEDIMAGE:
				{
					TFB_DrawCommand_FilledImage* cmd = &DC.data.filledimage;
					TFB_Image* DC_image = cmd->image;
					const int x = cmd->x;
					const int y = cmd->y;

					TFB_DrawCanvas_FilledImage(DC_image, x, y,
											   cmd->scale, cmd->scaleMode, cmd->color,
											   cmd->drawMode,
											   TFB_GetScreenCanvas(cmd->destBuffer));

					if (cmd->destBuffer == TFB_SCREEN_MAIN)
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

			case TFB_DRAWCOMMANDTYPE_FONTCHAR:
				{
					TFB_DrawCommand_FontChar* cmd = &DC.data.fontchar;
					TFB_Char* DC_char = cmd->fontchar;
					const int x = cmd->x;
					const int y = cmd->y;

					TFB_DrawCanvas_FontChar(DC_char, cmd->backing, x, y,
											cmd->drawMode, TFB_GetScreenCanvas(cmd->destBuffer));

					if (cmd->destBuffer == TFB_SCREEN_MAIN)
					{
						GFXRECT r;

						r.corner.x = x - DC_char->HotSpot.x;
						r.corner.y = y - DC_char->HotSpot.y;
						r.extent.width = DC_char->extent.width;
						r.extent.height = DC_char->extent.height;

						TFB_BBox_RegisterRect(&r);
					}

					break;
				}

			case TFB_DRAWCOMMANDTYPE_LINE:
				{
					TFB_DrawCommand_Line* cmd = &DC.data.line;

					if (cmd->destBuffer == TFB_SCREEN_MAIN)
					{
						GFXRECT r;
						r.corner.x = MIN(cmd->x1, cmd->x2);
						r.corner.y = MIN(cmd->y1, cmd->y2);
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

			case TFB_DRAWCOMMANDTYPE_RECTANGLE:
				{
					TFB_DrawCommand_Rect* cmd = &DC.data.rect;

					if (cmd->destBuffer == TFB_SCREEN_MAIN)
					{
						TFB_BBox_RegisterRect(&cmd->rect);
					}
					TFB_DrawCanvas_Rect(&cmd->rect, cmd->color, cmd->drawMode,
										TFB_GetScreenCanvas(cmd->destBuffer));

					break;
				}

			case TFB_DRAWCOMMANDTYPE_SCISSORENABLE:
				{
					TFB_DrawCommand_Scissor* cmd = &DC.data.scissor;

					TFB_DrawCanvas_SetClipRect(
						TFB_GetScreenCanvas(TFB_SCREEN_MAIN), &cmd->rect);
					TFB_BBox_SetClipRect(&DC.data.scissor.rect);
					break;
				}

			case TFB_DRAWCOMMANDTYPE_SCISSORDISABLE:
				TFB_DrawCanvas_SetClipRect(
					TFB_GetScreenCanvas(TFB_SCREEN_MAIN), nullptr);
				TFB_BBox_SetClipRect(nullptr);
				break;

			case TFB_DRAWCOMMANDTYPE_COPYTOIMAGE:
				{
					TFB_DrawCommand_CopyToImage* cmd = &DC.data.copytoimage;
					TFB_Image* DC_image = cmd->image;
					const GFXPOINT dstPt = {0, 0};

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

			case TFB_DRAWCOMMANDTYPE_COPY:
				{
					TFB_DrawCommand_Copy* cmd = &DC.data.copy;
					const GFXRECT r = cmd->rect;

					if (cmd->destBuffer == TFB_SCREEN_MAIN)
					{
						TFB_BBox_RegisterRect(&cmd->rect);
					}

					TFB_DrawCanvas_CopyRect(
						TFB_GetScreenCanvas(cmd->srcBuffer), &r,
						TFB_GetScreenCanvas(cmd->destBuffer), r.corner);
					break;
				}

			case TFB_DRAWCOMMANDTYPE_DELETEIMAGE:
				{
					TFB_Image* DC_image = DC.data.deleteimage.image;
					TFB_DrawImage_Delete(DC_image);
					break;
				}

			case TFB_DRAWCOMMANDTYPE_DELETEDATA:
				{
					delete[] DC.data.deletedata.data;
					break;
				}

			case TFB_DRAWCOMMANDTYPE_SENDSIGNAL:
				DC.data.sendsignal.sem->release();
				break;

			case TFB_DRAWCOMMANDTYPE_REINITVIDEO:
				{
					TFB_DrawCommand_ReinitVideo* cmd = &DC.data.reinitvideo;
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
					TFB_SwapBuffers(uqm::TFBRedraw::Yes);
					break;
				}

			case TFB_DRAWCOMMANDTYPE_CALLBACK:
				{
					DC.data.callback.callback(DC.data.callback.arg);
					break;
				}
		}
	}

	if (livelock_deterrence)
	{
		Unlock_DCQ();
	}

	if (testFlag(g_gfxFlags, uqm::GfxFlags::ShowFPS))
	{
		RenderFPS(&fps);
	}

	TFB_SwapBuffers(uqm::TFBRedraw::No);
	RenderedFrames++;
	s_renderingDone.notify_all();
}

void TFB_PurgeDanglingGraphics(void)
{
	Lock_DCQ(-1);

	for (;;)
	{
		TFB_DrawCommand DC;

		if (!TFB_DrawCommandQueue_Pop(&DC))
		{
			// the Queue is now empty.
			break;
		}

		switch (DC.Type)
		{
			case TFB_DRAWCOMMANDTYPE_DELETEIMAGE:
				{
					TFB_Image* DC_image = DC.data.deleteimage.image;
					TFB_DrawImage_Delete(DC_image);
					break;
				}
			case TFB_DRAWCOMMANDTYPE_DELETEDATA:
				{
					delete[] DC.data.deletedata.data;
					break;
				}
			case TFB_DRAWCOMMANDTYPE_IMAGE:
				{
					TFB_ColorMap* cmap = DC.data.image.colormap;
					if (cmap)
					{
						TFB_ReturnColorMap(cmap);
					}
					break;
				}
			case TFB_DRAWCOMMANDTYPE_SENDSIGNAL:
				{
					DC.data.sendsignal.sem->release();
					break;
				}
		}
	}
	Unlock_DCQ();
}
