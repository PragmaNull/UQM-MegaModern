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

#ifndef LIBS_DECOMP_LZH_H_
#define LIBS_DECOMP_LZH_H_

#include "libs/declib.h"
#include "libs/memlib.h"

/* LZSS Parameters */

static inline constexpr size_t LZSS_BufferSize {4096};		  /* Size of string buffer (was N)*/
static inline constexpr size_t LZSS_LookAheadBufferSize {16}; /*size of look - ahead buffer (was F) */
//#define F 60 /* Size of look-ahead buffer */
static inline constexpr size_t LZSS_Threshold {2};		   // (Was THRESHOLD)
static inline constexpr size_t LZSS_NIL {LZSS_BufferSize}; /* End of tree's node (was NIL)  */

/* Huffman coding parameters */

static inline constexpr size_t HUF_NumChars {256 - LZSS_Threshold + LZSS_LookAheadBufferSize}; // was N_CHAR
/* character code (= 0..HUF_NumChars-1) */
static inline constexpr size_t HUF_TableSize {HUF_NumChars * 2 - 1}; /* Size of table (Was T)*/
static inline constexpr size_t HUF_Root {HUF_TableSize - 1};		 /* root position (Was R)*/
static inline constexpr size_t HUF_MaxFrequency {0x8000};			 /* update when cumulative frequency (Was MAX_REQ)*/


struct _LZHCODE_DESC
{
	uint16_t buf_index, restart_index, bytes_left;
	uint8_t text_buf[LZSS_BufferSize + LZSS_LookAheadBufferSize - 1];
	/* reconstruct freq tree */
	uint16_t freq[HUF_TableSize + 1]; /* cumulative freq table */
									  /*
		 * pointing parent nodes.
		 * area [HUF_TableSize..(HUF_TableSize + HUF_NumChars - 1)] are pointers for leaves
		 */
	uint16_t prnt[HUF_TableSize + HUF_NumChars];
	/* pointing children nodes (son[], son[] + 1)*/
	uint16_t son[HUF_TableSize];
	uint16_t workbuf;
	uint8_t workbuflen;

	STREAM_TYPE StreamType;

	void* Stream;
	uint32_t StreamIndex, StreamLength;

	STREAM_MODE StreamMode;
	PVOIDFUNC CleanupFunc;
};

typedef struct _LZHCODE_DESC LZHCODE_DESC;
typedef LZHCODE_DESC* PLZHCODE_DESC;

#define InChar() (_StreamType == FILE_STREAM ?                                     \
					  GetResFileChar(*reinterpret_cast<uqm::io::File*>(_Stream)) : \
					  (int)*_Stream++)
#define OutChar(c) (_StreamType == FILE_STREAM ?                                          \
						PutResFileChar((c), *reinterpret_cast<uqm::io::File*>(_Stream)) : \
						(*_Stream++ = (uint8_t)(c)))


#define AllocCodeDesc() HCalloc(sizeof(LZHCODE_DESC))
#define FreeCodeDesc HFree

extern void _update(uint16_t c);
extern void StartHuff(void);

extern PLZHCODE_DESC _lpCurCodeDesc;
extern STREAM_TYPE _StreamType;
extern uint8_t* _Stream;
extern uint16_t _workbuf;
extern uint8_t _workbuflen;

#endif /* LIBS_DECOMP_LZH_H_ */
