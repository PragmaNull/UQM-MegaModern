// Tests for the host path expansion and directory creation.
#include <gtest/gtest.h>

#include "core/platform/platform.h"
#include "libs/file/paths.h"

#include <filesystem>

namespace uqm
{

namespace
{

namespace fs = uqstl::filesystem;

uqstl::string generic(const fs::path& path)
{
	return path.lexically_normal().generic_string();
}

#ifdef _WIN32
constexpr const char* c_testVar {"%UQM_TEST_PATHS_VAR%"};
constexpr const char* c_unsetVar {"%UQM_TEST_PATHS_UNSET%"};
constexpr const char* c_unterminatedVar {"%UQM_TEST_PATHS_VAR/x"};
#else
constexpr const char* c_testVar {"${UQM_TEST_PATHS_VAR}"};
constexpr const char* c_unsetVar {"$UQM_TEST_PATHS_UNSET"};
constexpr const char* c_unterminatedVar {"${UQM_TEST_PATHS_VAR/x"};
#endif

} // namespace

TEST(PathsTest, ExpandsEnvironmentVariables)
{
	const fs::path base {fs::temp_directory_path() / "uqm-paths-test"};
	ASSERT_TRUE(setEnvironmentValue("UQM_TEST_PATHS_VAR", base.string().c_str(), true));

	const auto expanded {expandPath(uqstl::string {c_testVar} + "/save/")};
	ASSERT_TRUE(expanded);
	EXPECT_EQ(*expanded, generic(base / "save/"));
}

TEST(PathsTest, UnsetVariableIsEmpty)
{
	const auto expanded {expandPath(uqstl::string {"a"} + c_unsetVar + "/b")};
	ASSERT_TRUE(expanded);
	EXPECT_EQ(*expanded, generic(fs::current_path() / "a" / "b"));
}

TEST(PathsTest, RejectsUnterminatedVariable)
{
	EXPECT_FALSE(expandPath(c_unterminatedVar));
}

TEST(PathsTest, ExpandsHomeDirectory)
{
	const auto expanded {expandPath("~/.uqm-test")};
	ASSERT_TRUE(expanded);
	EXPECT_TRUE(expanded->ends_with("/.uqm-test"));
	EXPECT_FALSE(expanded->starts_with("~"));
	EXPECT_TRUE(fs::path {*expanded}.is_absolute());

	EXPECT_FALSE(expandPath("~someone/x"));
}

TEST(PathsTest, NormalizesRelativePaths)
{
	const auto expanded {expandPath("a/./b/../c//d")};
	ASSERT_TRUE(expanded);
	EXPECT_EQ(*expanded, generic(fs::current_path() / "a" / "c" / "d"));
}

#ifdef _WIN32
TEST(PathsTest, AcceptsBackslashesAndDriveLetters)
{
	const auto expanded {expandPath("C:\\games\\\\uqm\\.\\content")};
	ASSERT_TRUE(expanded);
	EXPECT_EQ(*expanded, "C:/games/uqm/content");
}
#endif

TEST(PathsTest, CreatesDirectories)
{
	const fs::path dir {fs::temp_directory_path() / "uqm-paths-test" / "a" / "b"};
	fs::remove_all(dir.parent_path().parent_path());

	EXPECT_FALSE(fileExists(dir.string().c_str()));
	EXPECT_TRUE(createDirectories(dir.string().c_str()));
	EXPECT_TRUE(fileExists(dir.string().c_str()));
	EXPECT_TRUE(createDirectories(dir.string().c_str()));

	fs::remove_all(dir.parent_path().parent_path());
}

} // namespace uqm
