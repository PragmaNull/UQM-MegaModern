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

#ifndef LIBS_INPLIB_H_
#define LIBS_INPLIB_H_

#include <stddef.h>
#include "libs/compiler.h"
#include "libs/io/FileSystem.h"
#include "libs/unicode.h"
#include <atomic>

#if 0 //defined(__cplusplus)
extern "C" {
#endif


extern bool AnyButtonPress(bool DetectSpecial);
extern bool DirKeysPress(void);
extern bool ActKeysPress(void);

extern void TFB_ResetControls(void);

/*
 * Not used right now
extern bool FindMouse (void);
extern void MoveMouse (int16_t x, int16_t y);
extern uint8_t LocateMouse (int16_t *px, int16_t *py);
*/

extern uqstl::atomic<int> MouseButtonDown;
extern uqstl::atomic<int> QuitPosted;
extern uqstl::atomic<int> GameActive;

/* Functions for dealing with Character Mode */

void EnterCharacterMode(void);
void ExitCharacterMode(void);
UniChar GetNextCharacter(void);
UniChar GetLastCharacter(void);

/* Interrogating the current key configuration */

void InterrogateInputState(int templat, int control, int index, char* buffer, int maxlen);
void RemoveInputState(int templat, int control, int index);
void RebindInputState(int templat, int control, int index);

void SaveKeyConfiguration(const uqm::io::Directory& path);

/* Separate inputs into frames for dealing with very fast inputs */

void BeginInputFrame(void);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* LIBS_INPLIB_H_ */
