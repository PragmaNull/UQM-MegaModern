// Reads sound bank files: the JSON files that list the sounds of a sound resource.
#include "soundbank.h"

#include "core/log/log.h"

#include <yyjson.h>

namespace uqm
{

uqstl::optional<uqstl::vector<uqstl::string>> soundBankFromJson(uqstl::string_view json)
{
	yyjson_read_err err {};
	yyjson_doc* doc {yyjson_read_opts(const_cast<char*>(json.data()), json.size(), YYJSON_READ_NOFLAG, nullptr, &err)};
	if (doc == nullptr)
	{
		uqm::log::error("Sound bank: JSON parse error at byte {}: {}", err.pos, err.msg);
		return uqstl::nullopt;
	}

	uqstl::optional<uqstl::vector<uqstl::string>> result {};
	yyjson_val* const root {yyjson_doc_get_root(doc)};
	yyjson_val* const sounds {yyjson_obj_get(root, "sounds")};
	if (yyjson_get_int(yyjson_obj_get(root, "version")) == c_soundBankVersion && yyjson_is_arr(sounds))
	{
		result.emplace();
		size_t index {};
		size_t max {};
		yyjson_val* sound {};
		yyjson_arr_foreach(sounds, index, max, sound)
		{
			if (!yyjson_is_str(sound))
			{
				result.reset();
				break;
			}
			result->emplace_back(yyjson_get_str(sound), yyjson_get_len(sound));
		}
	}
	if (!result)
	{
		uqm::log::error("Sound bank: the file does not have the expected structure.");
	}
	yyjson_doc_free(doc);
	return result;
}

} // namespace uqm
