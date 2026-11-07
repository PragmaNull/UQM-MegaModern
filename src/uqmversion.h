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
#ifndef UQM_UQMVERSION_H
#define UQM_UQMVERSION_H

// The resource compiler reads these macros, so they cannot be constexpr.
#define UQM_MAJOR_VERSION 0
#define UQM_MAJOR_VERSION_S "0"
#define UQM_MINOR_VERSION 8
#define UQM_MINOR_VERSION_S "8"
#define UQM_PATCH_VERSION 4
#define UQM_PATCH_VERSION_S "4"
#define UQM_REVISION_VERSION 0
#if defined(DEBUG)
#define UQM_EXTRA_VERSION "MegaMod Debug"
#else
#define UQM_EXTRA_VERSION "MegaMod"
#endif
#define UQM_STRING_VERSION UQM_MAJOR_VERSION_S "." UQM_MINOR_VERSION_S "." UQM_PATCH_VERSION_S " " UQM_EXTRA_VERSION

#ifndef RC_INVOKED
inline constexpr auto UqmMajorVersion {UQM_MAJOR_VERSION};
inline constexpr auto UqmMinorVersion {UQM_MINOR_VERSION};
inline constexpr auto UqmPatchVersion {UQM_PATCH_VERSION};
inline constexpr const char* UqmExtraVersion {UQM_EXTRA_VERSION};
inline constexpr const char* UqmExtraVersionHd {"HD " UQM_EXTRA_VERSION};
inline constexpr const char* UqmStringVersion {UQM_STRING_VERSION};
#endif

#endif // UQM_UQMVERSION_H
