// Checks that a save file loads and saves again with the same content.

#include "SaveCheck.h"

#include <fmt/format.h>
#include <yyjson.h>

#include "core/log/log.h"
#include "core/stl/memory.h"
#include "core/stl/vector.h"
#include "libs/file/textfile.h"
#include "uqm/save.h"
#include "uqm/session/GameSession.h"
#include "UQMGame.h"


namespace uqm
{
namespace
{

using DocPtr = uqstl::unique_ptr<yyjson_doc, decltype(&yyjson_doc_free)>;

constexpr size_t c_maxReportedDifferences {50};
// Values that the save takes from the settings of the running game, not from the game state.
constexpr uqstl::array<uqstl::string_view, 1> c_ignoredPaths {"/summary/res_factor"};

[[nodiscard]] DocPtr readJson(uqstl::string_view text)
{
	return {yyjson_read(text.data(), text.size(), YYJSON_READ_ALLOW_INVALID_UNICODE), &yyjson_doc_free};
}

[[nodiscard]] uqstl::string valueText(yyjson_val* val)
{
	if (val == nullptr)
	{
		return "(missing)";
	}
	size_t length {};
	const uqstl::unique_ptr<char, decltype(&free)> text {yyjson_val_write(val, YYJSON_WRITE_ALLOW_INVALID_UNICODE, &length), &free};
	return text ? uqstl::string {text.get(), length} : uqstl::string {"?"};
}

// Adds the JSON pointer of each value that differs between 'loaded' and 'saved'.
void compare(yyjson_val* loaded, yyjson_val* saved, const uqstl::string& path, uqstl::vector<uqstl::string>& differences)
{
	if (uqstl::find(uqstl::begin(c_ignoredPaths), uqstl::end(c_ignoredPaths), path) != uqstl::end(c_ignoredPaths))
	{
		return;
	}
	if (yyjson_is_obj(loaded) && yyjson_is_obj(saved))
	{
		size_t index {};
		size_t max {};
		yyjson_val* key {};
		yyjson_val* child {};
		yyjson_obj_foreach(loaded, index, max, key, child)
		{
			compare(child, yyjson_obj_getn(saved, yyjson_get_str(key), yyjson_get_len(key)),
					fmt::format("{}/{}", path, yyjson_get_str(key)), differences);
		}
		yyjson_obj_foreach(saved, index, max, key, child)
		{
			if (yyjson_obj_getn(loaded, yyjson_get_str(key), yyjson_get_len(key)) == nullptr)
			{
				compare(nullptr, child, fmt::format("{}/{}", path, yyjson_get_str(key)), differences);
			}
		}
		return;
	}
	if (yyjson_is_arr(loaded) && yyjson_is_arr(saved) && yyjson_arr_size(loaded) == yyjson_arr_size(saved))
	{
		for (size_t index {0}; index < yyjson_arr_size(loaded); ++index)
		{
			compare(yyjson_arr_get(loaded, index), yyjson_arr_get(saved, index), fmt::format("{}/{}", path, index), differences);
		}
		return;
	}
	if (loaded == nullptr || saved == nullptr || !yyjson_equals(loaded, saved))
	{
		differences.push_back(fmt::format("{}: file {}, saved {}", path, valueText(loaded), valueText(saved)));
	}
}

[[nodiscard]] bool roundTrip(const uqstl::string& path, const uqstl::string& text)
{
	SaveSummaryData summary {};
	if (!LoadGameFromJson(text, path, &summary))
	{
		return false;
	}
	if (!LoadGameFromJson(text, path, nullptr))
	{
		log::error("{}: the game did not load.", path);
		return false;
	}
	// The game loop starts the loaded activity in the same way.
	currentActivity() = NextActivity;
	if (!(currentActivity() & ActivityFlag::StartEncounter) && (currentActivity() & ActivityFlag::StartInterplanetary))
	{
		currentActivity() = makeWord(ActivityKind::InInterplanetary, 0);
	}

	const auto saved {SaveGameJson(&summary, summary.SaveName)};
	if (!saved)
	{
		log::error("{}: the game did not save.", path);
		return false;
	}

	const DocPtr loadedDoc {readJson(text)};
	const DocPtr savedDoc {readJson(*saved)};
	if (!loadedDoc || !savedDoc)
	{
		log::error("{}: could not read the JSON to compare it.", path);
		return false;
	}
	uqstl::vector<uqstl::string> differences {};
	compare(yyjson_doc_get_root(loadedDoc.get()), yyjson_doc_get_root(savedDoc.get()), {}, differences);
	for (const uqstl::string& difference : differences | uqstl::views::take(c_maxReportedDifferences))
	{
		log::error("{}: {}", path, difference);
	}
	if (!differences.empty())
	{
		log::error("{}: {} values changed after a load and a save.", path, differences.size());
		return false;
	}
	log::info("{}: the save loads and saves again with no change.", path);
	return true;
}

} // namespace

bool checkSaveRoundTrip(const uqstl::string& path)
{
	const auto text {readTextFile(path.c_str())};
	if (!text)
	{
		log::error("Could not open the save file '{}'.", path);
		return false;
	}

	UQMGame& game {UQMGame::getInstance()};
	game.beginSession(SessionKind::FullGame);
	const bool same {roundTrip(path, *text)};
	game.endSession();
	return same;
}

} // namespace uqm
