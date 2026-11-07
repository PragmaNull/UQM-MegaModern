// Converts key binding templates to and from the keys.json format.
#include "keyconfig.h"

#include <cstdlib>
#include <yyjson.h>

#include "core/log/log.h"
#include "libs/file/json.h"

namespace uqm
{

static constexpr int64_t c_keyConfigVersion {1};

uqstl::string keyConfigToJson(uqstl::span<const KeyTemplate> templates, uqstl::span<const uqgsl::czstring> controlNames)
{
	const uqm::JsonMutDoc docOwner {uqm::newJsonDoc()};
	yyjson_mut_doc* const doc {docOwner.get()};
	yyjson_mut_val* root {yyjson_mut_obj(doc)};
	yyjson_mut_doc_set_root(doc, root);
	yyjson_mut_obj_add_sint(doc, root, "version", c_keyConfigVersion);

	yyjson_mut_val* templateArray {yyjson_mut_obj_add_arr(doc, root, "templates")};
	for (const KeyTemplate& keyTemplate : templates)
	{
		yyjson_mut_val* templateObj {yyjson_mut_arr_add_obj(doc, templateArray)};
		yyjson_mut_obj_add_strncpy(doc, templateObj, "name", keyTemplate.name.data(), keyTemplate.name.size());

		yyjson_mut_val* controls {yyjson_mut_obj_add_obj(doc, templateObj, "controls")};
		for (size_t control = 0; control < controlNames.size(); ++control)
		{
			yyjson_mut_val* gestures {yyjson_mut_obj_add_arr(doc, controls, controlNames[control])};
			if (control < keyTemplate.bindings.size())
			{
				for (const uqstl::string& gesture : keyTemplate.bindings[control])
				{
					yyjson_mut_arr_add_strncpy(doc, gestures, gesture.data(), gesture.size());
				}
			}
		}
	}

	return uqm::writeJson(docOwner).value_or(uqstl::string {});
}

namespace
{

uqstl::optional<KeyTemplate> readTemplate(yyjson_val* templateObj, uqstl::span<const uqgsl::czstring> controlNames)
{
	yyjson_val* name {yyjson_obj_get(templateObj, "name")};
	yyjson_val* controls {yyjson_obj_get(templateObj, "controls")};
	if (!yyjson_is_str(name) || !yyjson_is_obj(controls))
	{
		return uqstl::nullopt;
	}

	KeyTemplate keyTemplate {};
	keyTemplate.name.assign(yyjson_get_str(name), yyjson_get_len(name));
	keyTemplate.bindings.resize(controlNames.size());

	for (size_t control = 0; control < controlNames.size(); ++control)
	{
		yyjson_val* gestures {yyjson_obj_get(controls, controlNames[control])};
		if (gestures == nullptr)
		{
			continue;
		}
		if (!yyjson_is_arr(gestures))
		{
			return uqstl::nullopt;
		}

		size_t index {};
		size_t count {};
		yyjson_val* gesture {};
		yyjson_arr_foreach(gestures, index, count, gesture)
		{
			if (!yyjson_is_str(gesture))
			{
				return uqstl::nullopt;
			}
			keyTemplate.bindings[control].emplace_back(yyjson_get_str(gesture), yyjson_get_len(gesture));
		}
	}
	return keyTemplate;
}

} // namespace

uqstl::optional<uqstl::vector<KeyTemplate>> keyConfigFromJson(uqstl::string json, uqstl::span<const uqgsl::czstring> controlNames)
{
	const uqm::JsonDoc doc {uqm::parseJsonInPlace(json, "Key configuration")};
	if (!doc)
	{
		return uqstl::nullopt;
	}

	uqstl::optional<uqstl::vector<KeyTemplate>> result {};
	yyjson_val* templateArray {yyjson_obj_get(yyjson_doc_get_root(doc.get()), "templates")};
	if (yyjson_is_arr(templateArray))
	{
		result.emplace();
		size_t index {};
		size_t count {};
		yyjson_val* templateObj {};
		yyjson_arr_foreach(templateArray, index, count, templateObj)
		{
			auto keyTemplate {readTemplate(templateObj, controlNames)};
			if (!keyTemplate)
			{
				result.reset();
				break;
			}
			result->push_back(uqstl::move(*keyTemplate));
		}
	}

	if (!result)
	{
		uqm::log::error("Key configuration: the file does not have the expected structure.");
	}
	return result;
}

} // namespace uqm
