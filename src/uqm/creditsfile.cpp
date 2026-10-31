// Reads the credits file: the blocks of text that the credits show.
#include "creditsfile.h"

#include "core/log/log.h"

#include <yyjson.h>

namespace uqm
{

namespace
{

uqstl::optional<CreditsColumn> readColumn(yyjson_val* obj)
{
	CreditsColumn column {};
	yyjson_val* const align {yyjson_obj_get(obj, "align")};
	if (yyjson_equals_str(align, "left"))
	{
		column.align = CreditsAlign::Left;
	}
	else if (yyjson_equals_str(align, "center"))
	{
		column.align = CreditsAlign::Center;
	}
	else if (yyjson_equals_str(align, "right"))
	{
		column.align = CreditsAlign::Right;
	}
	else
	{
		return uqstl::nullopt;
	}
	if (yyjson_val* const x {yyjson_obj_get(obj, "x")})
	{
		if (!yyjson_is_int(x) || yyjson_get_sint(x) < INT16_MIN || yyjson_get_sint(x) > INT16_MAX)
		{
			return uqstl::nullopt;
		}
		column.x = static_cast<int32_t>(yyjson_get_sint(x));
	}
	return column;
}

// A line is a string, or an array of the strings of its cells.
uqstl::optional<uqstl::vector<uqstl::string>> readLine(yyjson_val* line)
{
	uqstl::vector<uqstl::string> cells {};
	if (yyjson_is_str(line))
	{
		cells.emplace_back(yyjson_get_str(line), yyjson_get_len(line));
		return cells;
	}
	const size_t count {yyjson_arr_size(line)};
	if (!yyjson_is_arr(line) || count == 0 || count > c_creditsMaxColumns)
	{
		return uqstl::nullopt;
	}
	size_t index {};
	size_t max {};
	yyjson_val* cell {};
	yyjson_arr_foreach(line, index, max, cell)
	{
		if (!yyjson_is_str(cell))
		{
			return uqstl::nullopt;
		}
		cells.emplace_back(yyjson_get_str(cell), yyjson_get_len(cell));
	}
	return cells;
}

uqstl::optional<CreditsBlock> readBlock(yyjson_val* obj)
{
	CreditsBlock block {};
	yyjson_val* const size {yyjson_obj_get(obj, "size")};
	yyjson_val* const columns {yyjson_obj_get(obj, "columns")};
	yyjson_val* const lines {yyjson_obj_get(obj, "lines")};
	const size_t columnCount {yyjson_arr_size(columns)};
	const size_t lineCount {yyjson_arr_size(lines)};
	if (!yyjson_is_int(size) || yyjson_get_sint(size) <= 0 || yyjson_get_sint(size) > INT32_MAX || !yyjson_is_arr(columns)
		|| columnCount == 0 || columnCount > c_creditsMaxColumns || !yyjson_is_arr(lines) || lineCount == 0
		|| lineCount > c_creditsMaxLines)
	{
		return uqstl::nullopt;
	}
	block.size = static_cast<int32_t>(yyjson_get_sint(size));

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
		if (!line)
		{
			return uqstl::nullopt;
		}
		block.lines.push_back(uqstl::move(*line));
	}
	return block;
}

} // namespace

uqstl::optional<uqstl::vector<CreditsBlock>> creditsFromJson(uqstl::string_view json)
{
	yyjson_read_err err {};
	yyjson_doc* doc {yyjson_read_opts(const_cast<char*>(json.data()), json.size(), YYJSON_READ_NOFLAG, nullptr, &err)};
	if (doc == nullptr)
	{
		uqm::log::error("Credits: JSON parse error at byte {}: {}", err.pos, err.msg);
		return uqstl::nullopt;
	}

	uqstl::optional<uqstl::vector<CreditsBlock>> result {};
	yyjson_val* const root {yyjson_doc_get_root(doc)};
	yyjson_val* const blocks {yyjson_obj_get(root, "blocks")};
	if (yyjson_get_int(yyjson_obj_get(root, "version")) != c_creditsFileVersion || !yyjson_is_arr(blocks))
	{
		uqm::log::error("Credits: the file does not have the expected structure.");
	}
	else
	{
		result.emplace();
		size_t index {};
		size_t max {};
		yyjson_val* obj {};
		yyjson_arr_foreach(blocks, index, max, obj)
		{
			auto block {readBlock(obj)};
			if (!block)
			{
				uqm::log::error("Credits: block {} is not valid.", index);
				result.reset();
				break;
			}
			result->push_back(uqstl::move(*block));
		}
	}
	yyjson_doc_free(doc);
	return result;
}

} // namespace uqm
