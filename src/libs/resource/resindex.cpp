// Reads resource index files: the JSON files that give the type and the data of each resource.
#include "resindex.h"

#include "core/log/log.h"
#include "core/stl/charconv.h"
#include "core/stl/ranges.h"
#include "core/string/StringUtils.h"

#include <cstdlib>

#include <yyjson.h>

const char* resourceString(const ResourceArg& arg)
{
	const uqstl::string* const text {uqstl::get_if<uqstl::string>(&arg)};
	return text != nullptr ? text->c_str() : nullptr;
}

namespace
{

const ResourceScalar* findField(const ResourceArg& arg, uqstl::string_view name)
{
	const ResourceFields* const fields {uqstl::get_if<ResourceFields>(&arg)};
	if (fields == nullptr)
	{
		return nullptr;
	}
	const auto it {fields->find(name)};
	return it != fields->end() ? &it->second : nullptr;
}

uqstl::optional<ResourceScalar> readScalar(yyjson_val* value)
{
	if (yyjson_is_str(value))
	{
		return uqstl::string {yyjson_get_str(value), yyjson_get_len(value)};
	}
	if (yyjson_is_int(value))
	{
		return yyjson_get_int(value);
	}
	if (yyjson_is_real(value))
	{
		return yyjson_get_real(value);
	}
	if (yyjson_is_bool(value))
	{
		return yyjson_get_bool(value);
	}
	return uqstl::nullopt;
}

uqstl::optional<ResourceArg> readArg(yyjson_val* data)
{
	if (const auto scalar {readScalar(data)})
	{
		return uqstl::visit([](const auto& value) {
			return ResourceArg {value};
		},
							*scalar);
	}
	if (!yyjson_is_obj(data))
	{
		return uqstl::nullopt;
	}
	ResourceFields fields {};
	size_t index {};
	size_t max {};
	yyjson_val* name {};
	yyjson_val* value {};
	yyjson_obj_foreach(data, index, max, name, value)
	{
		auto scalar {readScalar(value)};
		if (!scalar)
		{
			return uqstl::nullopt;
		}
		fields.emplace(uqstl::string {yyjson_get_str(name), yyjson_get_len(name)}, uqstl::move(*scalar));
	}
	return ResourceArg {uqstl::move(fields)};
}

} // namespace

const char* resourceStringField(const ResourceArg& arg, uqstl::string_view name)
{
	const ResourceScalar* const field {findField(arg, name)};
	const uqstl::string* const text {field != nullptr ? uqstl::get_if<uqstl::string>(field) : nullptr};
	return text != nullptr ? text->c_str() : nullptr;
}

uqstl::optional<int64_t> resourceIntField(const ResourceArg& arg, uqstl::string_view name)
{
	const ResourceScalar* const field {findField(arg, name)};
	const int64_t* const number {field != nullptr ? uqstl::get_if<int64_t>(field) : nullptr};
	return number != nullptr ? uqstl::optional<int64_t> {*number} : uqstl::nullopt;
}

namespace uqm
{

uqstl::optional<uqstl::vector<ResourceEntry>> resourceIndexFromJson(uqstl::string_view json, uqstl::string_view prefix)
{
	yyjson_read_err err {};
	yyjson_doc* doc {yyjson_read_opts(const_cast<char*>(json.data()), json.size(), YYJSON_READ_NOFLAG, nullptr, &err)};
	if (doc == nullptr)
	{
		uqm::log::error("Resource index: JSON parse error at byte {}: {}", err.pos, err.msg);
		return uqstl::nullopt;
	}

	uqstl::optional<uqstl::vector<ResourceEntry>> result {};
	yyjson_val* const root {yyjson_doc_get_root(doc)};
	yyjson_val* const resources {yyjson_obj_get(root, "resources")};
	if (yyjson_get_int(yyjson_obj_get(root, "version")) != c_resourceIndexVersion || !yyjson_is_obj(resources))
	{
		uqm::log::error("Resource index: the file does not have the expected structure.");
	}
	else
	{
		result.emplace();
		size_t index {};
		size_t max {};
		yyjson_val* key {};
		yyjson_val* entry {};
		yyjson_obj_foreach(resources, index, max, key, entry)
		{
			const uqstl::string_view keyText {yyjson_get_str(key), yyjson_get_len(key)};
			yyjson_val* const type {yyjson_obj_get(entry, "type")};
			auto arg {readArg(yyjson_obj_get(entry, "data"))};
			if (!yyjson_is_str(type) || !arg)
			{
				uqm::log::warn("Resource index: '{}' does not have a valid type and data.", keyText);
				continue;
			}
			result->push_back({
				uqstl::string {prefix}
					+ uqstl::string {keyText},
				uqstl::string {yyjson_get_str(type), yyjson_get_len(type)},
				uqstl::move(*arg)
			 });
		}
	}
	yyjson_doc_free(doc);
	return result;
}

namespace
{

// Returns 0 if 'text' does not start with a number.
template <typename T>
T parseNumber(uqstl::string_view text)
{
	text = trim(text);
	T value {};
	uqstl::from_chars(text.data(), text.data() + text.size(), value);
	return value;
}

} // namespace

uqstl::vector<ResourceEntry> resourceIndexFromLegacyText(uqstl::string_view text)
{
	uqstl::vector<ResourceEntry> entries {};
	for (const auto lineRange : uqstl::views::split(text, '\n'))
	{
		uqstl::string_view line {lineRange.begin(), lineRange.end()};
		line = line.substr(0, line.find('#'));
		const size_t equals {line.find('=')};
		if (trim(line).empty())
		{
			continue;
		}
		if (equals == uqstl::string_view::npos)
		{
			uqm::log::warn("Warning: Key without value");
			continue;
		}
		const uqstl::string_view key {trim(line.substr(0, equals))};
		const uqstl::string_view value {trim(line.substr(equals + 1))};
		const size_t colon {value.find(':')};
		if (colon == uqstl::string_view::npos)
		{
			uqm::log::warn("Could not find type information for resource '{}'", key);
			entries.push_back({uqstl::string {key}, "UNKNOWNRES", uqstl::string {value}});
			continue;
		}

		const uqstl::string_view type {value.substr(0, colon)};
		const uqstl::string_view data {value.substr(colon + 1)};
		ResourceArg arg {uqstl::string {data}};
		if (type == "INT32" || type == "SHIP")
		{
			arg = parseNumber<int64_t>(data);
		}
		else if (type == "FLT32")
		{
			arg = parseNumber<double>(data);
		}
		else if (type == "bool")
		{
			arg = compareICase(data, "true");
		}
		entries.push_back({uqstl::string {key}, uqstl::string {type}, uqstl::move(arg)});
	}
	return entries;
}

namespace
{

yyjson_mut_val* scalarToJson(yyjson_mut_doc* doc, const ResourceScalar& value)
{
	return uqstl::visit(
		[doc](const auto& v) -> yyjson_mut_val* {
			using T = uqstl::decay_t<decltype(v)>;
			if constexpr (uqstl::is_same_v<T, uqstl::string>)
			{
				return yyjson_mut_strncpy(doc, v.data(), v.size());
			}
			else if constexpr (uqstl::is_same_v<T, int64_t>)
			{
				return yyjson_mut_sint(doc, v);
			}
			else if constexpr (uqstl::is_same_v<T, double>)
			{
				return yyjson_mut_real(doc, v);
			}
			else
			{
				return yyjson_mut_bool(doc, v);
			}
		},
		value);
}

yyjson_mut_val* argToJson(yyjson_mut_doc* doc, const ResourceArg& arg)
{
	return uqstl::visit(
		[doc](const auto& v) -> yyjson_mut_val* {
			if constexpr (uqstl::is_same_v<uqstl::decay_t<decltype(v)>, ResourceFields>)
			{
				yyjson_mut_val* const obj {yyjson_mut_obj(doc)};
				for (const auto& [name, value] : v)
				{
					yyjson_mut_obj_add(obj, yyjson_mut_strncpy(doc, name.data(), name.size()), scalarToJson(doc, value));
				}
				return obj;
			}
			else
			{
				return scalarToJson(doc, ResourceScalar {v});
			}
		},
		arg);
}

} // namespace

uqstl::string resourceIndexToJson(uqstl::span<const ResourceEntry> entries)
{
	yyjson_mut_doc* const doc {yyjson_mut_doc_new(nullptr)};
	yyjson_mut_val* const root {yyjson_mut_obj(doc)};
	yyjson_mut_doc_set_root(doc, root);
	yyjson_mut_obj_add_sint(doc, root, "version", c_resourceIndexVersion);
	yyjson_mut_val* const resources {yyjson_mut_obj_add_obj(doc, root, "resources")};
	for (const ResourceEntry& entry : entries)
	{
		yyjson_mut_val* const obj {yyjson_mut_obj(doc)};
		yyjson_mut_obj_add_strncpy(doc, obj, "type", entry.type.data(), entry.type.size());
		yyjson_mut_obj_add_val(doc, obj, "data", argToJson(doc, entry.arg));
		yyjson_mut_obj_add(resources, yyjson_mut_strncpy(doc, entry.key.data(), entry.key.size()), obj);
	}
	size_t length {};
	char* const text {yyjson_mut_write(doc, YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_NEWLINE_AT_END, &length)};
	uqstl::string result {
		text != nullptr ? uqstl::string {text, length}
			   : uqstl::string {}
	 };
	free(text);
	yyjson_mut_doc_free(doc);
	return result;
}

} // namespace uqm
