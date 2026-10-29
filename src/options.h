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

/*
 * Eventually this should include all configuration stuff,
 * for now there's few options which indicate 3do/pc flavors.
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#include "core/stl/stl.h"
#include "port.h"
#include "libs/compiler.h"
#include "libs/io/FileSystem.h"
#include "uqm/setupmenu.h"

#include "options/OptionDefs.h"
#include "options/OptionTypes.h"
#include "options/options.h"

#include "uqmversion.h"


extern unsigned int resolutionFactor;
extern uqm::WindowStyle optWindowType;

// Runtime state (kept as globals)
extern bool optRequiresReload;
extern bool optRequiresRestart;
extern uint32_t oldRU;
extern uint32_t loadFuel;
extern int spaceMusicBySOI;
extern bool optSuperMelee;
extern bool optLoadGame;
extern bool optMaskOfDeceit;
extern bool restartGame;
extern bool optNoClassic;

namespace uqm
{
// What a device or upgrade cheat does when a game loads.
enum class CheatAction : uint8_t
{
	None,
	Remove,
	Add,
};
} // namespace uqm

#define NUM_UPGRADES 13

extern uqm::CheatAction optDeviceArray[28];
extern uqm::CheatAction optUpgradeArray[NUM_UPGRADES];

#define GAMMA_SCALE 1000

extern char baseContentPath[PATH_MAX];

extern uqgsl::czstring contentDirPath;
extern uqgsl::czstring addonDirPath;


extern uqstl::vector<uint32_t> g_addonList;

// addon names to check against
#define THREEDO_MUSIC "3domusic"
#define REMIX_MUSIC "remix"
#define VOL_RMX_MUSIC "volasaurus-remix-pack"
#define REGION_MUSIC "SpaceMusic"
#define HD_MODE "mm-hd"

#define DOS_MODE(a) ((a) ? "dos-mode-hd" : "dos-mode-sd")
#define THREEDO_MODE(a) ((a) ? "3do-mode-hd" : "3do-mode-sd")
#define WINDOW_MODE(a, b) ((b) == 0 ? DOS_MODE(a) : THREEDO_MODE(a))

#define BASE_CONTENT_NAME "mm-" UQM_MAJOR_VERSION_S "." UQM_MINOR_VERSION_S "." UQM_PATCH_VERSION_S "-content.uqm"

namespace uqm
{

/* These get edited by TEXTENTRY widgets, so they should have room to
 * hold as much as one of them allows by default. */
typedef struct _input_template
{
	char name[30];

	/* This should eventually also hold things like Joystick Port
	 * and whether or not joysticks are enabled at all, and
	 * possibly the whole configuration scheme.  If we do that, we
	 * can actually ditch much of VControl. */
} INPUT_TEMPLATE;

extern INPUT_TEMPLATE input_templates[6];

void prepareContentDir(uqgsl::czstring contentDirName, uqgsl::czstring addonDirName, uqgsl::czstring execFile);
// Returns the expanded config dir path, or an empty string if the path is not valid.
uqstl::string resolveConfigDir(uqstl::string_view configDirOption);
void prepareConfigDir(uqstl::string_view configDirOption);
void prepareMeleeDir(void);
void prepareSaveDir(void);
void prepareScrShotDir(void);
void prepareAddons(uqstl::span<const uqstl::string> addons);
void prepareShadowAddons(uqstl::span<const uqstl::string> addons);
// The file system for all game files and its directories. They exist between createGameFiles and releaseGameFiles,
// and each directory from its prepare function on.
void createGameFiles(void);
void releaseGameFiles(void);
io::FileSystem& gameFileSystem();
const io::Directory& contentDir();
const io::Directory& configDir();
const io::Directory& saveDir();
const io::Directory& meleeDir();
const io::Directory& scrShotDir();

bool loadAddon(uqgsl::czstring addon);
int loadIndices(const io::Directory& dir);
[[nodiscard]] bool isAddonAvailable(uqgsl::czstring addon_name);

bool setGammaCorrection(float gamma);
} // namespace uqm

#endif
