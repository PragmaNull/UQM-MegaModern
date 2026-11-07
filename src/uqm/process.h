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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once
#ifndef UQM_PROCESS_H
#define UQM_PROCESS_H

#include "core/misc/bittricks.h"
#include "element.h"
#include "libs/gfxlib.h"

namespace uqm
{
extern void RedrawQueue(bool clear);
extern void InitDisplayList(void);
extern void SetUpElement(Element* ElementPtr);
extern void InsertPrim(PrimLinks* pLinks, uint16_t primIndex, uint16_t iPI);

} // namespace uqm

#endif // UQM_PROCESS_H
