// Reads the credits file: the blocks of text that the credits show.
#include "creditsfile.h"

#include "core/log/log.h"
#include "libs/file/json.h"


namespace uqm
{
namespace
{

uqstl::optional<CreditsColumn> readColumn(yyjson_val* obj)
{
	const auto align {enumFromJsonName<CreditsAlign>(jsonString(yyjson_obj_get(obj, "align")))};
	if (!align)
	{
		return uqstl::nullopt;
	}
	CreditsColumn column {*align};
	if (yyjson_val* const x {yyjson_obj_get(obj, "x")})
	{
		column.x = jsonInt<int32_t>(x, INT16_MIN, INT16_MAX);
		if (!column.x)
		{
			return uqstl::nullopt;
		}
	}
	return column;
}

// A line is a string, or an array of the strings of its cells.
uqstl::optional<uqstl::vector<uqstl::string>> readLine(yyjson_val* line)
{
	if (yyjson_is_str(line))
	{
		return uqstl::vector<uqstl::string> {jsonString(line)};
	}
	auto cells {jsonStringArray(line, c_creditsMaxColumns)};
	return cells && !cells->empty() ? cells : uqstl::nullopt;
}

uqstl::optional<CreditsBlock> readBlock(yyjson_val* obj)
{
	CreditsBlock block {};
	const auto size {jsonInt<int32_t>(yyjson_obj_get(obj, "size"), 1)};
	yyjson_val* const columns {yyjson_obj_get(obj, "columns")};
	yyjson_val* const lines {yyjson_obj_get(obj, "lines")};
	const size_t columnCount {yyjson_arr_size(columns)};
	const size_t lineCount {yyjson_arr_size(lines)};
	if (!size || !yyjson_is_arr(columns) || columnCount == 0 || columnCount > c_creditsMaxColumns || !yyjson_is_arr(lines)
		|| lineCount == 0 || lineCount > c_creditsMaxLines)
	{
		return uqstl::nullopt;
	}
	block.size = *size;

	size_t index {};
	size_t max {};
	yyjson_val* value {};
	yyjson_arr_foreach(columns, index, max, value)
	{
		auto column {readColumn(value)};
		if (!column)
		{
			return uqstl::nullopt;
		}
		block.columns.push_back(*column);
	}
	yyjson_arr_foreach(lines, index, max, value)
	{
		auto line {readLine(value)};
		if (!line || line->size() > block.columns.size())
		{
			return uqstl::nullopt;
		}
		block.lines.push_back(uqstl::move(*line));
	}
	return block;
}

} // namespace

uqstl::optional<uqstl::vector<CreditsBlock>> creditsFromJson(uqstl::string json)
{
	static constexpr uqstl::string_view c_what {"Credits"};
	const JsonText doc {uqstl::move(json), c_what};
	yyjson_val* const blocks {versionedMember(doc, c_creditsFileVersion, "blocks", YYJSON_TYPE_ARR, c_what)};
	if (blocks == nullptr)
	{
		return uqstl::nullopt;
	}
	uqstl::vector<CreditsBlock> result {};
	size_t index {};
	size_t max {};
	yyjson_val* obj {};
	yyjson_arr_foreach(blocks, index, max, obj)
	{
		auto block {readBlock(obj)};
		if (!block)
		{
			uqm::log::error("{}: block {} is not valid.", c_what, index);
			return uqstl::nullopt;
		}
		result.push_back(uqstl::move(*block));
	}
	return result;
}

} // namespace uqm
