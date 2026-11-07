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

/* DUCK video player
 *
 * Status: fully functional
 */

#include "video.h"
#include "dukvid.h"
#include <stdio.h>
#include <string.h>
#include "core/misc/bittricks.h"
#include "uqm/units.h"
#include "core/string/StringUtils.h"


const static char* dukv_GetName(void);
static bool dukv_InitModule(int flags);
static void dukv_TermModule(void);
static tfb::VideoDecoder* dukv_Create(void);
static void dukv_Destroy(tfb::VideoDecoder* decoder);
static int dukv_GetError(tfb::VideoDecoder* This);
static bool dukv_Init(tfb::VideoDecoder* This, tfb::PixelFormat* fmt);
static void dukv_Term(tfb::VideoDecoder* This);
static bool dukv_Open(tfb::VideoDecoder* This, const uqm::io::Directory& dir, const char* filename);
static void dukv_Close(tfb::VideoDecoder* This);
static int dukv_DecodeNext(tfb::VideoDecoder* This);
static uint32_t dukv_SeekFrame(tfb::VideoDecoder* This, uint32_t frame);
static float dukv_SeekTime(tfb::VideoDecoder* This, float time);
static uint32_t dukv_GetFrame(tfb::VideoDecoder* This);
static float dukv_GetTime(tfb::VideoDecoder* This);

tfb::VideoDecoderFuncs dukv_DecoderVtbl =
	{
		dukv_GetName,
		dukv_InitModule,
		dukv_TermModule,
		dukv_Create,
		dukv_Destroy,
		dukv_GetError,
		dukv_Init,
		dukv_Term,
		dukv_Open,
		dukv_Close,
		dukv_DecodeNext,
		dukv_SeekFrame,
		dukv_SeekTime,
		dukv_GetFrame,
		dukv_GetTime,
};

namespace tfb
{
struct DuckVideoHeader
{
	uint32_t version;
	uint32_t scrn_x_ofs; // horz screen offset in pixels
	uint32_t scrn_y_ofs; // vert screen offset in pixels
	uint16_t wb, hb;	 // width + height in blocks
	int16_t lumas[8];	 // future luminance deltas
	int16_t chromas[8];	 // future chrominance deltas
};
} // namespace tfb

static constexpr auto NumVecItems {0x010};
static constexpr auto NumVectors {0x100};

namespace tfb
{
struct DuckVideoDeltas
{
	int32_t lumas[NumVectors][NumVecItems];
	int32_t chromas[NumVectors][NumVecItems];
};
} // namespace tfb

// specific video decoder struct derived from tfb::VideoDecoder
// the only sane way in C one can :)
namespace tfb
{
struct DuckVideoDecoder
{
	// always the first member
	VideoDecoder decoder;

	int32_t last_error;
	uqstl::string basename;
	uqgsl::owner<uqm::io::File*> stream;

	// loaded from disk
	uqstl::vector<uint32_t> frames;
	uint32_t cframes;
	uint32_t iframe;
	uint32_t version;
	uint32_t wb, hb; // width, height in blocks

	// generated
	DuckVideoDeltas d;

	uqstl::vector<uint8_t> inbuf;
	uqstl::vector<uint32_t> decbuf;
};
} // namespace tfb

static constexpr auto DuckGeneralFps {14.622f};
static constexpr auto DuckMaxFrameSize {0x8000U};
static constexpr auto DuckEndOfSequence {1};

static void dukv_DecodeFrame(uint8_t* src_p, uint32_t* dst_p, uint32_t wb, uint32_t hb,
							 tfb::DuckVideoDeltas* deltas)
{
	uint32_t x {};
	int32_t w {};
	uint32_t* d_p0 {};
	uint32_t* d_p1 {};
	int i {};

	w = wb * 4;

	int iVec {*(src_p++)};
	int iSeq {0};

	for (uint32_t y {0}; y < hb; ++y)
	{
		int32_t corr {};
		int32_t delta {};
		int32_t pix[4];

		d_p0 = dst_p + y * w * 2;
		d_p1 = d_p0 + w;

		int32_t accum0 {0};
		int32_t accum1 {0};
		int32_t corr0 {0};
		int32_t corr1 {0};

		for (x = 0; x < wb; ++x)
		{
			if (y == 0)
			{
				pix[0] = pix[1] = pix[2] = pix[3] = 0;
			}
			else
			{
				const uint32_t* p_p = d_p0 - w;
				pix[0] = p_p[0];
				pix[1] = p_p[1];
				pix[2] = p_p[2];
				pix[3] = p_p[3];
			}

			// start with chroma delta
			delta = deltas->chromas[iVec][iSeq++];
			iSeq++; // correctors ignored

			accum0 += delta >> 1;
			if (delta & 1)
			{
				iVec = *(src_p++);
				iSeq = 0;
			}

			// line 0
			for (i = 0; i < 4; ++i, ++d_p0)
			{
				delta = deltas->lumas[iVec][iSeq++];
				corr = deltas->lumas[iVec][iSeq++];

				accum0 += delta >> 1;
				corr0 ^= corr;
				pix[i] += accum0;
				pix[i] ^= corr0;

				if (delta & 1)
				{
					iVec = *(src_p++);
					iSeq = 0;
				}

				*d_p0 = pix[i];
			}

			// line 1
			for (i = 0; i < 4; ++i, ++d_p1)
			{
				delta = deltas->lumas[iVec][iSeq++];
				corr = deltas->lumas[iVec][iSeq++];

				accum1 += delta >> 1;
				corr1 ^= corr;
				pix[i] += accum1;
				pix[i] ^= corr1;

				if (delta & 1)
				{
					iVec = *(src_p++);
					iSeq = 0;
				}

				*d_p1 = pix[i];
			}
		}
	}
}

static void dukv_DecodeFrameV3(uint8_t* src_p, uint32_t* dst_p, uint32_t wb, uint32_t hb,
							   tfb::DuckVideoDeltas* deltas)
{
	uint32_t x {};
	int32_t w {};
	uint32_t* d_p {};
	int i {};

	int iVec {*(src_p++)};
	int iSeq {0};

	hb *= 2;
	w = wb * 4;

	for (uint32_t y {0}; y < hb; ++y)
	{
		int32_t delta {};
		int32_t pix {};

		d_p = dst_p + y * w;

		int32_t accum {0};

		for (x = 0; x < wb; ++x)
		{
			// start with chroma delta
			delta = deltas->chromas[iVec][iSeq];
			iSeq += 2; // correctors ignored

			accum += delta >> 1;

			if (delta & DuckEndOfSequence)
			{
				iVec = *(src_p++);
				iSeq = 0;
			}

			for (i = 0; i < 4; ++i, ++d_p)
			{
				if (y == 0)
				{
					pix = 0;
				}
				else
				{
					pix = d_p[-w];
				}

				// get next luma delta
				delta = deltas->lumas[iVec][iSeq];
				iSeq += 2; // correctors ignored

				accum += delta >> 1;
				pix += accum;

				if (delta & DuckEndOfSequence)
				{
					iVec = *(src_p++);
					iSeq = 0;
				}

				*d_p = pix;
			}
		}
	}
}

static bool dukv_OpenStream(tfb::DuckVideoDecoder* dukv, const uqm::io::Directory& dir)
{
	char filename[280] {};
	fmt::format_to_sz_n(filename, "{}.duk", dukv->basename);

	dukv->stream = res_NewResFile(dir, filename);
	return dukv->stream != nullptr;
}

static bool dukv_ReadFrames(tfb::DuckVideoDecoder* dukv, const uqm::io::Directory& dir)
{
	char filename[280] {};
	uqstl::optional<uqm::io::File> fp {};

	fmt::format_to_sz_n(filename, "{}.frm", dukv->basename);

	if (!(fp = res_OpenResFile(dir, filename, uqm::io::OpenMode::Read)))
	{
		return false;
	}

	// get number of frames
	dukv->cframes = static_cast<uint32_t>(fp->size() / sizeof(uint32_t));
	dukv->frames.assign(dukv->cframes, 0);

	if (!fp->readExact(uqm::io::asBytes(dukv->frames.data(), sizeof(uint32_t) * dukv->cframes)))
	{
		dukv->frames.clear();
		return false;
	}
	fp.reset();

	for (uint32_t i {0}; i < dukv->cframes; ++i)
	{
		dukv->frames[i] = uqm::convertBigEndian<uint32_t>(dukv->frames[i]);
	}

	return true;
}

static bool dukv_ReadVectors(tfb::DuckVideoDecoder* dukv, const uqm::io::Directory& dir, uint8_t* vectors)
{
	uqstl::optional<uqm::io::File> fp {};
	char filename[280] {};

	fmt::format_to_sz_n(filename, "{}.tbl", dukv->basename);

	if (!(fp = res_OpenResFile(dir, filename, uqm::io::OpenMode::Read)))
	{
		return false;
	}

	const int ret {static_cast<int>(fp->read(uqm::io::asBytes(vectors, NumVecItems * NumVectors)) / NumVecItems)};
	fp.reset();

	return ret == NumVectors;
}

static bool dukv_ReadHeader(tfb::DuckVideoDecoder* dukv, const uqm::io::Directory& dir, int32_t* pl, int32_t* pc)
{
	uqstl::optional<uqm::io::File> fp {};
	char filename[280] {};
	tfb::DuckVideoHeader hdr;

	fmt::format_to_sz_n(filename, "{}.hdr", dukv->basename);

	if (!(fp = res_OpenResFile(dir, filename, uqm::io::OpenMode::Read)))
	{
		return false;
	}

	const int ret {fp->readValue(hdr) ? 1 : 0};
	fp.reset();
	if (!ret)
	{
		return false;
	}

	dukv->version = uqm::convertBigEndian<uint32_t>(hdr.version);
	dukv->wb = uqm::convertBigEndian<uint16_t>(hdr.wb);
	dukv->hb = uqm::convertBigEndian<uint16_t>(hdr.hb);

	for (int i {0}; i < 8; ++i)
	{
		pl[i] = (int16_t)uqm::convertBigEndian<uint16_t>(hdr.lumas[i]);
		pc[i] = (int16_t)uqm::convertBigEndian<uint16_t>(hdr.chromas[i]);
	}

	dukv->decoder.w = dukv->wb * 4;
	dukv->decoder.h = dukv->hb * 4;

	return true;
}

static int32_t dukv_make_delta(int32_t* protos, bool is_chroma, int i1, int i2)
{
	int32_t d1 {};
	int32_t d2 {};

	if (!is_chroma)
	{
		// 0x421 is (r,g,b)=(1,1,1) in 15bit pixel coding
		d1 = (protos[i1] >> 1) * 0x421;
		d2 = (protos[i2] >> 1) * 0x421;
		return ((d1 << 16) + d2) << 1;
	}
	else
	{
		d1 = (protos[i1] << 10) + protos[i2];
		return ((d1 << 16) + d1) << 1;
	}
}

static int32_t dukv_make_corr(int32_t* protos, bool is_chroma, int i1, int i2)
{
	int32_t d1 {};
	int32_t d2 {};

	if (!is_chroma)
	{
		d1 = (protos[i1] & 1) << 15;
		d2 = (protos[i2] & 1) << 15;
		return (d1 << 16) + d2;
	}
	else
	{
		return (i1 << 3) + i2;
	}
}

static void dukv_DecodeVector(uint8_t* vec, int32_t* p, bool is_chroma, int32_t* deltas)
{
	const int citems = vec[0];

	for (int i {0}; i < citems; i += 2, vec += 2, deltas += 2)
	{
		int32_t d = dukv_make_delta(p, is_chroma, vec[1], vec[2]);

		if (i == citems - 2)
		{
			d |= DuckEndOfSequence;
		}

		deltas[0] = d;
		deltas[1] = dukv_make_corr(p, is_chroma, vec[1], vec[2]);
	}
}

static void dukv_InitDeltas(tfb::DuckVideoDecoder* dukv, uint8_t* vectors,
							int32_t* pl, int32_t* pc)
{
	for (int i {0}; i < NumVectors; ++i)
	{
		uint8_t* vector = vectors + i * NumVecItems;
		dukv_DecodeVector(vector, pl, false, dukv->d.lumas[i]);
		dukv_DecodeVector(vector, pc, true, dukv->d.chromas[i]);
	}
}

static inline uint32_t dukv_PixelConv(uint16_t pix, const tfb::PixelFormat* fmt)
{
	uint32_t r {};
	uint32_t g {};
	uint32_t b {};

	r = (pix >> 7) & 0xf8;
	g = (pix >> 2) & 0xf8;
	b = (pix << 3) & 0xf8;

	return ((r >> fmt->Rloss) << fmt->Rshift) | ((g >> fmt->Gloss) << fmt->Gshift) | ((b >> fmt->Bloss) << fmt->Bshift);
}

static void dukv_RenderFrame(tfb::VideoDecoder* This)
{
	tfb::DuckVideoDecoder* dukv = (tfb::DuckVideoDecoder*)This;
	const tfb::PixelFormat* fmt = This->format;
	uint32_t h {};
	uint32_t x {};
	uint32_t y {};
	uint32_t pair {};
	uint32_t* dec = dukv->decbuf.data();
	uint32_t bufInc = 0;
	const int scale = 4;

	h = dukv->decoder.h / 2;

	// separate bpp versions for speed
	switch (fmt->BytesPerPixel)
	{
		case 2:
			for (y = 0; y < h; ++y)
			{
				uint16_t* dst0 {};
				uint16_t* dst1 {};

				dst0 = (uint16_t*)This->callbacks.GetCanvasLine(
					This, y * 2 + IfHd(1));
				dst1 = (uint16_t*)This->callbacks.GetCanvasLine(
					This, y * 2 + 1);

				if (IsHd() && y % scale != 0)
				{
					dec -= dukv->decoder.w;
				}

				for (x = 0; x < dukv->decoder.w; ++x, chooseIfHd(++dec, reinterpret_cast<uint32_t*>(++bufInc)), ++dst0, ++dst1)
				{
					/*if (IsHd())
						++bufInc;
					else
						++dec;*/

					if (bufInc % scale == 0 && IsHd())
					{
						dec++;
					}

					pair = *dec;
					*dst0 = dukv_PixelConv((uint16_t)(pair >> 16), fmt);
					*dst1 = dukv_PixelConv((uint16_t)(pair & 0xffff), fmt);
				}
			}
			break;
		case 3:
			for (y = 0; y < h; ++y)
			{
				uint8_t* dst0 {};
				uint8_t* dst1 {};

				dst0 = (uint8_t*)This->callbacks.GetCanvasLine(
					This, y * 2 + IfHd(1));
				dst1 = (uint8_t*)This->callbacks.GetCanvasLine(
					This, y * 2 + 1);

				if (IsHd() && y % scale != 0)
				{
					dec -= dukv->decoder.w;
				}

				for (x = 0; x < dukv->decoder.w; ++x, chooseIfHd(++dec, reinterpret_cast<uint32_t*>(++bufInc)), dst0 += 3, dst1 += 3)
				{
					/*if (IsHd())
						++bufInc;
					else
						++dec;*/

					if (bufInc % scale == 0 && IsHd())
					{
						dec++;
					}

					pair = *dec;
					*(uint32_t*)dst0 =
						dukv_PixelConv((uint16_t)(pair >> 16), fmt);
					*(uint32_t*)dst1 =
						dukv_PixelConv((uint16_t)(pair & 0xffff), fmt);
				}
			}
			break;
		case 4:
			for (y = 0; y < h; ++y)
			{
				uint32_t* dst0 {};
				uint32_t* dst1 {};

				dst0 = (uint32_t*)This->callbacks.GetCanvasLine(
					This, y * 2);
				dst1 = (uint32_t*)This->callbacks.GetCanvasLine(
					This, y * 2 + 1);

				if (IsHd() && y % scale != 0)
				{
					dec -= ResDescale(dukv->decoder.w);
				}

				for (x = 0; x < dukv->decoder.w; ++x, chooseIfHd(++dec, reinterpret_cast<uint32_t*>(++bufInc)), ++dst0, ++dst1)
				{
					/*if (IsHd())
						++bufInc;
					else
						++dec;*/

					if (bufInc % scale == 0 && IsHd())
					{
						dec++;
					}

					pair = *dec;
					*dst0 = dukv_PixelConv((uint16_t)(pair >> 16), fmt);
					*dst1 = dukv_PixelConv((uint16_t)(pair & 0xffff), fmt);
				}
			}
			break;
		default:
			break;
	}
}

const static char* dukv_GetName(void)
{
	return "DukVid";
}

static bool dukv_InitModule(int flags)
{
	// no flags are defined for now
	return true;

	(void)flags; // dodge compiler warning
}

static void dukv_TermModule(void)
{
	// do an extensive search on the word 'nothing'
}

static tfb::VideoDecoder* dukv_Create(void)
{
	return &(new tfb::DuckVideoDecoder {})->decoder;
}

static void dukv_Destroy(tfb::VideoDecoder* decoder)
{
	delete reinterpret_cast<tfb::DuckVideoDecoder*>(decoder);
}

static int dukv_GetError(tfb::VideoDecoder* This)
{
	return This->error;
}

static bool dukv_Init(tfb::VideoDecoder* This, tfb::PixelFormat* fmt)
{
	This->format = fmt;
	This->audio_synced = true;
	return true;
}

static void dukv_Term(tfb::VideoDecoder* This)
{
	//tfb::DuckVideoDecoder* dukv = (tfb::DuckVideoDecoder*) This;

	dukv_Close(This);
}

static bool dukv_Open(tfb::VideoDecoder* This, const uqm::io::Directory& dir, const char* filename)
{
	tfb::DuckVideoDecoder* dukv = (tfb::DuckVideoDecoder*)This;
	int32_t lumas[8];
	int32_t chromas[8];
	uqstl::vector<uint8_t> vectors(NumVecItems * NumVectors);

	dukv->basename = filename;
	if (const size_t ext {dukv->basename.rfind('.')}; ext != uqstl::string::npos) // strip extension
	{
		dukv->basename.resize(ext);
	}

	if (!dukv_OpenStream(dukv, dir)
		|| !dukv_ReadFrames(dukv, dir)
		|| !dukv_ReadHeader(dukv, dir, lumas, chromas)
		|| !dukv_ReadVectors(dukv, dir, vectors.data()))
	{
		dukv_Close(This);
		dukv->last_error = *DukVidError::BadFile;
		return false;
	}

	dukv_InitDeltas(dukv, vectors.data(), lumas, chromas);

	This->length = (float)dukv->cframes / DuckGeneralFps;
	This->frame_count = dukv->cframes;
	This->interframe_wait = (uint32_t)(1000.0 / DuckGeneralFps);

	dukv->inbuf.assign(DuckMaxFrameSize, 0);
	// The buffer holds w * h 16-bit values.
	dukv->decbuf.assign((dukv->decoder.w * dukv->decoder.h + 1) / 2, 0);

	return true;
}

static void dukv_Close(tfb::VideoDecoder* This)
{
	tfb::DuckVideoDecoder* dukv = (tfb::DuckVideoDecoder*)This;

	dukv->basename.clear();
	dukv->frames = {};
	delete dukv->stream;
	dukv->stream = nullptr;
	dukv->inbuf = {};
	dukv->decbuf = {};
}

static int dukv_DecodeNext(tfb::VideoDecoder* This)
{
	tfb::DuckVideoDecoder* dukv = (tfb::DuckVideoDecoder*)This;
	uint32_t fh[2];

	if (!dukv->stream || dukv->iframe >= dukv->cframes)
	{
		return 0;
	}

	dukv->stream->seek(dukv->frames[dukv->iframe], uqm::io::SeekFrom::Start);
	if (!dukv->stream->readValue(fh))
	{
		dukv->last_error = *DukVidError::Eof;
		return 0;
	}

	const uint32_t vofs {uqm::convertBigEndian<uint32_t>(fh[0])};
	const uint32_t vsize {uqm::convertBigEndian<uint32_t>(fh[1])};
	if (vsize > DuckMaxFrameSize)
	{
		dukv->last_error = *DukVidError::OutOfBuf;
		return -1;
	}

	dukv->stream->seek(vofs, uqm::io::SeekFrom::Current);
	if (!dukv->stream->readExact(uqm::io::asBytes(dukv->inbuf.data(), vsize)))
	{
		dukv->last_error = *DukVidError::Eof;
		return 0;
	}

	const uint16_t ver {uqm::convertBigEndian<uint16_t>(*(uint16_t*)dukv->inbuf.data())};
	if (ver == 0x0300)
	{
		dukv_DecodeFrameV3(dukv->inbuf.data() + 0x10, dukv->decbuf.data(),
						   dukv->wb, dukv->hb, &dukv->d);
	}
	else
	{
		dukv_DecodeFrame(dukv->inbuf.data() + 0x10, dukv->decbuf.data(),
						 dukv->wb, dukv->hb, &dukv->d);
	}

	dukv->iframe++;

	This->callbacks.BeginFrame(This);
	dukv_RenderFrame(This);
	This->callbacks.EndFrame(This);

	if (!This->audio_synced)
	{
		This->callbacks.SetTimer(This, (uint32_t)(1000.0f / DuckGeneralFps));
	}

	return 1;
}

static uint32_t dukv_SeekFrame(tfb::VideoDecoder* This, uint32_t frame)
{
	tfb::DuckVideoDecoder* dukv = (tfb::DuckVideoDecoder*)This;

	if (frame > dukv->cframes)
	{
		frame = dukv->cframes; // EOS
	}

	return dukv->iframe = frame;
}

static float dukv_SeekTime(tfb::VideoDecoder* This, float time)
{
	//tfb::DuckVideoDecoder* dukv = (tfb::DuckVideoDecoder*) This;
	const uint32_t frame = (uint32_t)(time * DuckGeneralFps);

	// Note that DuckGeneralFps is a float constant
	return dukv_SeekFrame(This, frame) / DuckGeneralFps;
}

static uint32_t dukv_GetFrame(tfb::VideoDecoder* This)
{
	const tfb::DuckVideoDecoder* dukv = (tfb::DuckVideoDecoder*)This;

	return dukv->iframe;
}

static float dukv_GetTime(tfb::VideoDecoder* This)
{
	const tfb::DuckVideoDecoder* dukv = (tfb::DuckVideoDecoder*)This;

	return (float)dukv->iframe / DuckGeneralFps;
}
