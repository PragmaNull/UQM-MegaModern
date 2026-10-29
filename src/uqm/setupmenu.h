// Copyright Michael Martin, 2004.

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

#ifndef UQM_SETUPMENU_H_
#define UQM_SETUPMENU_H_

#include "options/OptionTypes.h"
#include "controls.h"

typedef enum
{
	OPTVAL_PC_WINDOW,
	OPTVAL_3DO_WINDOW,
	OPTVAL_UQM_WINDOW
} OPT_WINDOWTYPE;

typedef enum
{
	OPTVAL_DO_NOTHING,
	OPTVAL_REM_THING,
	OPTVAL_ADD_THING
} OPT_ADD_REMOVE;

#define NUM_UPGRADES 13

void SetupMenu(void);

#endif // UQM_SETUPMENU_H_
