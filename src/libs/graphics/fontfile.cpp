// Reads font metrics files: the JSON files that give the spacing and kerning of a font.
#include "fontfile.h"

#include "core/log/log.h"
#include "core/stl/limits.h"

#include <yyjson.h>

namespace uqm
{

namespace
{

template <typename T>
bool readNumber(yyjson_val* obj, const char* name, T& out)
{
	yyjson_val* const value {yyjson_obj_get(obj, name)};
	if (!yyjson_is_int(value) || yyjson_get_sint(value) < uqstl::numeric_limits<T>::min()
		|| yyjson_get_sint(value) > uqstl::numeric_limits<T>::max())
	{
		return false;
	}
	out = static_cast<T>(yyjson_get_sint(value));
	return true;
}

bool readKern(yyjson_val* key, yyjson_val* value, FontKern& kern)
{
	const char* text {yyjson_get_str(key)};
	const UniChar ch {getCharFromString(&text)};
	yyjson_val* const left {yyjson_arr_get(value, 0)};
	yyjson_val* const right {yyjson_arr_get(value, 1)};
	if (ch == 0 || *text != '\0' || yyjson_arr_size(value) != 2 || !yyjson_is_uint(left) || !yyjson_is_uint(right)
		|| yyjson_get_uint(left) > c_fontMaxKern || yyjson_get_uint(right) > c_fontMaxKern)
	{
		return false;
	}
	kern = {ch, static_cast<uint8_t>(yyjson_get_uint(left)), static_cast<uint8_t>(yyjson_get_uint(right))};
	return true;
}

} // namespace

uqstl::optional<FontMetrics> fontMetricsFromJson(uqstl::string_view json)
{
	yyjson_read_err err {};
	yyjson_doc* doc {yyjson_read_opts(const_cast<char*>(json.data()), json.size(), YYJSON_READ_NOFLAG, nullptr, &err)};
	if (doc == nullptr)
	{
		uqm::log::error("Font metrics: JSON parse error at byte {}: {}", err.pos, err.msg);
		return uqstl::nullopt;
	}

	uqstl::optional<FontMetrics> result {FontMetrics {}};
	yyjson_val* const root {yyjson_doc_get_root(doc)};
	yyjson_val* const kerning {yyjson_obj_get(root, "kerning")};
	if (yyjson_get_int(yyjson_obj_get(root, "version")) != c_fontMetricsVersion || !readNumber(root, "leading", result->leading)
		|| !readNumber(root, "charSpace", result->charSpace) || !readNumber(root, "kernAmount", result->kernAmount)
		|| !readNumber(root, "vertAlign", result->vertAlign) || (kerning != nullptr && !yyjson_is_obj(kerning)))
	{
		result.reset();
	}
	else
	{
		size_t index {};
		size_t max {};
		yyjson_val* key {};
		yyjson_val* value {};
		yyjson_obj_foreach(kerning, index, max, key, value)
		{
			FontKern kern {};
			if (!readKern(key, value, kern))
			{
				result.reset();
				break;
			}
			result->kerning.push_back(kern);
		}
	}
	if (!result)
	{
		uqm::log::error("Font metrics: the file does not have the expected structure.");
	}
	yyjson_doc_free(doc);
	return result;
}

} // namespace uqm
