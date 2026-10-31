// Reads JSON data files with yyjson.
#include "json.h"

#include "core/log/log.h"

namespace uqm
{

JsonDoc parseJson(uqstl::string_view text, uqstl::string_view what, yyjson_read_flag flags)
{
	yyjson_read_err err {};
	JsonDoc doc {yyjson_read_opts(const_cast<char*>(text.data()), text.size(), flags, nullptr, &err), &yyjson_doc_free};
	if (!doc)
	{
		uqm::log::error("{}: JSON parse error at byte {}: {}", what, err.pos, err.msg);
	}
	return doc;
}

yyjson_val* versionedRoot(const JsonDoc& doc, int64_t version, uqstl::string_view what)
{
	if (!doc)
	{
		return nullptr;
	}
	yyjson_val* const root {yyjson_doc_get_root(doc.get())};
	if (yyjson_get_sint(yyjson_obj_get(root, "version")) != version)
	{
		uqm::log::error("{}: the file does not have the expected version.", what);
		return nullptr;
	}
	return root;
}

yyjson_val* versionedMember(const JsonDoc& doc, int64_t version, const char* key, yyjson_type type, uqstl::string_view what)
{
	yyjson_val* const root {versionedRoot(doc, version, what)};
	if (root == nullptr)
	{
		return nullptr;
	}
	yyjson_val* const value {yyjson_obj_get(root, key)};
	if (yyjson_get_type(value) != type)
	{
		uqm::log::error("{}: the file does not have the expected structure.", what);
		return nullptr;
	}
	return value;
}

uqstl::optional<uqstl::vector<uqstl::string>> jsonStringArray(yyjson_val* value, size_t maxCount)
{
	if (!yyjson_is_arr(value) || yyjson_arr_size(value) > maxCount)
	{
		return uqstl::nullopt;
	}
	uqstl::vector<uqstl::string> strings {};
	size_t index {};
	size_t max {};
	yyjson_val* item {};
	yyjson_arr_foreach(value, index, max, item)
	{
		if (!yyjson_is_str(item))
		{
			return uqstl::nullopt;
		}
		strings.push_back(jsonString(item));
	}
	return strings;
}

} // namespace uqm
