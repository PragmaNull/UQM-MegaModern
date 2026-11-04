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

#include <stdlib.h>

#include "UQMGame.h"
#include <SDL3/SDL_main.h>

int main(int argc, char* argv[])
{
	uqm::UQMGame game {};

	if (const auto [setupResult, needExit] {game.setup({argv, static_cast<size_t>(argc)})}; setupResult != EXIT_SUCCESS || needExit)
	{
		return setupResult;
	}
	if (const int runResult {game.run()}; runResult != EXIT_SUCCESS)
	{
		return runResult;
	}
	game.teardown();

	return EXIT_SUCCESS;
}