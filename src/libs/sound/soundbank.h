// Reads sound bank files: the JSON files that list the sounds of a sound resource.

#pragma once
#ifndef UQM_LIBS_SOUND_SOUNDBANK_H
#define UQM_LIBS_SOUND_SOUNDBANK_H

#include "core/stl/stl.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"

namespace uqm
{

inline constexpr int c_soundBankVersion {1};

// Returns the sound files, relative to the directory of the bank, or nullopt if 'json' is not a valid sound bank.
[[nodiscard]] uqstl::optional<uqstl::vector<uqstl::string>> soundBankFromJson(uqstl::string_view json);

} // namespace uqm

#endif // UQM_LIBS_SOUND_SOUNDBANK_H
