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

/* Portions (C) Serge van den Boom (svdb at stack.nl) */
/* Portions (C) Alex Volkov (codepro at usa.net) */

/* AIFF decoder (.aif)
 *
 * Doesn't work on *all* aiff files in general, only 8/16 PCM and
 * 16-bit AIFF-C SDX2-compressed.
 */

#include <stdio.h>
#include <stdlib.h>
// for abs()
#include <errno.h>
#ifndef _WIN32_WCE
#include <memory.h>
#endif
#include <cstdint>
#include <string.h>

#include "aiffaud.h"
#include "core/log/log.h"
#include "core/misc/bittricks.h"
#include "libs/reslib.h"
#include "port.h"

typedef uint32_t aiff_ID;

constexpr auto AiffMakeId(auto x1, auto x2, auto x3, auto x4) { return (((x1) << 24) | ((x2) << 16) | ((x3) << 8) | (x4)); }

static constexpr auto aiff_FormID {AiffMakeId('F', 'O', 'R', 'M')};
static constexpr auto aiff_FormVersionID {AiffMakeId('F', 'V', 'E', 'R')};
static constexpr auto aiff_CommonID {AiffMakeId('C', 'O', 'M', 'M')};
static constexpr auto aiff_SoundDataID {AiffMakeId('S', 'S', 'N', 'D')};

static constexpr auto aiff_FormTypeAIFF {AiffMakeId('A', 'I', 'F', 'F')};
static constexpr auto aiff_FormTypeAIFC {AiffMakeId('A', 'I', 'F', 'C')};

static constexpr auto aiff_CompressionTypeSDX2 {AiffMakeId('S', 'D', 'X', '2')};


struct aiff_ChunkHeader
{
	aiff_ID id;
	uint32_t size;
};

static constexpr auto AiffChunkHdrSize {4 + 4};

struct aiff_FileHeader
{
	aiff_ChunkHeader chunk;
	aiff_ID type;
};

struct aiff_FormatVersionChunk
{
	uint32_t version; /* format version, in Mac format */
};

struct aiff_CommonChunk
{
	uint16_t channels;	   /* number of channels */
	uint32_t sampleFrames; /* number of sample frames */
	uint16_t sampleSize;   /* number of bits per sample */
	int32_t sampleRate;	   /* number of frames per second */
						   /* this is actually stored as IEEE-754 80bit in files */
};

static constexpr auto AiffCommSize {2 + 4 + 2 + 10};

struct aiff_ExtCommonChunk
{
	uint16_t channels;				/* number of channels */
	uint32_t sampleFrames;			/* number of sample frames */
	uint16_t sampleSize;			/* number of bits per sample */
	int32_t sampleRate;				/* number of frames per second */
	aiff_ID extTypeID;				/* compression type ID */
	uqstl::array<char, 32> extName; /* compression type name */
};

static constexpr auto AiffExtCommSize {AiffCommSize + 4};

struct aiff_SoundDataChunk
{
	uint32_t offset;	/* offset to sound data */
	uint32_t blockSize; /* size of alignment blocks */
};

static constexpr auto AiffSsndSize {4 + 4};


const static char* aifa_GetName(void);
static bool aifa_InitModule(AudioFlags flags, const tfb::DecoderFormats*);
static void aifa_TermModule(void);
static tfb::SoundDecoder* aifa_Create(void);
static void aifa_Destroy(tfb::SoundDecoder* decoder);
static int aifa_GetError(tfb::SoundDecoder* This);
static bool aifa_Init(tfb::SoundDecoder* This);
static void aifa_Term(tfb::SoundDecoder* This);
static bool aifa_Open(tfb::SoundDecoder* This, const uqm::io::Directory& dir, const char* filename);
static void aifa_Close(tfb::SoundDecoder* This);
static int aifa_Decode(tfb::SoundDecoder* This, void* buf, int32_t bufsize);
static uint32_t aifa_Seek(tfb::SoundDecoder* This, uint32_t pcm_pos);
static uint32_t aifa_GetFrame(tfb::SoundDecoder* This);

tfb::SoundDecoderFuncs aifa_DecoderVtbl =
	{
		aifa_GetName,
		aifa_InitModule,
		aifa_TermModule,
		aifa_Create,
		aifa_Destroy,
		aifa_GetError,
		aifa_Init,
		aifa_Term,
		aifa_Open,
		aifa_Close,
		aifa_Decode,
		aifa_Seek,
		aifa_GetFrame,
};


enum class AiffCompression
{
	None,
	Sdx2,
};

constexpr int c_maxAiffChannels {4};

namespace tfb
{
struct AiffSoundDecoder
{
	// always the first member
	SoundDecoder decoder;

	// private
	int32_t last_error;
	uqgsl::owner<uqm::io::File*> fp;
	aiff_ExtCommonChunk fmtHdr;
	AiffCompression comp_type;
	unsigned bits_per_sample;
	unsigned block_align;
	unsigned file_block;
	uint32_t data_ofs;
	uint32_t data_size;
	uint32_t max_pcm;
	uint32_t cur_pcm;
	uqstl::array<int32_t, c_maxAiffChannels> prev_val;
};
} // namespace tfb

const static tfb::DecoderFormats* aifa_formats = nullptr;

static int aifa_DecodePCM(tfb::AiffSoundDecoder*, void* buf, int32_t bufsize);
static int aifa_DecodeSDX2(tfb::AiffSoundDecoder*, void* buf, int32_t bufsize);


const static char* aifa_GetName(void)
{
	return "AIFF";
}

static bool aifa_InitModule(AudioFlags flags, const tfb::DecoderFormats* fmts)
{
	aifa_formats = fmts;
	return true;

	uqstl::ignore = flags; // laugh at compiler warning
}

static void aifa_TermModule(void)
{
	// no specific module term
}

static tfb::SoundDecoder* aifa_Create(void)
{
	return &(new tfb::AiffSoundDecoder {})->decoder;
}

static void aifa_Destroy(tfb::SoundDecoder* decoder)
{
	delete reinterpret_cast<tfb::AiffSoundDecoder*>(decoder);
}

static int aifa_GetError(tfb::SoundDecoder* This)
{
	tfb::AiffSoundDecoder* aifa = (tfb::AiffSoundDecoder*)This;
	const int ret = aifa->last_error;
	aifa->last_error = 0;
	return ret;
}

static bool aifa_Init(tfb::SoundDecoder* This)
{
	//tfb::AiffSoundDecoder* aifa = (tfb::AiffSoundDecoder*) This;
	This->need_swap = !aifa_formats->want_big_endian;
	return true;
}

static void aifa_Term(tfb::SoundDecoder* This)
{
	//tfb::AiffSoundDecoder* aifa = (tfb::AiffSoundDecoder*) This;
	aifa_Close(This); // ensure cleanup
}

static bool read_be_16(uqm::io::File* fp, uint16_t* v)
{
	if (!fp->readValue(*v))
	{
		return false;
	}
	*v = uqm::convertBigEndian<uint16_t>(*v);
	return true;
}

static bool read_be_32(uqm::io::File* fp, uint32_t* v)
{
	if (!fp->readValue(*v))
	{
		return false;
	}
	*v = uqm::convertBigEndian<uint32_t>(*v);
	return true;
}

// Read 80-bit IEEE 754 floating point number.
// We are only interested in values that we can work with,
//   so using an int32_t here is fine.
static bool read_be_f80(uqm::io::File* fp, int32_t* v)
{
	int sign {};
	int exp {};
	int shift {};
	uint16_t se {};
	uint32_t mant {};
	uint32_t mant_low {};
	if (!read_be_16(fp, &se) || !read_be_32(fp, &mant) || !read_be_32(fp, &mant_low))
	{
		return false;
	}

	sign = (se >> 15) & 1;		  // sign is the highest bit
	exp = (se & ((1 << 15) - 1)); // exponent is next highest 15 bits
#if 0							  // XXX: 80bit IEEE 754 used in AIFF uses explicit mantissa MS bit
	// mantissa has an implied leading bit which is typically 1
	mant >>= 1;
	if (exp != 0)
		mant |= 0x80000000;
#endif
	mant >>= 1;			  // we also need space for sign
	exp -= (1 << 14) - 1; // exponent is biased by (2^(e-1) - 1)
	shift = exp - 31 + 1; // mantissa is already 31 bits before decimal pt.
	if (shift > 0)
	{
		mant = 0x7fffffff; // already too big
	}
	else if (shift < 0)
	{
		mant >>= -shift;
	}

	*v = sign ? -(int32_t)mant : (int32_t)mant;

	return true;
}

static bool aifa_readFileHeader(tfb::AiffSoundDecoder* aifa, aiff_FileHeader* hdr)
{
	if (!read_be_32(aifa->fp, &hdr->chunk.id) || !read_be_32(aifa->fp, &hdr->chunk.size) || !read_be_32(aifa->fp, &hdr->type))
	{
		aifa->last_error = errno;
		return false;
	}
	return true;
}

static bool aifa_readChunkHeader(tfb::AiffSoundDecoder* aifa, aiff_ChunkHeader* hdr)
{
	if (!read_be_32(aifa->fp, &hdr->id) || !read_be_32(aifa->fp, &hdr->size))
	{
		aifa->last_error = errno;
		return false;
	}
	return true;
}

static int aifa_readCommonChunk(tfb::AiffSoundDecoder* aifa, uint32_t size,
								aiff_ExtCommonChunk* fmt)
{
	memset(fmt, 0, sizeof(*fmt));
	if (size < AiffCommSize)
	{
		aifa->last_error = *AiffError::BadFile;
		return 0;
	}

	if (!read_be_16(aifa->fp, &fmt->channels) || !read_be_32(aifa->fp, &fmt->sampleFrames) || !read_be_16(aifa->fp, &fmt->sampleSize) || !read_be_f80(aifa->fp, &fmt->sampleRate))
	{
		aifa->last_error = errno;
		return 0;
	}
	int bytes {AiffCommSize};

	if (size >= AiffExtCommSize)
	{
		if (!read_be_32(aifa->fp, &fmt->extTypeID))
		{
			aifa->last_error = errno;
			return 0;
		}
		bytes += sizeof(fmt->extTypeID);
	}

	return bytes;
}

static bool aifa_readSoundDataChunk(tfb::AiffSoundDecoder* aifa,
									aiff_SoundDataChunk* data)
{
	if (!read_be_32(aifa->fp, &data->offset) || !read_be_32(aifa->fp, &data->blockSize))
	{
		aifa->last_error = errno;
		return false;
	}
	return true;
}

static bool aifa_Open(tfb::SoundDecoder* This, const uqm::io::Directory& dir, const char* filename)
{
	tfb::AiffSoundDecoder* aifa = (tfb::AiffSoundDecoder*)This;
	aiff_FileHeader fileHdr;
	aiff_ChunkHeader chunkHdr;
	int32_t remSize {};

	aifa->fp = res_NewResFile(dir, filename);
	if (!aifa->fp)
	{
		aifa->last_error = errno;
		return false;
	}

	aifa->data_size = 0;
	aifa->max_pcm = 0;
	aifa->data_ofs = 0;
	memset(&aifa->fmtHdr, 0, sizeof(aifa->fmtHdr));
	memset(aifa->prev_val.data(), 0, sizeof(aifa->prev_val));

	// read wave header
	if (!aifa_readFileHeader(aifa, &fileHdr))
	{
		aifa->last_error = errno;
		aifa_Close(This);
		return false;
	}
	if (fileHdr.chunk.id != aiff_FormID)
	{
		uqm::log::warn("aifa_Open(): not an aiff file, ID 0x%08x",
					   fileHdr.chunk.id);
		aifa_Close(This);
		return false;
	}
	if (fileHdr.type != aiff_FormTypeAIFF && fileHdr.type != aiff_FormTypeAIFC)
	{
		uqm::log::warn("aifa_Open(): unsupported aiff file"
					   ", Type 0x%08x",
					   fileHdr.type);
		aifa_Close(This);
		return false;
	}

	for (remSize = fileHdr.chunk.size - sizeof(aiff_ID); remSize > 0;
		 remSize -= ((chunkHdr.size + 1) & ~1) + AiffChunkHdrSize)
	{
		if (!aifa_readChunkHeader(aifa, &chunkHdr))
		{
			aifa_Close(This);
			return false;
		}

		if (chunkHdr.id == aiff_CommonID)
		{
			const int read = aifa_readCommonChunk(aifa, chunkHdr.size, &aifa->fmtHdr);
			if (!read)
			{
				aifa_Close(This);
				return false;
			}
			aifa->fp->seek(chunkHdr.size - read, uqm::io::SeekFrom::Current);
		}
		else if (chunkHdr.id == aiff_SoundDataID)
		{
			aiff_SoundDataChunk data;
			if (!aifa_readSoundDataChunk(aifa, &data))
			{
				aifa_Close(This);
				return false;
			}
			aifa->data_ofs = aifa->fp->tell() + data.offset;
			aifa->fp->seek(chunkHdr.size - AiffSsndSize, uqm::io::SeekFrom::Current);
		}
		else
		{ // skip uninteresting chunk
			aifa->fp->seek(chunkHdr.size, uqm::io::SeekFrom::Current);
		}

		// 2-align the file ptr
		aifa->fp->seek(chunkHdr.size & 1, uqm::io::SeekFrom::Current);
	}

	if (aifa->fmtHdr.sampleFrames == 0)
	{
		uqm::log::warn("aifa_Open(): aiff file has no sound data");
		aifa_Close(This);
		return false;
	}

	// make bits-per-sample a multiple of 8
	aifa->bits_per_sample = (aifa->fmtHdr.sampleSize + 7) & ~7;
	if (aifa->bits_per_sample == 0 || aifa->bits_per_sample > 16)
	{ // XXX: for now we do not support 24 and 32 bps
		uqm::log::warn("aifa_Open(): unsupported sample size {}",
					   aifa->bits_per_sample);
		aifa_Close(This);
		return false;
	}
	if (aifa->fmtHdr.channels != 1 && aifa->fmtHdr.channels != 2)
	{
		uqm::log::warn("aifa_Open(): unsupported number of channels {}",
					   (unsigned)aifa->fmtHdr.channels);
		aifa_Close(This);
		return false;
	}
	if (aifa->fmtHdr.sampleRate < 300 || aifa->fmtHdr.sampleRate > 128000)
	{
		uqm::log::warn("aifa_Open(): unsupported sampling rate %ld",
					   (long)aifa->fmtHdr.sampleRate);
		aifa_Close(This);
		return false;
	}

	aifa->block_align = aifa->bits_per_sample / 8 * aifa->fmtHdr.channels;
	aifa->file_block = aifa->block_align;
	if (!aifa->data_ofs)
	{
		uqm::log::warn("aifa_Open(): bad aiff file,"
					   " no SSND chunk found");
		aifa_Close(This);
		return false;
	}

	if (fileHdr.type == aiff_FormTypeAIFF)
	{
		if (aifa->fmtHdr.extTypeID != 0)
		{
			uqm::log::warn("aifa_Open(): unsupported extension 0x%08x",
						   aifa->fmtHdr.extTypeID);
			aifa_Close(This);
			return false;
		}
		aifa->comp_type = AiffCompression::None;
	}
	else if (fileHdr.type == aiff_FormTypeAIFC)
	{
		if (aifa->fmtHdr.extTypeID != aiff_CompressionTypeSDX2)
		{
			uqm::log::warn("aifa_Open(): unsupported compression 0x%08x",
						   aifa->fmtHdr.extTypeID);
			aifa_Close(This);
			return false;
		}
		aifa->comp_type = AiffCompression::Sdx2;
		aifa->file_block /= 2;
		assert(aifa->fmtHdr.channels <= c_maxAiffChannels);
		// after decompression, we will get samples in machine byte order
		This->need_swap = (aifa_formats->big_endian
						   != aifa_formats->want_big_endian);
	}

	aifa->data_size = aifa->fmtHdr.sampleFrames * aifa->file_block;

	if (aifa->comp_type == AiffCompression::Sdx2 && aifa->bits_per_sample != 16)
	{
		uqm::log::warn("aifa_Open(): unsupported sample size {} for SDX2",
					   (unsigned)aifa->fmtHdr.sampleSize);
		aifa_Close(This);
		return false;
	}

	This->format = (aifa->fmtHdr.channels == 1 ?
						(aifa->bits_per_sample == 8 ?
							 aifa_formats->mono8 :
							 aifa_formats->mono16) :
						(aifa->bits_per_sample == 8 ?
							 aifa_formats->stereo8 :
							 aifa_formats->stereo16));
	This->frequency = aifa->fmtHdr.sampleRate;

	aifa->fp->seek(aifa->data_ofs, uqm::io::SeekFrom::Start);
	aifa->max_pcm = aifa->fmtHdr.sampleFrames;
	aifa->cur_pcm = 0;
	This->length = (float)aifa->max_pcm / aifa->fmtHdr.sampleRate;

	This->filename_hash = crc32b(filename);

	aifa->last_error = 0;

	return true;
}

static void aifa_Close(tfb::SoundDecoder* This)
{
	tfb::AiffSoundDecoder* aifa = (tfb::AiffSoundDecoder*)This;

	if (aifa->fp)
	{
		delete aifa->fp;
		aifa->fp = nullptr;
	}
}

static int aifa_Decode(tfb::SoundDecoder* This, void* buf, int32_t bufsize)
{
	tfb::AiffSoundDecoder* aifa = (tfb::AiffSoundDecoder*)This;
	switch (aifa->comp_type)
	{
		case AiffCompression::None:
			return aifa_DecodePCM(aifa, buf, bufsize);
		case AiffCompression::Sdx2:
			return aifa_DecodeSDX2(aifa, buf, bufsize);
		default:
			assert(false && "Unknown comp_type");
			return 0;
	}
}

static int aifa_DecodePCM(tfb::AiffSoundDecoder* aifa, void* buf, int32_t bufsize)
{
	uint32_t dec_pcm {bufsize / aifa->block_align};
	if (dec_pcm > aifa->max_pcm - aifa->cur_pcm)
	{
		dec_pcm = aifa->max_pcm - aifa->cur_pcm;
	}

	dec_pcm = static_cast<decltype(dec_pcm)>(aifa->fp->read(uqm::io::asBytes(buf, aifa->file_block * dec_pcm)) / aifa->file_block);
	aifa->cur_pcm += dec_pcm;
	const uint32_t size {dec_pcm * aifa->block_align};

	if (aifa->bits_per_sample == 8)
	{ // AIFF files store 8-bit data as signed
		// and we need it unsigned
		uint8_t* ptr = (uint8_t*)buf;
		for (uint32_t left {size}; left > 0; --left, ++ptr)
		{
			*ptr += 128;
		}
	}

	return size;
}

static int aifa_DecodeSDX2(tfb::AiffSoundDecoder* aifa, void* buf, int32_t bufsize)
{
	int16_t* dst = (int16_t*)buf;

	uint32_t dec_pcm {bufsize / aifa->block_align};
	if (dec_pcm > aifa->max_pcm - aifa->cur_pcm)
	{
		dec_pcm = aifa->max_pcm - aifa->cur_pcm;
	}

	int8_t* src {(int8_t*)buf + bufsize - (dec_pcm * aifa->file_block)};
	dec_pcm = static_cast<decltype(dec_pcm)>(aifa->fp->read(uqm::io::asBytes(src, aifa->file_block * dec_pcm)) / aifa->file_block);
	aifa->cur_pcm += dec_pcm;

	for (uint32_t left {dec_pcm}; left > 0; --left)
	{
		int32_t* prev = aifa->prev_val.data();
		for (int i {aifa->fmtHdr.channels}; i > 0; --i, ++prev, ++src, ++dst)
		{
			int32_t v = (*src * abs(*src)) << 1;
			if (*src & 1)
			{
				v += *prev;
			}
			// saturate the value
			if (v > 32767)
			{
				v = 32767;
			}
			else if (v < -32768)
			{
				v = -32768;
			}
			*prev = v;
			*dst = v;
		}
	}

	return dec_pcm * aifa->block_align;
}

static uint32_t aifa_Seek(tfb::SoundDecoder* This, uint32_t pcm_pos)
{
	tfb::AiffSoundDecoder* aifa = (tfb::AiffSoundDecoder*)This;

	if (pcm_pos > aifa->max_pcm)
	{
		pcm_pos = aifa->max_pcm;
	}
	aifa->cur_pcm = pcm_pos;
	aifa->fp->seek(
		aifa->data_ofs + pcm_pos * aifa->file_block, uqm::io::SeekFrom::Start);

	// reset previous values for SDX2 on seek ops
	// the delta will recover faster with reset
	memset(aifa->prev_val.data(), 0, sizeof(aifa->prev_val));

	return pcm_pos;
}

static uint32_t aifa_GetFrame(tfb::SoundDecoder* This)
{
	//tfb::AiffSoundDecoder* aifa = (tfb::AiffSoundDecoder*) This;
	return 0; // only 1 frame for now

	(void)This; // laugh at compiler warning
}
