// Tests that the JSON data files in the content directory are valid.
#include <gtest/gtest.h>

#include "core/stl/filesystem.h"
#include "core/stl/fstream.h"
#include "core/stl/sstream.h"
#include "libs/graphics/anifile.h"
#include "libs/graphics/fontfile.h"
#include "libs/resource/resindex.h"
#include "libs/sound/soundbank.h"
#include "uqm/creditsfile.h"
#include "uqm/slideshow.h"

namespace
{

uqstl::string readFile(const uqstl::filesystem::path& path)
{
	uqstl::ifstream file {path, uqstl::ios::binary};
	uqstl::ostringstream text {};
	text << file.rdbuf();
	return text.str();
}

bool endsWith(const uqstl::filesystem::path& path, uqstl::string_view suffix)
{
	return path.filename().string().ends_with(suffix);
}

} // namespace

TEST(ContentFilesTest, AllDataFilesAreValid)
{
	const uqstl::filesystem::path content {UQM_TEST_CONTENT_DIR};
	if (!uqstl::filesystem::exists(content / "uqm.rmp.json"))
	{
		GTEST_SKIP() << "No content directory";
	}

	int count {};
	for (auto it {uqstl::filesystem::recursive_directory_iterator {content}}; it != uqstl::filesystem::recursive_directory_iterator {};
		 ++it)
	{
		const uqstl::filesystem::path& path {it->path()};
		if (path.filename() == ".git")
		{
			it.disable_recursion_pending();
			continue;
		}
		if (!it->is_regular_file() || path.extension() != ".json")
		{
			continue;
		}
		const uqstl::string text {readFile(path)};
		SCOPED_TRACE(path.string());
		if (endsWith(path, ".rmp.json") || endsWith(path, ".key.json"))
		{
			EXPECT_TRUE(uqm::resourceIndexFromJson(text, ""));
		}
		else if (endsWith(path, ".ani.json"))
		{
			EXPECT_TRUE(uqm::aniFromJson(text));
		}
		else if (endsWith(path, ".snd.json"))
		{
			EXPECT_TRUE(uqm::soundBankFromJson(text));
		}
		else if (endsWith(path, "kerndat.fnt.json"))
		{
			EXPECT_TRUE(uqm::fontMetricsFromJson(text));
		}
		else if (text.find("\"commands\"") != uqstl::string::npos)
		{
			EXPECT_TRUE(uqm::slideshowFromJson(text));
		}
		else if (text.find("\"blocks\"") != uqstl::string::npos)
		{
			EXPECT_TRUE(uqm::creditsFromJson(text));
		}
		else
		{
			continue;
		}
		++count;
	}
	EXPECT_GT(count, 0);
}
