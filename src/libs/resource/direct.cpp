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

#include "libs/reslib.h"

namespace uqm
{

uqstl::vector<uqstl::string> listFiles(const io::Directory& dir, const char* path, const io::Matcher& matcher)
{
	const uqstl::optional<io::Directory> subdir {dir.subdir(path)};
	if (!subdir)
	{
		return {};
	}
	uqstl::vector<uqstl::string> names {subdir->list("", matcher).value_or(uqstl::vector<uqstl::string> {})};
	uqstl::erase_if(names, [&subdir](const uqstl::string& name) {
		if (name.starts_with('.'))
		{
			return true;
		}
		const uqstl::optional<io::FileInfo> info {subdir->stat(name)};
		return !info || info->isDirectory();
	});
	return names;
}

} // namespace uqm
