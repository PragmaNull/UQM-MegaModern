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

/* .duk sound track decoder
 */

#include "dukaud.h"

#include <cstdint>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "core/misc/bittricks.h"
#include "core/string/StringUtils.h"
#include "decoder.h"
#include "libs/reslib.h"

static constexpr auto DataBufSize {0x8000};
static constexpr auto DuckGeneralFps {14.622f};


const static char* duka_GetName(void);
static bool duka_InitModule(AudioFlags flags, const tfb::DecoderFormats*);
static void duka_TermModule(void);
static tfb::SoundDecoder* duka_Create(void);
static void duka_Destroy(tfb::SoundDecoder* decoder);
static int duka_GetError(tfb::SoundDecoder* This);
static bool duka_Init(tfb::SoundDecoder* This);
static void duka_Term(tfb::SoundDecoder* This);
static bool duka_Open(tfb::SoundDecoder* This, const uqm::io::Directory& dir, const char* filename);
static void duka_Close(tfb::SoundDecoder* This);
static int duka_Decode(tfb::SoundDecoder* This, void* buf, int32_t bufsize);
static uint32_t duka_Seek(tfb::SoundDecoder* This, uint32_t pcm_pos);
static uint32_t duka_GetFrame(tfb::SoundDecoder* This);

tfb::SoundDecoderFuncs duka_DecoderVtbl =
	{
		duka_GetName,
		duka_InitModule,
		duka_TermModule,
		duka_Create,
		duka_Destroy,
		duka_GetError,
		duka_Init,
		duka_Term,
		duka_Open,
		duka_Close,
		duka_Decode,
		duka_Seek,
		duka_GetFrame,
};

namespace tfb
{
struct DuckSoundDecoder
{
	// always the first member
	SoundDecoder decoder;

	// public read-only
	uint32_t iframe;	// current frame index
	uint32_t cframes;	// total count of frames
	uint32_t channels;	// number of channels
	uint32_t pcm_frame; // samples per frame

	// private
	int32_t last_error;
	uqgsl::owner<uqm::io::File*> duk;
	uqstl::vector<uint32_t> frames;
	// buffer
	uqstl::vector<uint8_t> data;
	uint32_t maxdata;
	uint32_t cbdata;
	uint32_t dataofs;
	// decoder stuff
	uqstl::array<int32_t, 2> predictors;
};
} // namespace tfb


struct DukAud_FrameHeader
{
	uint32_t audsize;
	uint32_t vidsize;
};

struct DukAud_AudSubframe
{
	uint16_t magic; // always 0xf77f
	uint16_t numsamples;
	uint16_t tag;
	uqstl::array<uint16_t, 2> indices; // initial indices for channels
};

const static tfb::DecoderFormats* duka_formats = nullptr;

static int32_t duka_readAudFrameHeader(tfb::DuckSoundDecoder* duka, uint32_t iframe,
									   DukAud_AudSubframe* aud)
{
	DukAud_FrameHeader hdr;

	duka->duk->seek(duka->frames[iframe], uqm::io::SeekFrom::Start);
	if (!duka->duk->readValue(hdr))
	{
		duka->last_error = errno;
		return *DukAudError::BadFile;
	}
	hdr.audsize = uqm::convertBigEndian<uint32_t>(hdr.audsize);

	if (!duka->duk->readValue(*aud))
	{
		duka->last_error = errno;
		return *DukAudError::BadFile;
	}

	aud->magic = uqm::convertBigEndian<uint16_t>(aud->magic);
	if (aud->magic != 0xf77f)
	{
		return duka->last_error = *DukAudError::BadFile;
	}

	aud->numsamples = uqm::convertBigEndian<uint16_t>(aud->numsamples);
	aud->tag = uqm::convertBigEndian<uint16_t>(aud->tag);
	aud->indices[0] = uqm::convertBigEndian<uint16_t>(aud->indices[0]);
	aud->indices[1] = uqm::convertBigEndian<uint16_t>(aud->indices[1]);

	return 0;
}

// This table is from one of the files that came with the original 3do source
// It's slightly different from the data used by MPlayer.
static uqstl::array<int, 89> adpcm_step {
	0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xF,
	0x10, 0x12, 0x13, 0x15, 0x17, 0x1A, 0x1C, 0x1F,
	0x22, 0x26, 0x29, 0x2E, 0x32, 0x37, 0x3D, 0x43,
	0x4A, 0x51, 0x59, 0x62, 0x6C, 0x76, 0x82, 0x8F,
	0x9E, 0xAD, 0xBF, 0xD2, 0xE7, 0xFE, 0x117, 0x133,
	0x152, 0x174, 0x199, 0x1C2, 0x1EF, 0x220, 0x256, 0x292,
	0x2D4, 0x31D, 0x36C, 0x3C4, 0x424, 0x48E, 0x503, 0x583,
	0x610, 0x6AC, 0x756, 0x812, 0x8E1, 0x9C4, 0xABE, 0xBD1,
	0xCFF, 0xE4C, 0xFBA, 0x114D, 0x1308, 0x14EF, 0x1707, 0x1954,
	0x1BDD, 0x1EA6, 0x21B7, 0x2516,
	0x28CB, 0x2CDF, 0x315C, 0x364C,
	0x3BBA, 0x41B2, 0x4844, 0x4F7E,
	0x5771, 0x6030, 0x69CE, 0x7463,
	0x7FFF};


// *** BEGIN part copied from MPlayer ***
// (some little changes)

#if 0
// pertinent tables for IMA ADPCM
static uqstl::array<int, 89> adpcm_step {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
		19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
		50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
		130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
		337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
		876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
		2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
		5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
		};
#endif

static uqstl::array<int, 16> adpcm_index {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8};

static void decode_nibbles(int16_t* output, int32_t output_size, int32_t channels,
						   int32_t* predictors, uint16_t* indices)
{
	uqstl::array<int32_t, 2> step;
	uqstl::array<int32_t, 2> index;
	int32_t diff {};
	int sign {};
	int32_t delta {};
	int channel_number = 0;

	channels -= 1;
	index[0] = indices[0];
	index[1] = indices[1];
	step[0] = adpcm_step[index[0]];
	step[1] = adpcm_step[index[1]];

	for (int32_t i {0}; i < output_size; i++)
	{
		delta = output[i];

		index[channel_number] += adpcm_index[delta];
		index[channel_number] = uqstl::clamp(index[channel_number], 0, 88);

		sign = delta & 8;
		delta = delta & 7;

#if 0
		// fast approximation, used in most decoders
		diff = step[channel_number] >> 3;
		if (delta & 4) diff += step[channel_number];
		if (delta & 2) diff += step[channel_number] >> 1;
		if (delta & 1) diff += step[channel_number] >> 2;
#else
		// real thing
		//		diff = ((signed)delta + 0.5) * step[channel_number] / 4;
		diff = (((delta << 1) + 1) * step[channel_number]) >> 3;
#endif

		if (sign)
		{
			predictors[channel_number] -= diff;
		}
		else
		{
			predictors[channel_number] += diff;
		}

		predictors[channel_number] = uqstl::clamp(predictors[channel_number], -32768, 32767);
		output[i] = predictors[channel_number];
		step[channel_number] = adpcm_step[index[channel_number]];

		// toggle channel
		channel_number ^= channels;
	}
}
// *** END part copied from MPlayer ***

static int32_t duka_decodeFrame(tfb::DuckSoundDecoder* duka, DukAud_AudSubframe* header,
								uint8_t* input)
{
	int16_t* output {};
	int16_t* outptr {};
	int32_t outputsize {};

	outputsize = header->numsamples * 2 * sizeof(int16_t);
	outptr = output = (int16_t*)(duka->data.data() + duka->cbdata);

	for (const uint8_t* inend {input + header->numsamples}; input < inend; ++input)
	{
		*(outptr++) = *input >> 4;
		*(outptr++) = *input & 0x0f;
	}

	decode_nibbles(output, header->numsamples * 2, duka->channels,
				   duka->predictors.data(), header->indices.data());

	duka->cbdata += outputsize;

	return outputsize;
}


static int32_t duka_readNextFrame(tfb::DuckSoundDecoder* duka)
{
	DukAud_FrameHeader hdr;

	duka->duk->seek(duka->frames[duka->iframe], uqm::io::SeekFrom::Start);
	if (!duka->duk->readValue(hdr))
	{
		duka->last_error = errno;
		return *DukAudError::BadFile;
	}
	hdr.audsize = uqm::convertBigEndian<uint32_t>(hdr.audsize);

	// dump encoded data at the end of the buffer aligned on 8-byte
	uint8_t* p {(duka->data.data() + duka->maxdata - ((hdr.audsize + 7) & (-8)))};
	if (!duka->duk->readExact(uqm::io::asBytes(p, hdr.audsize)))
	{
		duka->last_error = errno;
		return *DukAudError::BadFile;
	}
	DukAud_AudSubframe* aud {(DukAud_AudSubframe*)p};
	p += sizeof(DukAud_AudSubframe);

	aud->magic = uqm::convertBigEndian<uint16_t>(aud->magic);
	if (aud->magic != 0xf77f)
	{
		return duka->last_error = *DukAudError::BadFile;
	}

	aud->numsamples = uqm::convertBigEndian<uint16_t>(aud->numsamples);
	aud->tag = uqm::convertBigEndian<uint16_t>(aud->tag);
	aud->indices[0] = uqm::convertBigEndian<uint16_t>(aud->indices[0]);
	aud->indices[1] = uqm::convertBigEndian<uint16_t>(aud->indices[1]);

	duka->iframe++;

	return duka_decodeFrame(duka, aud, p);
}

static int32_t duka_stuffBuffer(tfb::DuckSoundDecoder* duka, void* buf, int32_t bufsize)
{
	int32_t dataleft {};

	dataleft = duka->cbdata - duka->dataofs;
	if (dataleft > 0)
	{
		if (dataleft > bufsize)
		{
			dataleft = bufsize & (-4);
		}
		memcpy(buf, duka->data.data() + duka->dataofs, dataleft);
		duka->dataofs += dataleft;
	}

	if (duka->cbdata > 0 && duka->dataofs >= duka->cbdata)
	{
		duka->cbdata = duka->dataofs = 0; // reset for new data
	}

	return dataleft;
}


const static char* duka_GetName(void)
{
	return "DukAud";
}

static bool duka_InitModule(AudioFlags flags, const tfb::DecoderFormats* fmts)
{
	duka_formats = fmts;
	return true;

	uqstl::ignore = flags; // laugh at compiler warning
}

static void duka_TermModule(void)
{
	// no specific module term
}

static tfb::SoundDecoder* duka_Create(void)
{
	return &(new tfb::DuckSoundDecoder {})->decoder;
}

static void duka_Destroy(tfb::SoundDecoder* decoder)
{
	delete reinterpret_cast<tfb::DuckSoundDecoder*>(decoder);
}

static int duka_GetError(tfb::SoundDecoder* This)
{
	tfb::DuckSoundDecoder* duka = (tfb::DuckSoundDecoder*)This;
	const int ret = duka->last_error;
	duka->last_error = *DukAudError::None;
	return ret;
}

static bool duka_Init(tfb::SoundDecoder* This)
{
	//tfb::DuckSoundDecoder* duka = (tfb::DuckSoundDecoder*) This;
	This->need_swap =
		duka_formats->big_endian != duka_formats->want_big_endian;
	return true;
}

static void duka_Term(tfb::SoundDecoder* This)
{
	//tfb::DuckSoundDecoder* duka = (tfb::DuckSoundDecoder*) This;
	duka_Close(This); // ensure cleanup
}

static bool duka_Open(tfb::SoundDecoder* This, const uqm::io::Directory& dir, const char* file)
{
	tfb::DuckSoundDecoder* duka = (tfb::DuckSoundDecoder*)This;
	DukAud_AudSubframe aud;
	uqstl::array<char, 256> filename {};
	uint32_t filelen {};

	filelen = strlen(file);
	if (filelen > sizeof(filename) - 1)
	{
		return false;
	}
	uqstl::span<char> filenameDst {filename};
	uqm::strncpy_safe(filenameDst, {file, filelen});

	uqgsl::owner<uqm::io::File*> duk {res_NewResFile(dir, filename.data())};
	if (!duk)
	{
		duka->last_error = errno;
		return false;
	}

	// overwrite the extension with 'frm".. weird.
	filenameDst = filenameDst.subspan(filelen - 3);
	uqm::strncpy_safe(filenameDst, "frm");
	uqgsl::owner<uqm::io::File*> frm {res_NewResFile(dir, filename.data())};
	if (!frm)
	{
		duka->last_error = errno;
		delete duk;
		return false;
	}

	duka->duk = duk;

	duka->cframes = static_cast<uint32_t>(frm->size() / sizeof(uint32_t));
	if (!duka->cframes)
	{
		duka->last_error = *DukAudError::BadFile;
		delete frm;
		duka_Close(This);
		return false;
	}

	duka->frames.assign(duka->cframes, 0);
	const size_t cread {frm->read(uqm::io::asBytes(duka->frames.data(), sizeof(uint32_t) * duka->cframes)) / sizeof(uint32_t)};
	delete frm;
	if (cread != duka->cframes)
	{
		duka->last_error = *DukAudError::BadFile;
		duka_Close(This);
		return false;
	}

	for (uint32_t i {0}; i < duka->cframes; ++i)
	{
		duka->frames[i] = uqm::convertBigEndian<uint32_t>(duka->frames[i]);
	}

	if (duka_readAudFrameHeader(duka, 0, &aud) < 0)
	{
		duka_Close(This);
		return false;
	}

	This->frequency = 22050;
	This->format = duka_formats->stereo16;
	duka->channels = 2;
	duka->pcm_frame = aud.numsamples;
	duka->data.assign(DataBufSize, 0);
	duka->maxdata = DataBufSize;

	// estimate
	This->length = (float)duka->cframes / DuckGeneralFps;

	This->filename_hash = crc32b(file);

	duka->last_error = 0;

	return true;
}

static void duka_Close(tfb::SoundDecoder* This)
{
	tfb::DuckSoundDecoder* duka = (tfb::DuckSoundDecoder*)This;

	duka->data = {};
	duka->frames = {};
	if (duka->duk)
	{
		delete duka->duk;
		duka->duk = nullptr;
	}
	duka->last_error = 0;
}

static int duka_Decode(tfb::SoundDecoder* This, void* buf, int32_t bufsize)
{
	tfb::DuckSoundDecoder* duka = (tfb::DuckSoundDecoder*)This;
	int32_t stuffed {};
	int32_t total = 0;

	if (bufsize <= 0)
	{
		return duka->last_error = *DukAudError::BadArg;
	}

	do
	{
		stuffed = duka_stuffBuffer(duka, buf, bufsize);
		buf = (uint8_t*)buf + stuffed;
		bufsize -= stuffed;
		total += stuffed;

		if (bufsize > 0 && duka->iframe < duka->cframes)
		{
			stuffed = duka_readNextFrame(duka);
			if (stuffed <= 0)
			{
				return stuffed;
			}
		}
	} while (bufsize > 0 && duka->iframe < duka->cframes);

	return total;
}

static uint32_t duka_Seek(tfb::SoundDecoder* This, uint32_t pcm_pos)
{
	tfb::DuckSoundDecoder* duka = (tfb::DuckSoundDecoder*)This;

	const uint32_t iframe {pcm_pos / duka->pcm_frame};
	if (iframe < duka->cframes)
	{
		duka->iframe = iframe;
		duka->cbdata = 0;
		duka->dataofs = 0;
		duka->predictors[0] = 0;
		duka->predictors[1] = 0;
	}
	return duka->iframe * duka->pcm_frame;
}

static uint32_t duka_GetFrame(tfb::SoundDecoder* This)
{
	const tfb::DuckSoundDecoder* duka = (tfb::DuckSoundDecoder*)This;

	// if there is nothing buffered return the actual current frame
	//  otherwise return previous
	return duka->dataofs == duka->cbdata ?
			   duka->iframe :
			   duka->iframe - 1;
}
