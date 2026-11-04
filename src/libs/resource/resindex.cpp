// Reads resource index files: the JSON files that give the type and the data of each resource.
#include "resindex.h"

#include "core/log/log.h"
#include "core/stl/charconv.h"
#include "core/stl/ranges.h"
#include "core/string/StringUtils.h"

#include "libs/file/json.h"

#include <cstdlib>

const char* resourceString(const ResourceArg& arg)
{
	const uqstl::string* const text {uqstl::get_if<uqstl::string>(&arg)};
	return text != nullptr ? text->c_str() : nullptr;
}

namespace
{

template <typename T>
const T* findField(const ResourceArg& arg, uqstl::string_view name)
{
	const ResourceFields* const fields {uqstl::get_if<ResourceFields>(&arg)};
	if (fields == nullptr)
	{
		return nullptr;
	}
	const auto it {fields->find(name)};
	return it != fields->end() ? uqstl::get_if<T>(&it->second) : nullptr;
}

// Returns a string, number or truth value as a 'V', or nullopt for other values.
template <typename V>
uqstl::optional<V> readScalar(yyjson_val* value)
{
	if (yyjson_is_str(value))
	{
		return V {uqm::jsonString(value)};
	}
	if (const auto number {uqm::jsonInt<int64_t>(value)})
	{
		return V {*number};
	}
	if (yyjson_is_real(value))
	{
		return V {yyjson_get_real(value)};
	}
	if (yyjson_is_bool(value))
	{
		return V {yyjson_get_bool(value)};
	}
	return uqstl::nullopt;
}

uqstl::optional<ResourceArg> readArg(yyjson_val* data)
{
	if (auto scalar {readScalar<ResourceArg>(data)})
	{
		return scalar;
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
		auto scalar {readScalar<ResourceScalar>(value)};
		if (!scalar)
		{
			return uqstl::nullopt;
		}
		fields.emplace(uqm::jsonString(name), uqstl::move(*scalar));
	}
	return ResourceArg {uqstl::move(fields)};
}

} // namespace

const char* resourceStringField(const ResourceArg& arg, uqstl::string_view name)
{
	const uqstl::string* const text {findField<uqstl::string>(arg, name)};
	return text != nullptr ? text->c_str() : nullptr;
}

uqstl::optional<int64_t> resourceIntField(const ResourceArg& arg, uqstl::string_view name)
{
	const int64_t* const number {findField<int64_t>(arg, name)};
	return number != nullptr ? uqstl::optional<int64_t> {*number} : uqstl::nullopt;
}

namespace uqm
{

uqstl::optional<uqstl::vector<ResourceEntry>> resourceIndexFromJson(uqstl::string_view json, uqstl::string_view prefix)
{
	static constexpr uqstl::string_view c_what {"Resource index"};
	const JsonDoc doc {parseJson(json, c_what)};
	yyjson_val* const resources {versionedMember(doc, c_resourceIndexVersion, "resources", YYJSON_TYPE_OBJ, c_what)};
	if (resources == nullptr)
	{
		return uqstl::nullopt;
	}
	uqstl::vector<ResourceEntry> result {};
	size_t index {};
	size_t max {};
	yyjson_val* key {};
	yyjson_val* entry {};
	yyjson_obj_foreach(resources, index, max, key, entry)
	{
		yyjson_val* const type {yyjson_obj_get(entry, "type")};
		auto arg {readArg(yyjson_obj_get(entry, "data"))};
		if (!yyjson_is_str(type) || !arg)
		{
			uqm::log::warn("{}: '{}' does not have a valid type and data.", c_what, yyjson_get_str(key));
			continue;
		}
		uqstl::string fullKey {prefix};
		fullKey.append(yyjson_get_str(key), yyjson_get_len(key));
		result.push_back({uqstl::move(fullKey), jsonString(type), uqstl::move(*arg)});
	}
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

yyjson_mut_val* valueToJson(yyjson_mut_doc* doc, const auto& value)
{
	using T = uqstl::decay_t<decltype(value)>;
	if constexpr (uqstl::is_same_v<T, uqstl::string>)
	{
		return yyjson_mut_strncpy(doc, value.data(), value.size());
	}
	else if constexpr (uqstl::is_same_v<T, int64_t>)
	{
		return yyjson_mut_sint(doc, value);
	}
	else if constexpr (uqstl::is_same_v<T, double>)
	{
		return yyjson_mut_real(doc, value);
	}
	else if constexpr (uqstl::is_same_v<T, bool>)
	{
		return yyjson_mut_bool(doc, value);
	}
	else if constexpr (uqstl::is_same_v<T, ResourceFields>)
	{
		yyjson_mut_val* const obj {yyjson_mut_obj(doc)};
		for (const auto& [name, field] : value)
		{
			yyjson_mut_obj_add(obj, yyjson_mut_strncpy(doc, name.data(), name.size()), valueToJson(doc, field));
		}
		return obj;
	}
	else
	{
		const auto alternativeToJson {[doc](const auto& alternative) {
			return valueToJson(doc, alternative);
		}};
		return uqstl::visit(alternativeToJson, value);
	}
}

} // namespace

uqstl::string resourceIndexToJson(uqstl::span<const ResourceEntry> entries)
{
	const uqstl::unique_ptr<yyjson_mut_doc, decltype(&yyjson_mut_doc_free)> doc {yyjson_mut_doc_new(nullptr), &yyjson_mut_doc_free};
	yyjson_mut_val* const root {yyjson_mut_obj(doc.get())};
	yyjson_mut_doc_set_root(doc.get(), root);
	yyjson_mut_obj_add_sint(doc.get(), root, "version", c_resourceIndexVersion);
	yyjson_mut_val* const resources {yyjson_mut_obj_add_obj(doc.get(), root, "resources")};
	for (const ResourceEntry& entry : entries)
	{
		yyjson_mut_val* const obj {yyjson_mut_obj(doc.get())};
		yyjson_mut_obj_add_strncpy(doc.get(), obj, "type", entry.type.data(), entry.type.size());
		yyjson_mut_obj_add_val(doc.get(), obj, "data", valueToJson(doc.get(), entry.arg));
		yyjson_mut_obj_add(resources, yyjson_mut_strncpy(doc.get(), entry.key.data(), entry.key.size()), obj);
	}
	size_t length {};
	const uqstl::unique_ptr<char, decltype(&free)> text {
		yyjson_mut_write(doc.get(), YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_NEWLINE_AT_END, &length), &free};
	return text ? uqstl::string {text.get(), length} : uqstl::string {};
}

} // namespace uqm
