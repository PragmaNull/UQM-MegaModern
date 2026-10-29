// Mount source for a directory on the host file system.
#pragma once
#ifndef UQM_LIBS_IO_STANDARD_DIRECTORYSOURCE_H_
#define UQM_LIBS_IO_STANDARD_DIRECTORYSOURCE_H_

#include "libs/io/standard/MountSource.h"

#include <filesystem>

namespace uqm::io::standard
{

class DirectorySource final : public MountSource
{
public:
	explicit DirectorySource(uqstl::filesystem::path root);

	[[nodiscard]] uqstl::optional<FileInfo> stat(uqstl::string_view path) const override;
	[[nodiscard]] uqstl::vector<uqstl::string> list(uqstl::string_view path, uqstl::error_code& error) const override;
	[[nodiscard]] uqstl::unique_ptr<Stream> open(uqstl::string_view path, OpenMode mode, uqstl::error_code& error) const override;

	[[nodiscard]] bool canWrite() const override { return true; }
	uqstl::error_code makeDir(uqstl::string_view path) const override;
	uqstl::error_code remove(uqstl::string_view path) const override;
	uqstl::error_code rename(uqstl::string_view from, uqstl::string_view to) const override;

private:
	[[nodiscard]] uqstl::filesystem::path hostPath(uqstl::string_view path) const;

	uqstl::filesystem::path m_root {};
};

} // namespace uqm::io::standard

#endif // UQM_LIBS_IO_STANDARD_DIRECTORYSOURCE_H_
