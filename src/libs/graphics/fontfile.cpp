// Reads font metrics files: the JSON files that give the spacing and kerning of a font.
#include "fontfile.h"

#include "core/log/log.h"
#include "libs/file/json.h"

namespace uqm
{

namespace
{

template <typename T>
bool readNumber(yyjson_val* obj, const char* name, T& out)
{
	const auto value {jsonInt<T>(yyjson_obj_get(obj, name))};
	out = value.value_or(T {});
	return value.has_value();
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
	static constexpr uqstl::string_view c_what {"Font metrics"};
	const JsonDoc doc {parseJson(json, c_what)};
	yyjson_val* const root {versionedRoot(doc, c_fontMetricsVersion, c_what)};
	if (root == nullptr)
	{
		return uqstl::nullopt;
	}
	FontMetrics result {};
	yyjson_val* const kerning {yyjson_obj_get(root, "kerning")};
	if (!readNumber(root, "leading", result.leading) || !readNumber(root, "charSpace", result.charSpace)
		|| !readNumber(root, "kernAmount", result.kernAmount) || !readNumber(root, "vertAlign", result.vertAlign)
		|| (kerning != nullptr && !yyjson_is_obj(kerning)))
	{
		uqm::log::error("{}: the file does not have the expected structure.", c_what);
		return uqstl::nullopt;
	}
	size_t index {};
	size_t max {};
	yyjson_val* key {};
	yyjson_val* value {};
	yyjson_obj_foreach(kerning, index, max, key, value)
	{
		FontKern kern {};
		if (!readKern(key, value, kern))
		{
			uqm::log::error("{}: the kerning of '{}' is not valid.", c_what, yyjson_get_str(key));
			return uqstl::nullopt;
		}
		result.kerning.push_back(kern);
	}
	return result;
}

} // namespace uqm
