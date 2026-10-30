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
#pragma once
#ifndef UQMGAME_H_
#define UQMGAME_H_

#include "core/stl/stl.h"
#include "options/options.h"
#include "core/log/logger.h"

#include <thread>

namespace uqm
{

class UQMGame
{
public:
	UQMGame();
	~UQMGame();

	[[nodiscard]] static UQMGame& getInstance();

	[[nodiscard]] uqstl::pair<int, bool> setup(uqstl::span<uqgsl::zstring> args);
	[[nodiscard]] int run();
	void teardown();

private:
	UQMOptions m_options {};
	Logger m_logger {};
	uqstl::jthread m_gameThread {};

	static UQMGame* s_instance;
};

} // namespace uqm

#endif /* UQMGAME_H_ */