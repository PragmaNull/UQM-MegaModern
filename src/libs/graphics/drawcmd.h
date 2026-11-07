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

struct TFB_DrawCommand_Line
{
	int x1, y1, x2, y2;
	Color color;
	DrawMode drawMode;
	ScreenBuffer destBuffer;
	uint8_t thickness;
};

struct TFB_DrawCommand_Rect
{
	GfxRect rect;
	Color color;
	DrawMode drawMode;
	ScreenBuffer destBuffer;
};

struct TFB_DrawCommand_Image
{
	TFB_Image* image;
	int x, y;
	ScreenBuffer destBuffer;
	TFB_ColorMap* colormap;
	DrawMode drawMode;
	int scale;
	uqm::TFBScaleMode scaleMode;
};

struct TFB_DrawCommand_FilledImage
{
	TFB_Image* image;
	int x, y;
	Color color;
	ScreenBuffer destBuffer;
	DrawMode drawMode;
	int scale;
	uqm::TFBScaleMode scaleMode;
};

struct TFB_DrawCommand_FontChar
{
	TFB_Char* fontchar;
	TFB_Image* backing;
	int x, y;
	DrawMode drawMode;
	ScreenBuffer destBuffer;
};

struct TFB_DrawCommand_Copy
{
	GfxRect rect;
	ScreenBuffer srcBuffer, destBuffer;
};

struct TFB_DrawCommand_CopyToImage
{
	TFB_Image* image;
	GfxRect rect;
	ScreenBuffer srcBuffer;
};

struct TFB_DrawCommand_Scissor
{
	GfxRect rect;
};

struct TFB_DrawCommand_SetMipmap
{
	TFB_Image* image;
	TFB_Image* mipmap;
	int hotx, hoty;
};

struct TFB_DrawCommand_DeleteImage
{
	TFB_Image* image;
};

struct TFB_DrawCommand_DeleteData
{
	// Made with new[].
	uint8_t* data;
};

struct TFB_DrawCommand_SendSignal
{
	uqstl::binary_semaphore* sem;
};

struct TFB_DrawCommand_ReinitVideo
{
	uqm::GfxDriver driver;
	uqm::GfxFlags flags;
	int width;
	int height;
};

struct TFB_DrawCommand_Callback
{
	void (*callback)(void* arg);
	void* arg;
};

struct TFB_DrawCommand
{
	DrawCommandType Type {};
	union
	{
		TFB_DrawCommand_Line line;
		TFB_DrawCommand_Rect rect;
		TFB_DrawCommand_Image image;
		TFB_DrawCommand_FilledImage filledimage;
		TFB_DrawCommand_FontChar fontchar;
		TFB_DrawCommand_Copy copy;
		TFB_DrawCommand_CopyToImage copytoimage;
		TFB_DrawCommand_Scissor scissor;
		TFB_DrawCommand_SetMipmap setmipmap;
		TFB_DrawCommand_DeleteImage deleteimage;
		TFB_DrawCommand_DeleteData deletedata;
		TFB_DrawCommand_SendSignal sendsignal;
		TFB_DrawCommand_ReinitVideo reinitvideo;
		TFB_DrawCommand_Callback callback;
	} data {};
};

// Queue Stuff

struct TFB_DrawCommandQueue
{
	int Front;
	int Back;
	int InsertionPoint;
	int Batching;
	int FullSize;
	int Size;
};

void Init_DrawCommandQueue(void);

void Uninit_DrawCommandQueue(void);

void TFB_BatchGraphics(void);

void TFB_UnbatchGraphics(void);

void TFB_BatchReset(void);

void TFB_DrawCommandQueue_Push(TFB_DrawCommand* Command);

int TFB_DrawCommandQueue_Pop(TFB_DrawCommand* Command);

void TFB_DrawCommandQueue_Clear(void);

void TFB_EnqueueDrawCommand(TFB_DrawCommand* DrawCommand);

void Lock_DCQ(int slots);

void Unlock_DCQ(void);

#endif // UQM_LIBS_GRAPHICS_DRAWCMD_H
