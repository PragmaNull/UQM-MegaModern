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
#include "libs/file.h"
#include "config.h"
#include "libs/compiler.h"
#include "libs/uio.h"
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
uio_DirHandle* contentDir;
uio_DirHandle* configDir;
uio_DirHandle* saveDir;
uio_DirHandle* meleeDir;
uio_DirHandle* scrShotDir;
uio_MountHandle* contentMountHandle;

uqgsl::czstring contentDirPath;
uqgsl::czstring addonDirPath;

char baseContentPath[PATH_MAX];

// addon availability
uqstl::vector<uint32_t> g_addonList {};

extern uio_Repository* repository;
extern uio_DirHandle* rootDir;

namespace uqm
{
INPUT_TEMPLATE input_templates[6];


static const char* findFileInDirs(const char* locs[], int numLocs,
								  const char* file);
static uio_MountHandle* mountContentDir(uio_Repository* repository,
										const char* contentPath);
static void mountAddonDir(uio_Repository* repository, uio_MountHandle* contentMountHandle, uqgsl::czstring addonDirName);

static void mountDirZips(uio_DirHandle* dirHandle, const char* mountPoint,
						 int relativeFlags, uio_MountHandle* relativeHandle);

static void mountBaseZip(uio_DirHandle* dirHandle, const char* mountPoint,
						 int relativeFlags, uio_MountHandle* relativeHandle);

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
		if (fileExists(path))
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
		char buf[PATH_MAX];

		if (expandPath(buf, PATH_MAX - 13, CONTENTDIR, EP_ALL_SYSTEM)
			== -1)
		{
			uqm::log::critical("Fatal error: Invalid path to "
							   "content files.");
			exit(EXIT_FAILURE);
		}

		// Try the default content locations.
		const char* locs[] = {
			CONTENTDIR, /* defined in config.h */
			buf,		/* expanded CONTENTDIR */
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

	if (expandPath(baseContentPath, sizeof baseContentPath, loc,
				   EP_ALL_SYSTEM)
		== -1)
	{
		uqm::log::critical("Fatal error: Could not expand path to content "
						   "directory: {}",
						   uqm::strerror(errno));
		exit(EXIT_FAILURE);
	}

	uqm::log::debug("Using '{}' as base content dir.", baseContentPath);
	contentMountHandle = mountContentDir(repository, baseContentPath);

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
	mountAddonDir(repository, contentMountHandle, addonDirName);

#ifndef __APPLE__
	(void)execFile;
#endif
}

void prepareConfigDir(uqstl::string_view configDirOption)
{
	char buf[PATH_MAX];
	static uio_AutoMount* autoMount[] = {nullptr};
	uio_MountHandle* contentHandle;

	uqstl::string configDirPath {configDirOption};
	if (configDirPath.empty())
	{
		configDirPath = uqm::getEnvironmentValue("UQM_CONFIG_DIR");
	}
	if (configDirPath.empty())
	{
		configDirPath = CONFIGDIR;
	}

	if (expandPath(buf, PATH_MAX - 13, configDirPath.c_str(), EP_ALL_SYSTEM)
		== -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		uqm::log::critical("Fatal error: Invalid path to config files.");
		exit(EXIT_FAILURE);
	}
	const char* const configDirName {buf};

	uqm::log::debug("Using config dir '{}'", configDirName);

	// Set the environment variable UQM_CONFIG_DIR so UQM_MELEE_DIR
	// and UQM_SAVE_DIR can refer to it.
	uqm::setEnvironmentValue("UQM_CONFIG_DIR", configDirName, true);

	// Create the path upto the config dir, if not already existing.
	if (mkdirhier(configDirName) == -1)
	{
		exit(EXIT_FAILURE);
	}

	contentHandle = uio_mountDir(repository, "/",
								 uio_FSTYPE_STDIO, nullptr, nullptr, configDirName, autoMount,
								 uio_MOUNT_TOP, nullptr);
	if (contentHandle == nullptr)
	{
		uqm::log::critical("Fatal error: Could not mount config dir: {}",
						   uqm::strerror(errno));
		exit(EXIT_FAILURE);
	}

	configDir = uio_openDir(repository, "/", 0);
	if (configDir == nullptr)
	{
		uqm::log::critical("Fatal error: Could not open config dir: {}",
						   uqm::strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void prepareSaveDir(void)
{
	char buf[PATH_MAX] {};

	uqstl::string saveDirName = uqm::getEnvironmentValue("UQM_SAVE_DIR");
	if (saveDirName.empty())
	{
		saveDirName = SAVEDIR;
	}

	if (expandPath(buf, PATH_MAX - 13, saveDirName.c_str(), EP_ALL_SYSTEM) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		uqm::log::critical("Fatal error: Invalid path to config files.");
		exit(EXIT_FAILURE);
	}

	saveDirName = buf;
	uqm::setEnvironmentValue("UQM_SAVE_DIR", saveDirName.c_str(), true);

	// Create the path upto the save dir, if not already existing.
	if (mkdirhier(saveDirName.c_str()) == -1)
	{
		exit(EXIT_FAILURE);
	}

	uqm::log::debug("Saved games are kept in {}.", saveDirName);

	saveDir = uio_openDirRelative(configDir, "save", 0);
	// TODO: this doesn't work if the save dir is not
	//       "save" in the config dir.
	if (saveDir == nullptr)
	{
		uqm::log::critical("Fatal error: Could not open save dir: {}",
						   uqm::strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void prepareMeleeDir(void)
{
	char buf[PATH_MAX] {};

	uqstl::string meleeDirName = uqm::getEnvironmentValue("UQM_MELEE_DIR");
	if (meleeDirName.empty())
	{
		meleeDirName = MELEEDIR;
	}

	if (expandPath(buf, PATH_MAX - 13, meleeDirName.c_str(), EP_ALL_SYSTEM) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		uqm::log::critical("Fatal error: Invalid path to config files.");
		exit(EXIT_FAILURE);
	}

	meleeDirName = buf;
	uqm::setEnvironmentValue("UQM_MELEE_DIR", meleeDirName.c_str(), true);

	// Create the path upto the save dir, if not already existing.
	if (mkdirhier(meleeDirName.c_str()) == -1)
	{
		exit(EXIT_FAILURE);
	}

	meleeDir = uio_openDirRelative(configDir, "teams", 0);
	// TODO: this doesn't work if the save dir is not
	//       "teams" in the config dir.
	if (meleeDir == nullptr)
	{
		uqm::log::critical("Fatal error: Could not open melee teams dir: {}",
						   uqm::strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void prepareScrShotDir(void)
{
	char buf[PATH_MAX] {};

	uqstl::string shotDirName = uqm::getEnvironmentValue("UQM_SCR_SHOT_DIR");
	if (shotDirName.empty())
	{
		shotDirName = SCRSHOTDIR;
	}

	if (expandPath(buf, PATH_MAX - 13, shotDirName.c_str(), EP_ALL_SYSTEM) == -1)
	{
		// Doesn't have to be fatal, but might mess up things when saving
		// config files.
		uqm::log::critical("Fatal error: Invalid path to screen shot files.");
		exit(EXIT_FAILURE);
	}

	shotDirName = buf;
	uqm::setEnvironmentValue("UQM_SCR_SHOT_DIR", shotDirName.c_str(), true);

	// Create the path upto the save dir, if not already existing.
	if (mkdirhier(shotDirName.c_str()) == -1)
	{
		exit(EXIT_FAILURE);
	}

	scrShotDir = uio_openDirRelative(configDir, "screenshots", 0);
	// TODO: this doesn't work if the save dir is not
	//       "screenshots" in the SCRSHOTDIR macro.
	if (scrShotDir == nullptr)
	{
		uqm::log::critical("Fatal error: Could not open screenshot dir: {}",
						   uqm::strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static uio_MountHandle*
mountContentDir(uio_Repository* repository, const char* contentPath)
{
	uio_DirHandle* packagesDir;
	static uio_AutoMount* autoMount[] = {nullptr};
	uio_MountHandle* contentMountHandle;

	contentMountHandle = uio_mountDir(repository, "/",
									  uio_FSTYPE_STDIO, nullptr, nullptr, contentPath, autoMount,
									  uio_MOUNT_TOP | uio_MOUNT_RDONLY, nullptr);
	if (contentMountHandle == nullptr)
	{
		uqm::log::critical("Fatal error: Could not mount content dir: {}",
						   uqm::strerror(errno));
		exit(EXIT_FAILURE);
	}

	contentDir = uio_openDir(repository, "/", 0);
	if (contentDir == nullptr)
	{
		uqm::log::critical("Fatal error: Could not open content dir: {}",
						   uqm::strerror(errno));
		exit(EXIT_FAILURE);
	}

	packagesDir = uio_openDir(repository, "/packages", 0);
	if (packagesDir != nullptr)
	{
		mountBaseZip(packagesDir, "/", uio_MOUNT_BELOW,
					 contentMountHandle);
		uio_closeDir(packagesDir);
	}

	return contentMountHandle;
}

void mountAddonDir(uio_Repository* repository, uio_MountHandle* contentMountHandle, uqgsl::czstring addonDirName)
{
	uio_DirHandle* addonsDir;
	static uio_AutoMount* autoMount[] = {nullptr};
	uio_MountHandle* mountHandle;
	uio_DirList* availableAddons;

	if (!isEmpty(addonDirName))
	{
		mountHandle = uio_mountDir(repository, "addons",
								   uio_FSTYPE_STDIO, nullptr, nullptr, addonDirName, autoMount,
								   uio_MOUNT_TOP | uio_MOUNT_RDONLY, nullptr);
		if (mountHandle == nullptr)
		{
			uqm::log::warn("Warning: Could not mount addon directory: {}"
						   ";\n\t'--addon' options are ignored.",
						   uqm::strerror(errno));
			return;
		}
	}
	else
	{
		mountHandle = contentMountHandle;
	}

	// NB: note the difference between addonsDir and addonDir.
	//     the former is the dir 'addons', the latter a directory
	//     in that dir.
	addonsDir = uio_openDirRelative(contentDir, "addons", 0);
	if (addonsDir == nullptr)
	{ // No addon dir found.
		uqm::log::warn("Warning: There's no 'addons' "
					   "directory in the 'content' directory;\n\t'--addon' "
					   "options are ignored.");
		return;
	}

	mountDirZips(addonsDir, "addons", uio_MOUNT_BELOW, mountHandle);

	availableAddons = uio_getDirList(addonsDir, "", "", match_MATCH_PREFIX);
	if (availableAddons != nullptr)
	{
		int i, count;

		// count the actual addon dirs
		count = 0;
		for (i = 0; i < availableAddons->numNames; ++i)
		{
			struct stat sb;

			if (availableAddons->names[i][0] == '.' || uio_stat(addonsDir, availableAddons->names[i], &sb) == -1
				|| !S_ISDIR(sb.st_mode))
			{ // this dir entry ignored
				availableAddons->names[i] = nullptr;
				continue;
			}
			++count;
		}
		uqm::log::info("{} available addon pack{}.", count,
					   count == 1 ? "" : "s");

		count = 0;
		for (i = 0; i < availableAddons->numNames; ++i)
		{
			char mountname[128] {};
			uio_DirHandle* addonDir;
			const char* addon = availableAddons->names[i];

			if (!addon)
			{
				continue;
			}

			g_addonList.push_back(crc32b(addon));

			++count;
			uqm::log::info("    {}. {}", count, addon);

			fmt::format_to_sz_n(mountname, "addons/{}", addon);

			addonDir = uio_openDirRelative(addonsDir, addon, 0);
			if (addonDir == nullptr)
			{
				uqm::log::warn("Warning: directory 'addons/{}' "
							   "not found; addon skipped.",
							   addon);
				continue;
			}
			mountDirZips(addonDir, mountname, uio_MOUNT_BELOW, mountHandle);
			uio_closeDir(addonDir);
		}
	}
	else
	{
		uqm::log::info("0 available addon packs.");
	}

	uio_DirList_free(availableAddons);
	uio_closeDir(addonsDir);
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
mountDirZips(uio_DirHandle* dirHandle, const char* mountPoint,
			 int relativeFlags, uio_MountHandle* relativeHandle)
{
	static uio_AutoMount* autoMount[] = {nullptr};
	uio_DirList* dirList;
	const char* pattern = "\\.([zZ][iI][pP]|[uU][qQ][mM])$";

	dirList = uio_getDirList(dirHandle, "", pattern, match_MATCH_REGEX);
	if (dirList != nullptr)
	{
		int i;

		for (i = 0; i < dirList->numNames; i++)
		{
			if (uio_mountDir(repository, mountPoint, uio_FSTYPE_ZIP,
							 dirHandle, dirList->names[i], "/", autoMount,
							 relativeFlags | uio_MOUNT_RDONLY,
							 relativeHandle)
				== nullptr)
			{
				uqm::log::warn("Warning: Could not mount '{}': {}.",
							   dirList->names[i], uqm::strerror(errno));
			}
		}
	}
	uio_DirList_free(dirList);
}

static void
mountBaseZip(uio_DirHandle* dirHandle, const char* mountPoint,
			 int relativeFlags, uio_MountHandle* relativeHandle)
{
	static uio_AutoMount* autoMount[] = {nullptr};
	uio_DirList* dirList;
	const char* pattern = "\\.([zZ][iI][pP]|[uU][qQ][mM])$";
	const uint32_t name_hash = crc32b(BASE_CONTENT_NAME);

	dirList = uio_getDirList(dirHandle, "", pattern, match_MATCH_REGEX);
	if (dirList != nullptr)
	{
		uint32_t names_hash = 0;
		int i;

		for (i = 0; i < dirList->numNames; i++)
		{
			names_hash = crc32b(dirList->names[i]);
			if (name_hash == names_hash)
			{
				break;
			}
		}

		if (i == dirList->numNames)
		{
			if (name_hash != names_hash)
			{
				uqm::log::warn("Warning: Could not find '{}': {}.",
							   BASE_CONTENT_NAME, uqm::strerror(errno));

				uio_DirList_free(dirList);
				return;
			}
			else
			{
				i--;
			}
		}

		if (uio_mountDir(repository, mountPoint, uio_FSTYPE_ZIP,
						 dirHandle, dirList->names[i], "/", autoMount,
						 relativeFlags | uio_MOUNT_RDONLY,
						 relativeHandle)
			== nullptr)
		{
			uqm::log::warn("Warning: Could not mount '{}': {}.",
						   dirList->names[i], uqm::strerror(errno));
		}
	}
	uio_DirList_free(dirList);
}

int loadIndices(uio_DirHandle* dir)
{
	uio_DirList* indices;
	int numLoaded = 0;

	indices = uio_getDirList(dir, "", "\\.[rR][mM][pP]$",
							 match_MATCH_REGEX);

	if (indices != nullptr)
	{
		int i;

		for (i = 0; i < indices->numNames; i++)
		{
			uqm::log::debug("Loading resource index '{}'",
							indices->names[i]);
			LoadResourceIndex(dir, indices->names[i], nullptr);
			numLoaded++;
		}
	}
	uio_DirList_free(indices);

	/* Return the number of index files loaded. */
	return numLoaded;
}

bool loadAddon(uqgsl::czstring addon)
{
	if (isEmpty(addon))
	{
		return false;
	}

	uio_DirHandle *addonsDir, *addonDir;
	int numLoaded;

	addonsDir = uio_openDirRelative(contentDir, "addons", 0);
	if (addonsDir == nullptr)
	{
		// No addon dir found.
		uqm::log::warn("Warning: There's no 'addons' "
					   "directory in the 'content' directory;\n\t'--addon' "
					   "options are ignored.");
		return false;
	}
	addonDir = uio_openDirRelative(addonsDir, addon, 0);
	if (addonDir == nullptr)
	{
		uqm::log::warn("Warning: Addon '{}' not found", c_str(addon));
		uio_closeDir(addonsDir);
		return false;
	}

	numLoaded = loadIndices(addonDir);
	if (!numLoaded)
	{
		uqm::log::error("No RMP index files were loaded for addon '{}'", c_str(addon));
	}

	uio_closeDir(addonDir);
	uio_closeDir(addonsDir);

	return (bool)(numLoaded > 0);
}

void prepareShadowAddons(uqstl::span<const std::string> addons)
{
	uio_DirHandle* addonsDir;
	const char* shadowDirName = "shadow-content";

	addonsDir = uio_openDirRelative(contentDir, "addons", 0);
	// If anything fails here, it will fail again later, so
	// we'll just keep quiet about it for now
	if (addonsDir == nullptr)
	{
		return;
	}

	for (const auto& addon : addons)
	{
		if (isEmpty(addon))
		{
			break;
		}

		uio_DirHandle* addonDir;
		uio_DirHandle* shadowDir;

		addonDir = uio_openDirRelative(addonsDir, c_str(addon), 0);
		if (addonDir == nullptr)
		{
			continue;
		}

		// Mount addon's "shadow-content" on top of "/"
		shadowDir = uio_openDirRelative(addonDir, shadowDirName, 0);
		if (shadowDir)
		{
			uqm::log::debug("Mounting shadow content of '{}' addon", c_str(addon));
			mountDirZips(shadowDir, "/", uio_MOUNT_ABOVE, contentMountHandle);
			// Mount non-zipped shadow content
			if (uio_transplantDir("/", shadowDir, uio_MOUNT_RDONLY | uio_MOUNT_ABOVE, contentMountHandle) == nullptr)
			{
				uqm::log::warn("Warning: Could not mount shadow content"
							   " of '{}': {}.",
							   c_str(addon), uqm::strerror(errno));
			}

			uio_closeDir(shadowDir);
		}
		uio_closeDir(addonDir);
	}

	uio_closeDir(addonsDir);
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

void unprepareAllDirs(void)
{
	if (saveDir)
	{
		uio_closeDir(saveDir);
		saveDir = 0;
	}
	if (meleeDir)
	{
		uio_closeDir(meleeDir);
		meleeDir = 0;
	}
	if (contentDir)
	{
		uio_closeDir(contentDir);
		contentDir = 0;
	}
	if (configDir)
	{
		uio_closeDir(configDir);
		configDir = 0;
	}
	if (scrShotDir)
	{
		uio_closeDir(scrShotDir);
		scrShotDir = 0;
	}
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