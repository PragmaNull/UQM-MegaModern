// Read-only mount source for a zip archive.
#pragma once
#ifndef UQM_LIBS_IO_STANDARD_ZIPSOURCE_H_
#define UQM_LIBS_IO_STANDARD_ZIPSOURCE_H_

#include "core/stl/map.h"
#include "libs/io/standard/MountSource.h"

namespace uqm::io::standard
{

class ZipSource final : public MountSource
{
public:
	struct Entry
	{
		uint64_t headerOffset {0};
		uint64_t compressedSize {0};
		uint64_t size {0};
		int64_t modifiedTime {0};
		bool deflated {false};
	};

	// Reads the central directory of the archive at 'path' in 'archive'. Returns nullptr and sets 'error' on failure.
	static uqstl::shared_ptr<ZipSource> create(uqstl::shared_ptr<const MountSource> archive, uqstl::string path, uqstl::error_code& error);

	[[nodiscard]] uqstl::optional<FileInfo> stat(uqstl::string_view path) const override;
	[[nodiscard]] uqstl::vector<uqstl::string> list(uqstl::string_view path, uqstl::error_code& error) const override;
	[[nodiscard]] uqstl::unique_ptr<Stream> open(uqstl::string_view path, OpenMode mode, uqstl::error_code& error) const override;

	ZipSource(uqstl::shared_ptr<const MountSource> archive, uqstl::string path);

private:
	bool readCentralDirectory(uqstl::error_code& error);
	void addEntry(uqstl::string name, bool isDirectory, const Entry& entry);
	void addDirectory(const uqstl::string& name);
	// Adds 'name' to the list of its parent directory, which is created if needed. Returns false if the parent is a file.
	bool linkToParent(const uqstl::string& name);

	uqstl::shared_ptr<const MountSource> m_archive {};
	uqstl::string m_path {};
	uqstl::map<uqstl::string, Entry, uqstl::less<>> m_files {};
	// The names in each directory, by directory path. The root is "".
	uqstl::map<uqstl::string, uqstl::vector<uqstl::string>, uqstl::less<>> m_directories {};
};

} // namespace uqm::io::standard

#endif // UQM_LIBS_IO_STANDARD_ZIPSOURCE_H_
