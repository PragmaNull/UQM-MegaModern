// Tests for the virtual file system in libs/io.
#include "libs/io/FileSystem.h"
#include "libs/io/Path.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>

namespace fs = uqstl::filesystem;
using namespace uqm::io;

namespace
{

uqstl::string readHostFile(const fs::path& path)
{
	uqstl::ifstream file {path, uqstl::ios::binary};
	uqstl::stringstream contents {};
	contents << file.rdbuf();
	return contents.str();
}

void writeHostFile(const fs::path& path, uqstl::string_view contents)
{
	fs::create_directories(path.parent_path());
	uqstl::ofstream file {path, uqstl::ios::binary};
	file << contents;
}

uqstl::string readAll(File& file)
{
	uqstl::string result {};
	uqstl::array<uqstl::byte, 1000> buffer {};
	while (const size_t count {file.read(buffer)})
	{
		for (size_t i {0}; i < count; ++i)
		{
			result += uqstl::to_integer<char>(buffer[i]);
		}
	}
	return result;
}

size_t writeText(File& file, uqstl::string_view text)
{
	return file.write(uqstl::as_bytes(uqstl::span {text.data(), text.size()}));
}

uqstl::string readText(const Directory& dir, uqstl::string_view path)
{
	uqstl::error_code error {};
	uqstl::optional<File> file {dir.open(path, OpenMode::Read, error)};
	if (!file)
	{
		return "<" + error.message() + ">";
	}
	return readAll(*file);
}

class IoTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		static uqstl::atomic<int> counter {0};
		const auto stamp {uqstl::chrono::steady_clock::now().time_since_epoch().count()};
		m_root = fs::temp_directory_path() / ("uqm_io_test_" + uqstl::to_string(stamp) + "_" + uqstl::to_string(counter++));
		fs::create_directories(m_root);
	}

	void TearDown() override
	{
		uqstl::error_code error {};
		fs::remove_all(m_root, error);
	}

	fs::path hostDir(uqstl::string_view name)
	{
		const fs::path dir {m_root / uqstl::string {name}};
		fs::create_directories(dir);
		return dir;
	}

	Mount mount(uqstl::string_view mountPoint, const fs::path& host, Access access, MountPosition position = MountPosition::Top, const Mount* relative = nullptr)
	{
		uqstl::error_code error {};
		uqstl::optional<Mount> result {m_fileSystem.mountDirectory(mountPoint, host, access, position, relative, error)};
		EXPECT_TRUE(result.has_value()) << error.message();
		return result ? uqstl::move(*result) : Mount {};
	}

	fs::path m_root {};
	FileSystem m_fileSystem {};
};

} // namespace

TEST(IoPath, Normalize)
{
	EXPECT_EQ(path::normalize("/a//b/./c/"), "a/b/c");
	EXPECT_EQ(path::normalize("a/../b"), "b");
	EXPECT_EQ(path::normalize("../a"), "a");
	EXPECT_EQ(path::normalize("/"), "");
}

TEST(IoPath, JoinAndWithin)
{
	EXPECT_EQ(path::join("a/b", "c"), "a/b/c");
	EXPECT_EQ(path::join("a/b", "/c"), "c");
	EXPECT_EQ(path::join("", "c/.."), "");
	EXPECT_TRUE(path::isWithin("a/b", "a"));
	EXPECT_TRUE(path::isWithin("a", ""));
	EXPECT_FALSE(path::isWithin("ab", "a"));
	EXPECT_EQ(path::relativeTo("a/b/c", "a"), "b/c");
	EXPECT_EQ(path::parent("a/b/c"), "a/b");
	EXPECT_EQ(path::fileName("a/b/c"), "c");
}

TEST(IoMatcher, Kinds)
{
	EXPECT_TRUE(Matcher::create(Matcher::Kind::Literal, "a.txt")->matches("a.txt"));
	EXPECT_FALSE(Matcher::create(Matcher::Kind::Literal, "a.txt")->matches("a.txt2"));
	EXPECT_TRUE(Matcher::create(Matcher::Kind::Prefix, "a")->matches("abc"));
	EXPECT_TRUE(Matcher::create(Matcher::Kind::Suffix, ".rmp")->matches("uqm.rmp"));
	EXPECT_TRUE(Matcher::create(Matcher::Kind::Substring, "mp")->matches("uqm.rmp"));
	EXPECT_TRUE(Matcher::create(Matcher::Kind::Regex, "\\.([zZ][iI][pP]|[uU][qQ][mM])$")->matches("pack.UQM"));
	EXPECT_FALSE(Matcher::create(Matcher::Kind::Regex, "\\.zip$")->matches("zip.txt"));
	EXPECT_FALSE(Matcher::create(Matcher::Kind::Regex, "(").has_value());
	EXPECT_TRUE(Matcher {}.matches("anything"));
}

TEST_F(IoTest, WriteAndReadBack)
{
	const fs::path host {hostDir("rw")};
	const Mount rw {mount("/", host, Access::ReadWrite)};
	const Directory root {m_fileSystem.root()};

	uqstl::error_code error {};
	{
		uqstl::optional<File> file {root.open("hello.txt", OpenMode::Write | OpenMode::Create | OpenMode::Truncate, error)};
		ASSERT_TRUE(file.has_value()) << error.message();
		EXPECT_EQ(writeText(*file, "hello"), 5u);
	}
	EXPECT_EQ(readHostFile(host / "hello.txt"), "hello");
	EXPECT_EQ(readText(root, "hello.txt"), "hello");

	const uqstl::optional<FileInfo> info {root.stat("hello.txt")};
	ASSERT_TRUE(info.has_value());
	EXPECT_EQ(info->size, 5u);
	EXPECT_FALSE(info->isDirectory());
}

TEST_F(IoTest, OpenMissingFileFails)
{
	const Mount rw {mount("/", hostDir("rw"), Access::ReadWrite)};
	uqstl::error_code error {};
	EXPECT_FALSE(m_fileSystem.root().open("missing.txt", OpenMode::Read, error).has_value());
	EXPECT_EQ(error, uqstl::errc::no_such_file_or_directory);
	EXPECT_FALSE(m_fileSystem.root().open("nodir/new.txt", OpenMode::Write | OpenMode::Create, error).has_value());
}

TEST_F(IoTest, TopMountHidesBottomMount)
{
	const fs::path lower {hostDir("lower")};
	const fs::path upper {hostDir("upper")};
	writeHostFile(lower / "a.txt", "lower");
	writeHostFile(lower / "only-lower.txt", "only");
	writeHostFile(upper / "a.txt", "upper");

	const Mount lowerMount {mount("/", lower, Access::ReadOnly)};
	const Mount upperMount {mount("/", upper, Access::ReadOnly)};
	const Directory root {m_fileSystem.root()};
	EXPECT_EQ(readText(root, "a.txt"), "upper");
	EXPECT_EQ(readText(root, "only-lower.txt"), "only");
}

TEST_F(IoTest, AboveAndBelowPlaceMountsNextToTheRelativeMount)
{
	const fs::path first {hostDir("first")};
	const fs::path second {hostDir("second")};
	const fs::path third {hostDir("third")};
	writeHostFile(first / "a.txt", "first");
	writeHostFile(second / "a.txt", "second");
	writeHostFile(second / "b.txt", "second");
	writeHostFile(third / "b.txt", "third");

	const Mount top {mount("/", first, Access::ReadOnly)};
	const Mount above {mount("/", second, Access::ReadOnly, MountPosition::Above, &top)};
	const Mount below {mount("/", third, Access::ReadOnly, MountPosition::Below, &above)};
	const Directory root {m_fileSystem.root()};
	EXPECT_EQ(readText(root, "a.txt"), "second");
	// 'third' is below 'second', so it does not hide b.txt.
	EXPECT_EQ(readText(root, "b.txt"), "second");
}

TEST_F(IoTest, NewFilesGoToTheWritableMount)
{
	const fs::path config {hostDir("config")};
	const fs::path content {hostDir("content")};
	fs::create_directories(content / "save");

	const Mount configMount {mount("/", config, Access::ReadWrite)};
	const Mount contentMount {mount("/", content, Access::ReadOnly)};

	uqstl::error_code error {};
	uqstl::optional<File> file {m_fileSystem.root().open("save/game.sav", OpenMode::Write | OpenMode::Create, error)};
	ASSERT_TRUE(file.has_value()) << error.message();
	writeText(*file, "data");
	file->close();
	EXPECT_EQ(readHostFile(config / "save" / "game.sav"), "data");
	EXPECT_FALSE(fs::exists(content / "save" / "game.sav"));
}

TEST_F(IoTest, WritingAReadOnlyFileCopiesItUp)
{
	const fs::path lower {hostDir("lower")};
	const fs::path upper {hostDir("upper")};
	writeHostFile(lower / "dir" / "a.txt", "abc");

	const Mount lowerMount {mount("/", lower, Access::ReadOnly)};
	const Mount upperMount {mount("/", upper, Access::ReadWrite)};

	uqstl::error_code error {};
	uqstl::optional<File> file {m_fileSystem.root().open("dir/a.txt", OpenMode::Read | OpenMode::Write, error)};
	ASSERT_TRUE(file.has_value()) << error.message();
	ASSERT_TRUE(file->seek(0, SeekFrom::End));
	writeText(*file, "d");
	file->close();

	EXPECT_EQ(readHostFile(upper / "dir" / "a.txt"), "abcd");
	EXPECT_EQ(readHostFile(lower / "dir" / "a.txt"), "abc");
}

TEST_F(IoTest, WritingAReadOnlyFileWithoutAWritableMountAboveFails)
{
	const fs::path lower {hostDir("lower")};
	const fs::path upper {hostDir("upper")};
	writeHostFile(upper / "a.txt", "abc");

	const Mount lowerMount {mount("/", lower, Access::ReadWrite)};
	const Mount upperMount {mount("/", upper, Access::ReadOnly)};

	uqstl::error_code error {};
	EXPECT_FALSE(m_fileSystem.root().open("a.txt", OpenMode::Write, error).has_value());
	EXPECT_EQ(error, uqstl::errc::permission_denied);
}

TEST_F(IoTest, ListMergesMountsAndShowsMountPoints)
{
	const fs::path lower {hostDir("lower")};
	const fs::path upper {hostDir("upper")};
	const fs::path addons {hostDir("addons")};
	writeHostFile(lower / "b.txt", "");
	writeHostFile(lower / "c.rmp", "");
	writeHostFile(upper / "a.txt", "");
	writeHostFile(upper / "b.txt", "");

	const Mount lowerMount {mount("/", lower, Access::ReadOnly)};
	const Mount upperMount {mount("/", upper, Access::ReadOnly)};
	const Mount addonsMount {mount("packs/addons", addons, Access::ReadOnly)};

	uqstl::error_code error {};
	const Directory root {m_fileSystem.root()};
	EXPECT_EQ(root.list("", Matcher {}, error), (uqstl::vector<uqstl::string> {"a.txt", "b.txt", "c.rmp", "packs"}));
	EXPECT_EQ(root.list("", *Matcher::create(Matcher::Kind::Regex, "\\.[rR][mM][pP]$"), error), (uqstl::vector<uqstl::string> {"c.rmp"}));
	EXPECT_TRUE(root.stat("packs").value().isDirectory());
	EXPECT_TRUE(root.subdir("packs/addons").has_value());
}

TEST_F(IoTest, MakeDirAndRemove)
{
	const fs::path host {hostDir("rw")};
	const Mount rw {mount("/", host, Access::ReadWrite)};
	const Directory root {m_fileSystem.root()};

	EXPECT_FALSE(root.makeDir("newdir"));
	EXPECT_TRUE(fs::is_directory(host / "newdir"));
	EXPECT_EQ(root.makeDir("newdir"), uqstl::errc::file_exists);

	writeHostFile(host / "newdir" / "x.txt", "x");
	EXPECT_FALSE(root.remove("newdir/x.txt"));
	EXPECT_FALSE(fs::exists(host / "newdir" / "x.txt"));
	EXPECT_EQ(root.remove("newdir/x.txt"), uqstl::errc::no_such_file_or_directory);
}

TEST_F(IoTest, Rename)
{
	const fs::path host {hostDir("rw")};
	writeHostFile(host / "a.txt", "a");
	const Mount rw {mount("/", host, Access::ReadWrite)};
	const Directory root {m_fileSystem.root()};

	EXPECT_FALSE(root.rename("a.txt", root, "sub/b.txt"));
	EXPECT_EQ(readHostFile(host / "sub" / "b.txt"), "a");
}

TEST_F(IoTest, TransplantShowsADirectoryAtAnotherMountPoint)
{
	const fs::path content {hostDir("content")};
	writeHostFile(content / "base.txt", "base");
	writeHostFile(content / "addons" / "mod" / "shadow" / "base.txt", "shadow");

	const Mount contentMount {mount("/", content, Access::ReadOnly)};
	const uqstl::optional<Directory> shadow {m_fileSystem.root().subdir("addons/mod/shadow")};
	ASSERT_TRUE(shadow.has_value());

	uqstl::error_code error {};
	uqstl::optional<Mount> transplant {m_fileSystem.mountTransplant("/", *shadow, Access::ReadOnly, MountPosition::Above, &contentMount, error)};
	ASSERT_TRUE(transplant.has_value()) << error.message();
	EXPECT_EQ(readText(m_fileSystem.root(), "base.txt"), "shadow");

	transplant->unmount();
	EXPECT_EQ(readText(m_fileSystem.root(), "base.txt"), "base");
}

TEST_F(IoTest, DestroyingAMountUnmountsIt)
{
	const fs::path host {hostDir("rw")};
	writeHostFile(host / "a.txt", "a");
	{
		const Mount rw {mount("/", host, Access::ReadOnly)};
		EXPECT_TRUE(m_fileSystem.root().exists("a.txt"));
	}
	EXPECT_FALSE(m_fileSystem.root().exists("a.txt"));
}

TEST_F(IoTest, RemoveChangesNothingWhenACopyIsReadOnly)
{
	const fs::path lower {hostDir("lower")};
	const fs::path upper {hostDir("upper")};
	writeHostFile(lower / "a.txt", "lower");
	writeHostFile(upper / "a.txt", "upper");
	const Mount lowerMount {mount("/", lower, Access::ReadOnly)};
	const Mount upperMount {mount("/", upper, Access::ReadWrite)};

	EXPECT_EQ(m_fileSystem.root().remove("a.txt"), uqstl::errc::permission_denied);
	EXPECT_TRUE(fs::exists(upper / "a.txt"));
	EXPECT_EQ(readText(m_fileSystem.root(), "a.txt"), "upper");
}

TEST_F(IoTest, RenameDoesNotReplaceAFileAndRemoveKeepsDirectories)
{
	const fs::path host {hostDir("rw")};
	writeHostFile(host / "a.txt", "a");
	writeHostFile(host / "b.txt", "b");
	fs::create_directories(host / "dir");
	const Mount rw {mount("/", host, Access::ReadWrite)};
	const Directory root {m_fileSystem.root()};

	EXPECT_EQ(root.rename("a.txt", root, "b.txt"), uqstl::errc::file_exists);
	EXPECT_EQ(readHostFile(host / "b.txt"), "b");
	EXPECT_EQ(root.remove("dir"), uqstl::errc::is_a_directory);
	EXPECT_TRUE(fs::is_directory(host / "dir"));
}

TEST_F(IoTest, AppendModes)
{
	const fs::path host {hostDir("rw")};
	writeHostFile(host / "old.txt", "abc");
	const Mount rw {mount("/", host, Access::ReadWrite)};
	const Directory root {m_fileSystem.root()};
	uqstl::error_code error {};

	{
		uqstl::optional<File> file {root.open("new.txt", OpenMode::Read | OpenMode::Write | OpenMode::Create | OpenMode::Append, error)};
		ASSERT_TRUE(file.has_value()) << error.message();
		writeText(*file, "x");
	}
	EXPECT_EQ(readHostFile(host / "new.txt"), "x");
	{
		uqstl::optional<File> file {root.open("old.txt", OpenMode::Write | OpenMode::Append, error)};
		ASSERT_TRUE(file.has_value()) << error.message();
		writeText(*file, "d");
	}
	EXPECT_EQ(readHostFile(host / "old.txt"), "abcd");
	{
		uqstl::optional<File> file {root.open("old.txt", OpenMode::Write | OpenMode::Append | OpenMode::Truncate, error)};
		ASSERT_TRUE(file.has_value()) << error.message();
		writeText(*file, "e");
	}
	EXPECT_EQ(readHostFile(host / "old.txt"), "e");
}

TEST_F(IoTest, TextModeTellIsTheFilePosition)
{
	const fs::path host {hostDir("rw")};
	writeHostFile(host / "lines.txt", "a\r\nb\r\n");
	const Mount rw {mount("/", host, Access::ReadOnly)};

	uqstl::error_code error {};
	uqstl::optional<File> file {m_fileSystem.root().open("lines.txt", OpenMode::Read | OpenMode::Text, error)};
	ASSERT_TRUE(file.has_value()) << error.message();
	uqstl::array<char, 16> line {};
	ASSERT_TRUE(file->readLine(line));
	EXPECT_EQ(file->tell(), 3u);
	ASSERT_TRUE(file->seek(0, SeekFrom::Current));
	EXPECT_EQ(file->getChar(), 'b');
}

TEST_F(IoTest, HandlesCanOutliveTheFileSystem)
{
	const fs::path host {hostDir("rw")};
	writeHostFile(host / "a.txt", "a");

	uqstl::optional<Directory> directory {};
	Mount mount {};
	{
		FileSystem fileSystem {};
		uqstl::error_code error {};
		mount = fileSystem.mountDirectory("/", host, Access::ReadOnly, MountPosition::Top, nullptr, error).value();
		directory = fileSystem.root();
	}
	EXPECT_EQ(readText(*directory, "a.txt"), "a");
	mount.unmount();
	EXPECT_FALSE(directory->exists("a.txt"));

	directory.reset();
	EXPECT_FALSE(mount.isMounted());
}

TEST_F(IoTest, FileCharacterAccessAndMove)
{
	const fs::path host {hostDir("rw")};
	writeHostFile(host / "a.txt", "xy");
	const Mount rw {mount("/", host, Access::ReadOnly)};

	uqstl::error_code error {};
	uqstl::optional<File> file {m_fileSystem.root().open("a.txt", OpenMode::Read, error)};
	ASSERT_TRUE(file.has_value());
	EXPECT_EQ(file->getChar(), 'x');
	EXPECT_TRUE(file->unget());
	EXPECT_EQ(file->tell(), 0u);
	EXPECT_EQ(file->getChar(), 'x');
	EXPECT_EQ(file->getChar(), 'y');
	EXPECT_EQ(file->getChar(), File::c_endOfFile);
	EXPECT_TRUE(file->eof());

	ASSERT_TRUE(file->seek(0, SeekFrom::Start));
	uqstl::array<char, 2> line {};
	EXPECT_TRUE(file->readLine(line));
	EXPECT_STREQ(line.data(), "x");
	EXPECT_TRUE(file->readLine(line));
	EXPECT_STREQ(line.data(), "y");
	EXPECT_FALSE(file->readLine(line));

	File moved {uqstl::move(*file)};
	EXPECT_TRUE(moved.isOpen());
	EXPECT_FALSE(file->isOpen());
	moved.close();
	EXPECT_FALSE(moved.isOpen());
}

class IoZipTest : public IoTest
{
protected:
	void SetUp() override
	{
		IoTest::SetUp();
		const fs::path host {hostDir("content")};
		fs::copy_file(fs::path {UQM_TEST_DATA_DIR} / "io" / "sample.zip", host / "packages" / "sample.uqm", fs::copy_options::none);
		m_content = mount("/", host, Access::ReadOnly);

		const uqstl::optional<Directory> packages {m_fileSystem.root().subdir("packages")};
		ASSERT_TRUE(packages.has_value());
		uqstl::error_code error {};
		uqstl::optional<Mount> zip {m_fileSystem.mountArchive("/", *packages, "sample.uqm", MountPosition::Below, &m_content, error)};
		ASSERT_TRUE(zip.has_value()) << error.message();
		m_zip = uqstl::move(*zip);
	}

	fs::path hostDir(uqstl::string_view name)
	{
		const fs::path dir {IoTest::hostDir(name)};
		fs::create_directories(dir / "packages");
		return dir;
	}

	static uqstl::string deflatedContents()
	{
		uqstl::string lines {};
		for (int i {0}; i < 5000; ++i)
		{
			lines += "line " + uqstl::to_string(i) + "\n";
		}
		return lines;
	}

	Mount m_content {};
	Mount m_zip {};
};

TEST_F(IoZipTest, ReadsStoredAndDeflatedEntries)
{
	const Directory root {m_fileSystem.root()};
	EXPECT_EQ(readText(root, "stored.txt"), "stored contents\n");
	EXPECT_EQ(readText(root, "deflated.txt"), deflatedContents());
	EXPECT_EQ(readText(root, "dir/nested.txt"), "nested\n");
	EXPECT_EQ(root.stat("deflated.txt").value().size, deflatedContents().size());
	EXPECT_TRUE(root.stat("emptydir").value().isDirectory());
}

TEST_F(IoZipTest, ListsEntries)
{
	uqstl::error_code error {};
	EXPECT_EQ(m_fileSystem.root().list("", Matcher {}, error), (uqstl::vector<uqstl::string> {"deflated.txt", "dir", "emptydir", "packages", "stored.txt"}));
	EXPECT_EQ(m_fileSystem.root().list("dir", Matcher {}, error), (uqstl::vector<uqstl::string> {"nested.txt"}));
}

TEST_F(IoZipTest, SeeksInDeflatedEntry)
{
	const uqstl::string expected {deflatedContents()};
	uqstl::error_code error {};
	uqstl::optional<File> file {m_fileSystem.root().open("deflated.txt", OpenMode::Read, error)};
	ASSERT_TRUE(file.has_value()) << error.message();

	uqstl::array<uqstl::byte, 8> buffer {};
	const auto readAt {[&](int64_t offset, SeekFrom from) {
		EXPECT_TRUE(file->seek(offset, from));
		const size_t count {file->read(buffer)};
		uqstl::string text {};
		for (size_t i {0}; i < count; ++i)
		{
			text += uqstl::to_integer<char>(buffer[i]);
		}
		return text;
	}};

	EXPECT_EQ(readAt(30000, SeekFrom::Start), expected.substr(30000, 8));
	EXPECT_EQ(readAt(100, SeekFrom::Start), expected.substr(100, 8));
	EXPECT_EQ(readAt(-8, SeekFrom::End), expected.substr(expected.size() - 8));
	EXPECT_EQ(file->tell(), expected.size());
}

TEST_F(IoZipTest, ZipIsReadOnly)
{
	uqstl::error_code error {};
	EXPECT_FALSE(m_fileSystem.root().open("stored.txt", OpenMode::Write, error).has_value());
}
