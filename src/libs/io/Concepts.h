// Requirements that a file system back end must meet.
#pragma once
#ifndef UQM_LIBS_IO_CONCEPTS_H_
#define UQM_LIBS_IO_CONCEPTS_H_

#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "libs/io/Matcher.h"
#include "libs/io/Types.h"

#include <concepts>
#include <filesystem>
#include <system_error>

namespace uqm::io
{

template <typename F>
concept FileImpl = uqstl::movable<F> && !uqstl::copyable<F> && uqstl::default_initializable<F>
				&& requires(F file, const F constFile, uqstl::span<uqstl::byte> buffer, uqstl::span<const uqstl::byte> data, uqstl::span<char> chars, int64_t offset, SeekFrom from, size_t count) {
					   { constFile.isOpen() } -> uqstl::same_as<bool>;
					   { file.close() };
					   { file.read(buffer) } -> uqstl::same_as<size_t>;
					   { file.write(data) } -> uqstl::same_as<size_t>;
					   { file.readLine(chars) } -> uqstl::same_as<bool>;
					   { file.getChar() } -> uqstl::same_as<int>;
					   { file.unget(count) } -> uqstl::same_as<bool>;
					   { file.seek(offset, from) } -> uqstl::same_as<bool>;
					   { constFile.tell() } -> uqstl::same_as<uint64_t>;
					   { file.size() } -> uqstl::same_as<uint64_t>;
					   { file.flush() } -> uqstl::same_as<bool>;
					   { constFile.eof() } -> uqstl::same_as<bool>;
					   { constFile.error() } -> uqstl::same_as<uqstl::error_code>;
					   { file.clearError() };
				   };

template <typename D, typename F>
concept DirectoryImpl = uqstl::copyable<D>
					 && requires(const D directory, uqstl::string_view path, OpenMode mode, uqstl::error_code& error, const Matcher& matcher) {
							{ directory.open(path, mode, error) } -> uqstl::same_as<uqstl::optional<F>>;
							{ directory.stat(path) } -> uqstl::same_as<uqstl::optional<FileInfo>>;
							{ directory.exists(path) } -> uqstl::same_as<bool>;
							{ directory.list(path, matcher, error) } -> uqstl::same_as<uqstl::vector<uqstl::string>>;
							{ directory.subdir(path) } -> uqstl::same_as<uqstl::optional<D>>;
							{ directory.makeDir(path) } -> uqstl::same_as<uqstl::error_code>;
							{ directory.remove(path) } -> uqstl::same_as<uqstl::error_code>;
							{ directory.rename(path, directory, path) } -> uqstl::same_as<uqstl::error_code>;
							{ directory.path() } -> uqstl::convertible_to<uqstl::string_view>;
						};

template <typename M>
concept MountImpl = uqstl::movable<M> && !uqstl::copyable<M> && uqstl::default_initializable<M>
				 && requires(M mount, const M constMount) {
						{ constMount.isMounted() } -> uqstl::same_as<bool>;
						{ mount.unmount() };
					};

template <typename FS>
concept FileSystemImpl = FileImpl<typename FS::File> && DirectoryImpl<typename FS::Directory, typename FS::File> && MountImpl<typename FS::Mount>
					  && requires(FS fileSystem, uqstl::string_view path, const uqstl::filesystem::path& hostPath, const typename FS::Directory& directory, Access access, MountPosition position, const typename FS::Mount* relative, uqstl::error_code& error) {
							 { fileSystem.root() } -> uqstl::same_as<typename FS::Directory>;
							 { fileSystem.mountDirectory(path, hostPath, access, position, relative, error) } -> uqstl::same_as<uqstl::optional<typename FS::Mount>>;
							 { fileSystem.mountArchive(path, directory, path, position, relative, error) } -> uqstl::same_as<uqstl::optional<typename FS::Mount>>;
							 { fileSystem.mountTransplant(path, directory, access, position, relative, error) } -> uqstl::same_as<uqstl::optional<typename FS::Mount>>;
						 };

} // namespace uqm::io

#endif // UQM_LIBS_IO_CONCEPTS_H_
