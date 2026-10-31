// Reads resource index files: the JSON files that give the type and the data of each resource.
#include "resindex.h"

#include "core/log/log.h"

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

} // namespace uqm
