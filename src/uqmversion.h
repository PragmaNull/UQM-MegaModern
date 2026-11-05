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

#ifndef UQMVERSION_H
#define UQMVERSION_H

inline constexpr auto UqmMajorVersion {0};
#define UQM_MAJOR_VERSION_S "0"
inline constexpr auto UqmMinorVersion {8};
#define UQM_MINOR_VERSION_S "8"
#ifdef RC_INVOKED
// Versions in resource files must be integers.
inline constexpr auto UqmPatchVersion {4};

#else
inline constexpr auto UqmPatchVersion {4};
#endif
#define UQM_PATCH_VERSION_S "4"
#if defined(DEBUG)
#define UQM_EXTRA_VERSION "MegaMod Debug"
#else
#define UQM_EXTRA_VERSION "MegaMod"
#endif
/* The final version is interpreted as:
 * printf ("{}.{}.{}{}", UqmMajorVersion, UqmMinorVersion,
 * 		UqmPatchVersion, UQM_EXTRA_VERSION);
 */


#endif
