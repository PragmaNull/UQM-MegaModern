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

#include "core/platform/platform.h"
#include "core/string/StringUtils.h"

#include "options.h"

#include "port.h"
#include "libs/graphics/gfx_common.h"
#include "libs/file/paths.h"
#include "config.h"
#include "core/misc/bittricks.h"
#include "libs/strlib.h"
#include "core/log/log.h"
#include "libs/reslib.h"
#include "libs/memlib.h"
#include "uqm/starmap.h"
#include "uqm/planets/scan.h"
#include <cstdint>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#ifdef __APPLE__
#include <libgen.h>
/* for dirname() */
#endif


unsigned int resolutionFactor;
uqm::WindowStyle optWindowType {uqm::WindowStyle::UQM};

// Runtime state (kept as globals)
bool optRequiresReload;
bool optRequiresRestart;
uint32_t oldRU;
uint32_t loadFuel;
int spaceMusicBySOI;
bool optSuperMelee;
bool optLoadGame;
uqm::SeedType optSeedType;
bool optMaskOfDeceit;
bool optNoClassic;
uqm::CheatAction optDeviceArray[28];
uqm::CheatAction optUpgradeArray[NUM_UPGRADES];
namespace
{
// The game file system. The members are destroyed in reverse order: the directories, the mounts, then the file system.
struct GameFiles
{
	uqm::io::FileSystem fileSystem {};
	uqstl::optional<uqm::io::Mount> configMount {};
	uqstl::optional<uqm::io::Mount> contentMount {};
	uqstl::optional<uqm::io::Mount> addonsMount {};
	uqstl::vector<uqm::io::Mount> mounts {};
	uqstl::optional<uqm::io::Directory> contentDir {};
	uqstl::optional<uqm::io::Directory> configDir {};
	uqstl::optional<uqm::io::Directory> saveDir {};
	uqstl::optional<uqm::io::Directory> meleeDir {};
	uqstl::optional<uqm::io::Directory> scrShotDir {};
};

uqstl::optional<GameFiles> s_gameFiles {};

const uqm::io::Directory& gameDirectory(const uqstl::optional<uqm::io::Directory>& dir)
{
	assert(dir.has_value());
	return *dir;
}

const uqm::io::Matcher& packageMatcher()
{
	static const uqm::io::Matcher matcher {*uqm::io::Matcher::create(uqm::io::Matcher::Kind::Regex, "\\.([zZ][iI][pP]|[uU][qQ][mM])$")};
	return matcher;
}
} // namespace

uqgsl::czstring contentDirPath;
uqgsl::czstring addonDirPath;

char baseContentPath[PATH_MAX];

// addon availability
uqstl::vector<uint32_t> g_addonList {};


namespace uqm
{
INPUT_TEMPLATE input_templates[6];

uqm::io::FileSystem& gameFileSystem()
{
	assert(s_gameFiles.has_value());
	return s_gameFiles->fileSystem;
}

const uqm::io::Directory& contentDir()
{
	return gameDirectory(s_gameFiles->contentDir);
}

const uqm::io::Directory& configDir()
{
	return gameDirectory(s_gameFiles->configDir);
}

const uqm::io::Directory& saveDir()
{
	return gameDirectory(s_gameFiles->saveDir);
}

const uqm::io::Directory& meleeDir()
{
	return gameDirectory(s_gameFiles->meleeDir);
}

const uqm::io::Directory& scrShotDir()
{
	return gameDirectory(s_gameFiles->scrShotDir);
}


static const char* findFileInDirs(const char* locs[], int numLocs,
								  const char* file);
static void mountContentDir(const char* contentPath);
static void mountAddonDir(uqgsl::czstring addonDirName);
static void mountDirZips(const uqm::io::Directory& dir, const char* mountPoint, uqm::io::MountPosition position, const uqm::io::Mount& relative);
static void mountBaseZip(const uqm::io::Directory& dir, const char* mountPoint, uqm::io::MountPosition position, const uqm::io::Mount& relative);

// Looks for a file 'file' in all 'numLocs' locations from 'locs'.
// returns the first element from locs where 'file' is found.
// If there is no matching location, nullptr will be returned and
// errno will be set to 'ENOENT'.
// Entries from 'locs' that together with 'file' are longer than
// PATH_MAX will be ignored, except for a warning given to stderr.
static const char* findFileInDirs(const char* locs[], int numLocs, const char* file)
{
	int locI;
	char path[PATH_MAX];
	size_t fileLen;

	fileLen = strlen(file);
	for (locI = 0; locI < numLocs; locI++)
	{
		size_t locLen;
		const char* loc;
		bool needSlash;

		loc = locs[locI];
		locLen = strlen(loc);

		needSlash = (locLen != 0 && loc[locLen - 1] != '/');
		if (locLen + (needSlash ? 1 : 0) + fileLen + 1 >= sizeof path)
		{
			// This dir plus the file name is too long.
			uqm::log::warn("Warning: path '{}' is ignored"
						   " because it is too long.",
						   loc);
			continue;
		}

		fmt::format_to_sz_n(path, "{}{}{}", loc, needSlash ? "/" : "", file);
		if (uqm::fileExists(path))
		{
			return loc;
		}
	}

	// No matching location was found.
	errno = ENOENT;
	return nullptr;
}

// contentDirName is an explicitely specified location for the content,
// or nullptr if none was explicitely specified.
// execFile is the path to the uqm executable, as acquired through
// main()'s argv[0].
void prepareContentDir(uqgsl::czstring contentDirName, uqgsl::czstring addonDirName, uqgsl::czstring execFile)
{
	const char* testFile = "version";
	const char* loc;

	if (isEmpty(contentDirName))
	{
		const uqstl::optional<uqstl::string> expandedContentDir {uqm::expandPath(CONTENTDIR)};
		if (!expandedContentDir)
		{
			uqm::log::critical("Fatal error: Invalid path to "
							   "content files.");
			exit(EXIT_FAILURE);
		}

		// Try the default content locations.
		const char* locs[] = {
			CONTENTDIR,					 /* defined in config.h */
			expandedContentDir->c_str(), /* expanded CONTENTDIR */
			"content",
		};
		loc = findFileInDirs(locs, std::size(locs), testFile);

#ifdef __APPLE__
		/* On OSX, if the content can't be found in any of the static
		 * locations, attempt to look inside the application bundle,
		 * by looking relative to the location of the uqm executable. */
		if (loc == nullptr)
		{
			char* tempDir = (char*)HMalloc(PATH_MAX);
			char* execFileDup;

			/* dirname can modify its argument, so we need a local
			 * mutable copy of it. */
			const uint32_t execFileLen = strlen(execFile);
			execFileDup = (char*)HMalloc(execFileLen + 1);
			uqm::strncpy_safe({execFileDup, execFileLen + 1}, {execFile, execFileLen});
			fmt::format_to_sz_n(tempDir, PATH_MAX, "{}/../Resources/content", dirname(execFileDup));
			loc = findFileInDirs((const char**)&tempDir, 1, testFile);
			HFree(execFileDup);
			HFree(tempDir);
		}
#endif
	}
	else
	{
		// Only use the explicitely supplied content dir.
		loc = findFileInDirs(&contentDirName, 1, testFile);
	}
	if (loc == nullptr)
	{
		uqm::log::critical("Fatal error: Could not find content.");
		exit(EXIT_FAILURE);
	}

	const uqstl::optional<uqstl::string> expandedBase {uqm::expandPath(loc)};
	if (!expandedBase || expandedBase->size() >= sizeof baseContentPath)
	{
		uqm::log::critical("Fatal error: Could not expand path to content "
						   "directory: {}",
						   loc);
		exit(EXIT_FAILURE);
	}
	uqm::strncpy_safe(baseContentPath, *expandedBase);

	uqm::log::debug("Using '{}' as base content dir.", baseContentPath);
	mountContentDir(baseContentPath);

	if (!isEmpty(contentDirName) && isEmpty(contentDirPath))
	{
		contentDirPath = contentDirName;
	}

	if (!isEmpty(addonDirName))
	{
		if (isEmpty(addonDirPath))
		{
			addonDirPath = addonDirName;
		}

		uqm::log::debug("Using '{}' as addon dir.", c_str(addonDirName));
	}
	mountAddonDir(addonDirName);

#ifndef __APPLE__
	(void)execFile;
#endif
}

uqstl::string resolveConfigDir(uqstl::string_view configDirOption)
{
	uqstl::string configDirPath {configDirOption};
	if (configDirPath.empty())
	{
		configDirPath = uqm::getEnvironmentValue("UQM_CONFIG_DIR");
	}
	if (configDirPath.empty())
	{
		configDirPath = CONFIGDIR;
	}

	return uqm::expandPath(configDirPath).value_or(uqstl::string {});
}

void prepareConfigDir(uqstl::string_view configDirOption)
{
	const uqstl::string configDirPath {resolveConfigDir(configDirOption)};
	if (configDirPath.empty())
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		uqm::log::critical("Fatal error: Invalid path to config files.");
		exit(EXIT_FAILURE);
	}
	const char* const configDirName {configDirPath.c_str()};

	uqm::log::debug("Using config dir '{}'", configDirName);

	// Set the environment variable UQM_CONFIG_DIR so UQM_MELEE_DIR
	// and UQM_SAVE_DIR can refer to it.
	uqm::setEnvironmentValue("UQM_CONFIG_DIR", configDirName, true);

	// Create the path upto the config dir, if not already existing.
	if (!uqm::createDirectories(configDirName))
	{
		exit(EXIT_FAILURE);
	}

	uqstl::error_code error {};
	s_gameFiles->configMount = s_gameFiles->fileSystem.mountDirectory("/", configDirName, uqm::io::Access::ReadWrite, uqm::io::MountPosition::Top, nullptr, error);
	if (!s_gameFiles->configMount)
	{
		uqm::log::critical("Fatal error: Could not mount config dir: {}",
						   error.message());
		exit(EXIT_FAILURE);
	}
	s_gameFiles->configDir = s_gameFiles->fileSystem.root();
}

void prepareSaveDir(void)
{
	uqstl::string saveDirName = uqm::getEnvironmentValue("UQM_SAVE_DIR");
	if (saveDirName.empty())
	{
		saveDirName = SAVEDIR;
	}

	const uqstl::optional<uqstl::string> expanded {uqm::expandPath(saveDirName)};
	if (!expanded)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		uqm::log::critical("Fatal error: Invalid path to config files.");
		exit(EXIT_FAILURE);
	}

	saveDirName = *expanded;
	uqm::setEnvironmentValue("UQM_SAVE_DIR", saveDirName.c_str(), true);

	// Create the path upto the save dir, if not already existing.
	if (!uqm::createDirectories(saveDirName.c_str()))
	{
		exit(EXIT_FAILURE);
	}

	uqm::log::debug("Saved games are kept in {}.", saveDirName);

	s_gameFiles->saveDir = uqm::configDir().subdir("save");
	// TODO: this doesn't work if the save dir is not
	//       "save" in the config dir.
	if (!s_gameFiles->saveDir)
	{
		uqm::log::critical("Fatal error: Could not open save dir.");
		exit(EXIT_FAILURE);
	}
}

void prepareMeleeDir(void)
{
	uqstl::string meleeDirName = uqm::getEnvironmentValue("UQM_MELEE_DIR");
	if (meleeDirName.empty())
	{
		meleeDirName = MELEEDIR;
	}

	const uqstl::optional<uqstl::string> expanded {uqm::expandPath(meleeDirName)};
	if (!expanded)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		uqm::log::critical("Fatal error: Invalid path to config files.");
		exit(EXIT_FAILURE);
	}

	meleeDirName = *expanded;
	uqm::setEnvironmentValue("UQM_MELEE_DIR", meleeDirName.c_str(), true);

	// Create the path upto the save dir, if not already existing.
	if (!uqm::createDirectories(meleeDirName.c_str()))
	{
		exit(EXIT_FAILURE);
	}

	s_gameFiles->meleeDir = uqm::configDir().subdir("teams");
	// TODO: this doesn't work if the save dir is not
	//       "teams" in the config dir.
	if (!s_gameFiles->meleeDir)
	{
		uqm::log::critical("Fatal error: Could not open melee teams dir.");
		exit(EXIT_FAILURE);
	}
}

void prepareScrShotDir(void)
{
	uqstl::string shotDirName = uqm::getEnvironmentValue("UQM_SCR_SHOT_DIR");
	if (shotDirName.empty())
	{
		shotDirName = SCRSHOTDIR;
	}

	const uqstl::optional<uqstl::string> expanded {uqm::expandPath(shotDirName)};
	if (!expanded)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		uqm::log::critical("Fatal error: Invalid path to screen shot files.");
		exit(EXIT_FAILURE);
	}

	shotDirName = *expanded;
	uqm::setEnvironmentValue("UQM_SCR_SHOT_DIR", shotDirName.c_str(), true);

	// Create the path upto the save dir, if not already existing.
	if (!uqm::createDirectories(shotDirName.c_str()))
	{
		exit(EXIT_FAILURE);
	}

	s_gameFiles->scrShotDir = uqm::configDir().subdir("screenshots");
	// TODO: this doesn't work if the save dir is not
	//       "screenshots" in the SCRSHOTDIR macro.
	if (!s_gameFiles->scrShotDir)
	{
		uqm::log::critical("Fatal error: Could not open screenshot dir.");
		exit(EXIT_FAILURE);
	}
}

static void
mountContentDir(const char* contentPath)
{
	uqstl::error_code error {};
	s_gameFiles->contentMount = s_gameFiles->fileSystem.mountDirectory("/", contentPath, uqm::io::Access::ReadOnly, uqm::io::MountPosition::Top, nullptr, error);
	if (!s_gameFiles->contentMount)
	{
		uqm::log::critical("Fatal error: Could not mount content dir: {}",
						   error.message());
		exit(EXIT_FAILURE);
	}

	s_gameFiles->contentDir = s_gameFiles->fileSystem.root();

	if (const uqstl::optional<uqm::io::Directory> packagesDir {uqm::contentDir().subdir("packages")})
	{
		mountBaseZip(*packagesDir, "/", uqm::io::MountPosition::Below, *s_gameFiles->contentMount);
	}
}

void mountAddonDir(uqgsl::czstring addonDirName)
{
	const uqm::io::Mount* addonsMount {&*s_gameFiles->contentMount};
	if (!isEmpty(addonDirName))
	{
		uqstl::error_code error {};
		s_gameFiles->addonsMount = s_gameFiles->fileSystem.mountDirectory("addons", addonDirName, uqm::io::Access::ReadOnly, uqm::io::MountPosition::Top, nullptr, error);
		if (!s_gameFiles->addonsMount)
		{
			uqm::log::warn("Warning: Could not mount addon directory: {}"
						   ";\n\t'--addon' options are ignored.",
						   error.message());
			return;
		}
		addonsMount = &*s_gameFiles->addonsMount;
	}

	// NB: note the difference between addonsDir and addonDir.
	//     the former is the dir 'addons', the latter a directory
	//     in that dir.
	const uqstl::optional<uqm::io::Directory> addonsDir {uqm::contentDir().subdir("addons")};
	if (!addonsDir)
	{ // No addon dir found.
		uqm::log::warn("Warning: There's no 'addons' "
					   "directory in the 'content' directory;\n\t'--addon' "
					   "options are ignored.");
		return;
	}

	mountDirZips(*addonsDir, "addons", uqm::io::MountPosition::Below, *addonsMount);

	uqstl::error_code error {};
	uqstl::vector<uqstl::string> availableAddons {addonsDir->list("", uqm::io::Matcher {}, error)};
	// count the actual addon dirs
	uqstl::erase_if(availableAddons, [&addonsDir](const uqstl::string& name) {
		const uqstl::optional<uqm::io::FileInfo> info {addonsDir->stat(name)};
		return name.starts_with('.') || !info || !info->isDirectory();
	});
	uqm::log::info("{} available addon pack{}.", availableAddons.size(),
				   availableAddons.size() == 1 ? "" : "s");

	int count {0};
	for (const uqstl::string& addon : availableAddons)
	{
		g_addonList.push_back(crc32b(addon.c_str()));

		++count;
		uqm::log::info("    {}. {}", count, addon);

		const uqstl::optional<uqm::io::Directory> addonDir {addonsDir->subdir(addon)};
		if (!addonDir)
		{
			uqm::log::warn("Warning: directory 'addons/{}' "
						   "not found; addon skipped.",
						   addon);
			continue;
		}
		const uqstl::string mountName {fmt::format("addons/{}", addon)};
		mountDirZips(*addonDir, mountName.c_str(), uqm::io::MountPosition::Below, *addonsMount);
	}
}

bool isAddonAvailable(uqgsl::czstring addon_name)
{
	if (isEmpty(addon_name))
	{
		return false;
	}

	return uqstl::find(g_addonList.begin(), g_addonList.end(), crc32b(addon_name)) != g_addonList.end();
}

static void
mountArchive(const uqm::io::Directory& dir, const uqstl::string& name, const char* mountPoint, uqm::io::MountPosition position, const uqm::io::Mount& relative)
{
	uqstl::error_code error {};
	if (uqstl::optional<uqm::io::Mount> mount {s_gameFiles->fileSystem.mountArchive(mountPoint, dir, name, position, &relative, error)})
	{
		s_gameFiles->mounts.push_back(uqstl::move(*mount));
	}
	else
	{
		uqm::log::warn("Warning: Could not mount '{}': {}.", name, error.message());
	}
}

static void
mountDirZips(const uqm::io::Directory& dir, const char* mountPoint, uqm::io::MountPosition position, const uqm::io::Mount& relative)
{
	uqstl::error_code error {};
	for (const uqstl::string& name : dir.list("", packageMatcher(), error))
	{
		mountArchive(dir, name, mountPoint, position, relative);
	}
}

static void
mountBaseZip(const uqm::io::Directory& dir, const char* mountPoint, uqm::io::MountPosition position, const uqm::io::Mount& relative)
{
	if (!dir.exists(BASE_CONTENT_NAME))
	{
		uqm::log::warn("Warning: Could not find '{}'.", BASE_CONTENT_NAME);
		return;
	}
	mountArchive(dir, BASE_CONTENT_NAME, mountPoint, position, relative);
}

int loadIndices(const uqm::io::Directory& dir)
{
	uqstl::error_code error {};
	int numLoaded {0};
	for (const uqstl::string& name : dir.list("", uqm::io::Matcher {uqm::io::Matcher::Kind::SuffixIgnoreCase, ".rmp"}, error))
	{
		uqm::log::debug("Loading resource index '{}'", name);
		LoadResourceIndex(dir, name.c_str(), nullptr);
		numLoaded++;
	}

	/* Return the number of index files loaded. */
	return numLoaded;
}

bool loadAddon(uqgsl::czstring addon)
{
	if (isEmpty(addon))
	{
		return false;
	}

	const uqstl::optional<uqm::io::Directory> addonsDir {uqm::contentDir().subdir("addons")};
	if (!addonsDir)
	{
		// If anything fails here, it will fail again later, so
		// we'll just keep quiet about it for now
		uqm::log::warn("Warning: There's no 'addons' "
					   "directory in the 'content' directory;\n\t'--addon' "
					   "options are ignored.");
		return false;
	}
	const uqstl::optional<uqm::io::Directory> addonDir {addonsDir->subdir(addon)};
	if (!addonDir)
	{
		uqm::log::warn("Warning: Addon '{}' not found", c_str(addon));
		return false;
	}

	const int numLoaded {loadIndices(*addonDir)};
	if (!numLoaded)
	{
		uqm::log::error("No RMP index files were loaded for addon '{}'", c_str(addon));
	}

	return numLoaded > 0;
}

void prepareShadowAddons(uqstl::span<const std::string> addons)
{
	const char* shadowDirName = "shadow-content";

	const uqstl::optional<uqm::io::Directory> addonsDir {uqm::contentDir().subdir("addons")};
	if (!addonsDir)
	{
		return;
	}

	for (const auto& addon : addons)
	{
		if (isEmpty(addon))
		{
			break;
		}

		const uqstl::optional<uqm::io::Directory> addonDir {addonsDir->subdir(c_str(addon))};
		if (!addonDir)
		{
			continue;
		}

		// Mount addon's "shadow-content" on top of "/"
		if (const uqstl::optional<uqm::io::Directory> shadowDir {addonDir->subdir(shadowDirName)})
		{
			uqm::log::debug("Mounting shadow content of '{}' addon", c_str(addon));
			mountDirZips(*shadowDir, "/", uqm::io::MountPosition::Above, *s_gameFiles->contentMount);
			// Mount non-zipped shadow content
			uqstl::error_code error {};
			if (uqstl::optional<uqm::io::Mount> mount {s_gameFiles->fileSystem.mountTransplant("/", *shadowDir, uqm::io::Access::ReadOnly, uqm::io::MountPosition::Above, &*s_gameFiles->contentMount, error)})
			{
				s_gameFiles->mounts.push_back(uqstl::move(*mount));
			}
			else
			{
				uqm::log::warn("Warning: Could not mount shadow content"
							   " of '{}': {}.",
							   c_str(addon), error.message());
			}
		}
	}
}

void prepareAddons(uqstl::span<const uqstl::string> addons)
{
	for (const auto& addon : addons)
	{
		uqm::log::info("Loading addon '{}'", addon.c_str());
		if (!loadAddon(c_str(addon)))
		{
			// TODO: Should we do something like inform the user?
			//   Why simply refuse to load other addons?
			//   Maybe exit() to inform the user of the failure?
			break;
		}
	}
}

void createGameFiles(void)
{
	s_gameFiles.emplace();
}

void releaseGameFiles(void)
{
	s_gameFiles.reset();
}

bool setGammaCorrection(float gamma)
{
	bool set = TFB_SetGamma(gamma);
	if (set)
	{
		uqm::log::info("Gamma correction set to {:.4}.", gamma);
	}
	else
	{
		uqm::log::warn("Unable to set gamma correction.");
	}
	return set;
}
} // namespace uqm