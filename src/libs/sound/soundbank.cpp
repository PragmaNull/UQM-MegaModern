// Reads sound bank files: the JSON files that list the sounds of a sound resource.
#include "soundbank.h"

#include "core/log/log.h"
#include "libs/file/json.h"

namespace uqm
{

uqstl::optional<uqstl::vector<uqstl::string>> soundBankFromJson(uqstl::string_view json)
{
	static constexpr uqstl::string_view c_what {"Sound bank"};
	const JsonDoc doc {parseJson(json, c_what)};
	yyjson_val* const sounds {versionedMember(doc, c_soundBankVersion, "sounds", YYJSON_TYPE_ARR, c_what)};
	if (sounds == nullptr)
	{
		return uqstl::nullopt;
	}
	auto result {jsonStringArray(sounds)};
	if (!result)
	{
		uqm::log::error("{}: the sounds are not all strings.", c_what);
	}
	return result;
}

} // namespace uqm
