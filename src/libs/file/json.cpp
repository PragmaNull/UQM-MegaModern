// Reads JSON data files with yyjson.
#include "json.h"

#include "core/log/log.h"

namespace uqm
{

JsonText::JsonText(uqstl::string text, uqstl::string_view what, yyjson_read_flag flags)
	: m_text {uqstl::make_unique<uqstl::string>(uqstl::move(text))}
{
	const size_t length {m_text->size()};
	m_text->append(JsonPadding, '\0');
	yyjson_read_err err {};
	m_doc.reset(yyjson_read_opts(m_text->data(), length, flags | YYJSON_READ_INSITU, nullptr, &err));
	if (!m_doc)
	{
		uqm::log::error("{}: JSON parse error at byte {}: {}", what, err.pos, err.msg);
	}
}

uqstl::optional<uqstl::string> writeJson(const JsonMutDoc& doc, yyjson_write_flag flags)
{
	size_t length {};
	const uqstl::unique_ptr<char, decltype(&free)> text {yyjson_mut_write(doc.get(), flags, &length), &free};
	if (!text)
	{
		return uqstl::nullopt;
	}
	return uqstl::string {text.get(), length};
}

yyjson_val* versionedRoot(const JsonText& doc, int64_t version, uqstl::string_view what)
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

yyjson_val* versionedMember(const JsonText& doc, int64_t version, const char* key, yyjson_type type, uqstl::string_view what)
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
