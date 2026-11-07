//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#pragma once
#ifndef UQM_LIBS_GRAPHICS_DRAWCMD_H
#define UQM_LIBS_GRAPHICS_DRAWCMD_H

#include "libs/graphics/tfb_draw.h"

#include "core/stl/semaphore.h"

enum class DrawCommandType
{
	Line,
	Rectangle,
	Image,
	FilledImage,
	FontChar,

	Copy,
	CopyToImage,

	ScissorEnable,
	ScissorDisable,

	SetMipmap,
	DeleteImage,
	DeleteData,
	SendSignal,
	ReinitVideo,
	Callback,
};

namespace tfb
{
struct DrawCommandLine
{
	int x1, y1, x2, y2;
	Color color;
	DrawMode drawMode;
	ScreenBuffer destBuffer;
	uint8_t thickness;
};

struct DrawCommandRect
{
	GfxRect rect;
	Color color;
	DrawMode drawMode;
	ScreenBuffer destBuffer;
};

struct DrawCommandImage
{
	Image* image;
	int x, y;
	ScreenBuffer destBuffer;
	ColorMap* colormap;
	DrawMode drawMode;
	int scale;
	ScaleMode scaleMode;
};

struct DrawCommandFilledImage
{
	Image* image;
	int x, y;
	Color color;
	ScreenBuffer destBuffer;
	DrawMode drawMode;
	int scale;
	ScaleMode scaleMode;
};

struct DrawCommandFontChar
{
	Char* fontchar;
	Image* backing;
	int x, y;
	DrawMode drawMode;
	ScreenBuffer destBuffer;
};

struct DrawCommandCopy
{
	GfxRect rect;
	ScreenBuffer srcBuffer, destBuffer;
};

struct DrawCommandCopyToImage
{
	Image* image;
	GfxRect rect;
	ScreenBuffer srcBuffer;
};

struct DrawCommandScissor
{
	GfxRect rect;
};

struct DrawCommandSetMipmap
{
	Image* image;
	Image* mipmap;
	int hotx, hoty;
};

struct DrawCommandDeleteImage
{
	Image* image;
};

struct DrawCommandDeleteData
{
	// Made with new[].
	uint8_t* data;
};

struct DrawCommandSendSignal
{
	uqstl::binary_semaphore* sem;
};

struct DrawCommandReinitVideo
{
	uqm::GfxDriver driver;
	uqm::GfxFlags flags;
	int width;
	int height;
};

struct DrawCommandCallback
{
	void (*callback)(void* arg);
	void* arg;
};

struct DrawCommand
{
	DrawCommandType Type {};
	union
	{
		DrawCommandLine line;
		DrawCommandRect rect;
		DrawCommandImage image;
		DrawCommandFilledImage filledimage;
		DrawCommandFontChar fontchar;
		DrawCommandCopy copy;
		DrawCommandCopyToImage copytoimage;
		DrawCommandScissor scissor;
		DrawCommandSetMipmap setmipmap;
		DrawCommandDeleteImage deleteimage;
		DrawCommandDeleteData deletedata;
		DrawCommandSendSignal sendsignal;
		DrawCommandReinitVideo reinitvideo;
		DrawCommandCallback callback;
	} data {};
};
} // namespace tfb

// Queue Stuff

namespace tfb
{
struct DrawCommandQueue
{
	int Front;
	int Back;
	int InsertionPoint;
	int Batching;
	int FullSize;
	int Size;
};
} // namespace tfb

void Init_DrawCommandQueue(void);

void Uninit_DrawCommandQueue(void);

void TFB_BatchGraphics(void);

void TFB_UnbatchGraphics(void);

void TFB_BatchReset(void);

void TFB_DrawCommandQueue_Push(tfb::DrawCommand* Command);

int TFB_DrawCommandQueue_Pop(tfb::DrawCommand* Command);

void TFB_DrawCommandQueue_Clear(void);

void TFB_EnqueueDrawCommand(tfb::DrawCommand* DrawCommand);

void Lock_DCQ(int slots);

void Unlock_DCQ(void);

#endif // UQM_LIBS_GRAPHICS_DRAWCMD_H
