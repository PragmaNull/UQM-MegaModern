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

/* MikMod decoder (.mod adapter)
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "port.h"
#include <cstdint>
#include "endian_uqm.h"
#include "decoder.h"
#include "core/misc/bittricks.h"
#include "libs/memlib.h"
#include "libs/reslib.h"
#include "libs/sound/audiocore.h"
#include "core/log/log.h"
#include "modaud.h"

#include <mikmod.h>

#define THIS_PTR TFB_SoundDecoder* This

static const char* moda_GetName(void);
static bool moda_InitModule(AudioFlags flags, const TFB_DecoderFormats*);
static void moda_TermModule(void);
static TFB_SoundDecoder* moda_Create(void);
static void moda_Destroy(TFB_SoundDecoder* decoder);
static int moda_GetError(THIS_PTR);
static bool moda_Init(THIS_PTR);
static void moda_Term(THIS_PTR);
static bool moda_Open(THIS_PTR, const uqm::io::Directory& dir, const char* filename);
static void moda_Close(THIS_PTR);
static int moda_Decode(THIS_PTR, void* buf, int32_t bufsize);
static uint32_t moda_Seek(THIS_PTR, uint32_t pcm_pos);
static uint32_t moda_GetFrame(THIS_PTR);

TFB_SoundDecoderFuncs moda_DecoderVtbl =
	{
		moda_GetName,
		moda_InitModule,
		moda_TermModule,
		moda_Create,
		moda_Destroy,
		moda_GetError,
		moda_Init,
		moda_Term,
		moda_Open,
		moda_Close,
		moda_Decode,
		moda_Seek,
		moda_GetFrame,
};

typedef struct tfb_modsounddecoder
{
	// always the first member
	TFB_SoundDecoder decoder;

	// private
	int32_t last_error;
	MODULE* module;

} TFB_ModSoundDecoder;


// MikMod Output driver
//  we provide our own so that we can use MikMod as
//  generic decoder

static void* buffer;
static ULONG bufsize;
static ULONG written;

static ULONG*
moda_mmout_SetOutputBuffer(void* buf, ULONG size)
{
	buffer = buf;
	bufsize = size;
	written = 0;
	return &written;
}

static BOOL
moda_mmout_IsThere(void)
{
	return 1;
}

static BOOL
moda_mmout_Init(void)
{
	md_mode |= DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;
	return VC_Init();
}

static void
moda_mmout_Exit(void)
{
	VC_Exit();
}

static void
moda_mmout_Update(void)
{
	written = 0;
	if (!buffer || bufsize == 0)
	{
		return;
	}

	written = VC_WriteBytes((int8_t*)buffer, bufsize);
}

static BOOL
moda_mmout_Reset(void)
{
	return 0;
}

static char MDRIVER_name[] = "Mem Buffer";
static char MDRIVER_version[] = "Mem Buffer driver v1.1";
static char MDRIVER_alias[] = "membuf";

static MDRIVER moda_mmout_drv =
	{
		nullptr,
		//xxx libmikmod does not declare these fields const; it probably should.
		MDRIVER_name,	 // Name
		MDRIVER_version, // Version
		0, 255,			 // Voice limits
		MDRIVER_alias,	 // Alias

// The minimum mikmod version we support is 3.1.8
#if (LIBMIKMOD_VERSION_MAJOR > 3) || ((LIBMIKMOD_VERSION_MAJOR == 3) && (LIBMIKMOD_VERSION_MINOR >= 2))
		nullptr, // Cmdline help
#endif

		nullptr,
		moda_mmout_IsThere,
		VC_SampleLoad,
		VC_SampleUnload,
		VC_SampleSpace,
		VC_SampleLength,
		moda_mmout_Init,
		moda_mmout_Exit,
		moda_mmout_Reset,
		VC_SetNumVoices,
		VC_PlayStart,
		VC_PlayStop,
		moda_mmout_Update,
		nullptr, /* FIXME: Pause */
		VC_VoiceSetVolume,
		VC_VoiceGetVolume,
		VC_VoiceSetFrequency,
		VC_VoiceGetFrequency,
		VC_VoiceSetPanning,
		VC_VoiceGetPanning,
		VC_VoicePlay,
		VC_VoiceStop,
		VC_VoiceStopped,
		VC_VoiceGetPosition,
		VC_VoiceRealVolume};


static const TFB_DecoderFormats* moda_formats = nullptr;

// MikMod READER interface
//  we provide our own so that we can do loading via libs/io
//
typedef struct MIOREADER
{
	MREADER core;
	uqm::io::File* file;

} MIOREADER;

static BOOL
moda_ioReader_Eof(MREADER* reader)
{
	return ((MIOREADER*)reader)->file->eof();
}

static BOOL
moda_ioReader_Read(MREADER* reader, void* ptr, size_t size)
{
	return ReadResFile(ptr, size, 1, *((MIOREADER*)reader)->file);
}

static int
moda_ioReader_Get(MREADER* reader)
{
	return ((MIOREADER*)reader)->file->getChar();
}

static BOOL
moda_ioReader_Seek(MREADER* reader, long offset, int whence)
{
	return SeekResFile(*((MIOREADER*)reader)->file, offset, whence);
}

static long
moda_ioReader_Tell(MREADER* reader)
{
	return TellResFile(*((MIOREADER*)reader)->file);
}

static MREADER*
moda_new_ioReader(uqm::io::File* fp)
{
	MIOREADER* reader = (MIOREADER*)HMalloc(sizeof(MIOREADER));
	if (reader)
	{
		reader->core.Eof = &moda_ioReader_Eof;
		reader->core.Read = &moda_ioReader_Read;
		reader->core.Get = &moda_ioReader_Get;
		reader->core.Seek = &moda_ioReader_Seek;
		reader->core.Tell = &moda_ioReader_Tell;
		reader->core.iobase = 0;
		reader->core.prev_iobase = 0;
		reader->file = fp;
	}
	return (MREADER*)reader;
}

static void
moda_delete_ioReader(MREADER* reader)
{
	if (reader)
	{
		HFree(reader);
	}
}


static const char*
moda_GetName(void)
{
	return "MikMod";
}

static bool
moda_InitModule(AudioFlags flags, const TFB_DecoderFormats* fmts)
{
	MikMod_RegisterDriver(&moda_mmout_drv);
	MikMod_RegisterAllLoaders();

	if (testFlag(flags, AudioFlags::QualityHigh))
	{
		md_mode = DMODE_HQMIXER | DMODE_STEREO | DMODE_16BITS | DMODE_INTERP | DMODE_SURROUND;
		md_mixfreq = 44100;
		md_reverb = 1;
	}
	else if (testFlag(flags, AudioFlags::QualityLow))
	{
		md_mode = DMODE_SOFT_MUSIC | DMODE_STEREO | DMODE_16BITS;
#ifdef __SYMBIAN32__
		md_mixfreq = 11025;
#else
		md_mixfreq = 22050;
#endif
		md_reverb = 0;
	}
	else
	{
		md_mode = DMODE_SOFT_MUSIC | DMODE_STEREO | DMODE_16BITS | DMODE_INTERP;
		md_mixfreq = 44100;
		md_reverb = 0;
	}

	md_pansep = 64;

	if (MikMod_Init(nullptr))
	{
		uqm::log::error("MikMod_Init() failed, {}",
						MikMod_strerror(MikMod_errno));
		return false;
	}

	moda_formats = fmts;

	return true;
}

static void
moda_TermModule(void)
{
	MikMod_Exit();
}

static TFB_SoundDecoder*
moda_Create(void)
{
	return &(new TFB_ModSoundDecoder {})->decoder;
}

static void
moda_Destroy(TFB_SoundDecoder* decoder)
{
	delete reinterpret_cast<TFB_ModSoundDecoder*>(decoder);
}

static int
moda_GetError(THIS_PTR)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*)This;
	int ret = moda->last_error;
	moda->last_error = 0;
	return ret;
}

static bool
moda_Init(THIS_PTR)
{
	//TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*) This;
	This->need_swap =
		moda_formats->big_endian != moda_formats->want_big_endian;
	return true;
}

static void
moda_Term(THIS_PTR)
{
	//TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*) This;
	moda_Close(This); // ensure cleanup
}

static bool
moda_Open(THIS_PTR, const uqm::io::Directory& dir, const char* filename)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*)This;
	uqgsl::owner<uqm::io::File*> fp {};
	MREADER* reader;
	MODULE* mod;

	fp = res_NewResFile(dir, filename);
	if (!fp)
	{
		moda->last_error = errno;
		return false;
	}

	reader = moda_new_ioReader(fp);
	if (!reader)
	{
		moda->last_error = -1;
		delete fp;
		return false;
	}

	mod = Player_LoadGeneric(reader, 8, 0);

	// can already dispose of reader and fileh
	moda_delete_ioReader(reader);
	delete fp;
	if (!mod)
	{
		uqm::log::warn("moda_Open(): could not load {}", filename);
		return false;
	}

	moda->module = mod;
	mod->extspd = 1;
	mod->panflag = 1;
	mod->wrap = 0;
	mod->loop = 1;

	This->format = moda_formats->stereo16;
	This->frequency = md_mixfreq;
	This->length = 0; // FIXME way to obtain this from mikmod?
	This->numpos = mod->numpos;
	This->filename_hash = crc32b(filename);

	moda->last_error = 0;

	return true;
}

static void
moda_Close(THIS_PTR)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*)This;

	if (moda->module)
	{
		Player_Free(moda->module);
		moda->module = nullptr;
	}
}

static int
moda_Decode(THIS_PTR, void* buf, int32_t bufsize)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*)This;
	volatile ULONG* poutsize;

	Player_Start(moda->module);
	if (!Player_Active())
	{
		return 0;
	}

	poutsize = moda_mmout_SetOutputBuffer(buf, bufsize);
	MikMod_Update();

	return *poutsize;
}

static uint32_t
moda_Seek(THIS_PTR, uint32_t pcm_pos)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*)This;

	Player_Start(moda->module);
	Player_SetPosition(pcm_pos);

	return 0;
}

static uint32_t
moda_GetFrame(THIS_PTR)
{
	TFB_ModSoundDecoder* moda = (TFB_ModSoundDecoder*)This;
	return moda->module->sngpos;
}
