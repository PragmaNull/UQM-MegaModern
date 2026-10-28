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

#include "msgbox.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "core/string/Utf8.h"

namespace uqm
{


void displayMessageBox(uqgsl::czstring utf8Title, uqgsl::czstring utf8Message, MessageBoxType type)
{
	const UINT msgBoxType = [&type]() {
		switch (type)
		{
			case MessageBoxType::Info:
				return MB_ICONINFORMATION;
			case MessageBoxType::Warning:
				return MB_ICONWARNING;
			case MessageBoxType::Error:
				return MB_ICONERROR;
			default:
				break;
		}

		return MB_ICONEXCLAMATION;
	}();

	const auto wsTitle {toWideChar(utf8Title)};
	const auto wsMsg {toWideChar(utf8Message)};

	if (!wsTitle.empty() && !wsMsg.empty())
	{
		MessageBoxW(nullptr, wsMsg.c_str(), wsTitle.c_str(), msgBoxType);
		return;
	}

	// fallback to ascii
	MessageBoxA(nullptr, utf8Message, utf8Title, msgBoxType);
}

} // namespace uqm