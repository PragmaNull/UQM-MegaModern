#include "Setting.h"

#include "core/log/log.h"

#include <yyjson.h>

#include <cassert>
#include <cstdlib>
#include <limits>

namespace uqm
{

SettingBase::SettingBase(SettingsRegistry& registry, const SettingInfo& info)
	: m_info {info}
{
	registry.add(*this);
}

namespace detail
{

yyjson_mut_val* JsonCodec<bool>::encode(yyjson_mut_doc* doc, bool value)
{
	return yyjson_mut_bool(doc, value);
}

uqstl::optional<bool> JsonCodec<bool>::decode(yyjson_val* val)
{
	if (!yyjson_is_bool(val))
	{
		return uqstl::nullopt;
	}
	return yyjson_get_bool(val);
}

yyjson_mut_val* JsonCodec<int>::encode(yyjson_mut_doc* doc, int value)
{
	return yyjson_mut_sint(doc, value);
}

uqstl::optional<int> JsonCodec<int>::decode(yyjson_val* val)
{
	if (!yyjson_is_int(val))
	{
		return uqstl::nullopt;
	}
	const int64_t value {yyjson_get_sint(val)};
	if (yyjson_is_uint(val) && yyjson_get_uint(val) > static_cast<uint64_t>(uqstl::numeric_limits<int>::max()))
	{
		return uqstl::nullopt;
	}
	if (value < uqstl::numeric_limits<int>::min() || value > uqstl::numeric_limits<int>::max())
	{
		return uqstl::nullopt;
	}
	return static_cast<int>(value);
}

yyjson_mut_val* JsonCodec<float>::encode(yyjson_mut_doc* doc, float value)
{
	return yyjson_mut_float(doc, value);
}

uqstl::optional<float> JsonCodec<float>::decode(yyjson_val* val)
{
	if (!yyjson_is_num(val))
	{
		return uqstl::nullopt;
	}
	return static_cast<float>(yyjson_get_num(val));
}

yyjson_mut_val* JsonCodec<uqstl::string>::encode(yyjson_mut_doc* doc, const uqstl::string& value)
{
	return yyjson_mut_strncpy(doc, value.data(), value.size());
}

uqstl::optional<uqstl::string> JsonCodec<uqstl::string>::decode(yyjson_val* val)
{
	if (!yyjson_is_str(val))
	{
		return uqstl::nullopt;
	}
	return uqstl::string {yyjson_get_str(val), yyjson_get_len(val)};
}

yyjson_mut_val* JsonCodec<Vec2u>::encode(yyjson_mut_doc* doc, const Vec2u& value)
{
	yyjson_mut_val* obj {yyjson_mut_obj(doc)};
	yyjson_mut_obj_add_uint(doc, obj, "x", value.x);
	yyjson_mut_obj_add_uint(doc, obj, "y", value.y);
	return obj;
}

uqstl::optional<Vec2u> JsonCodec<Vec2u>::decode(yyjson_val* val)
{
	yyjson_val* x {yyjson_obj_get(val, "x")};
	yyjson_val* y {yyjson_obj_get(val, "y")};
	if (!yyjson_is_uint(x) || !yyjson_is_uint(y)
		|| yyjson_get_uint(x) > uqstl::numeric_limits<uint32_t>::max()
		|| yyjson_get_uint(y) > uqstl::numeric_limits<uint32_t>::max())
	{
		return uqstl::nullopt;
	}
	return Vec2u {static_cast<uint32_t>(yyjson_get_uint(x)), static_cast<uint32_t>(yyjson_get_uint(y))};
}

uqstl::optional<Vec2u> TextCodec<Vec2u>::fromText(uqstl::string_view text)
{
	const size_t separator {text.find_first_of("x,")};
	if (separator == uqstl::string_view::npos)
	{
		return uqstl::nullopt;
	}
	Vec2u value {};
	if (parseStr(text.substr(0, separator), value.x) != uqstl::errc {}
		|| parseStr(text.substr(separator + 1), value.y) != uqstl::errc {})
	{
		return uqstl::nullopt;
	}
	return value;
}

uqstl::string TextCodec<Vec2u>::toText(const Vec2u& value)
{
	return fmt::format("{}x{}", value.x, value.y);
}

yyjson_mut_val* writeEnum(yyjson_mut_doc* doc, uqstl::string_view name, int64_t value)
{
	if (name.empty())
	{
		return yyjson_mut_sint(doc, value);
	}
	return yyjson_mut_strncpy(doc, name.data(), name.size());
}

uqstl::optional<int64_t> readEnumInt(yyjson_val* val)
{
	if (!yyjson_is_int(val))
	{
		return uqstl::nullopt;
	}
	return yyjson_get_sint(val);
}

uqstl::optional<uqstl::string_view> readEnumName(yyjson_val* val)
{
	if (!yyjson_is_str(val))
	{
		return uqstl::nullopt;
	}
	return uqstl::string_view {yyjson_get_str(val), yyjson_get_len(val)};
}

} // namespace detail

void SettingsRegistry::add(SettingBase& setting)
{
	assert(find(setting.key()) == nullptr && "Duplicate setting key");
	m_settings.push_back(&setting);
}

SettingBase* SettingsRegistry::find(uqstl::string_view key) const noexcept
{
	for (SettingBase* setting : m_settings)
	{
		if (setting->key() == key)
		{
			return setting;
		}
	}
	return nullptr;
}

void SettingsRegistry::resetAll()
{
	for (SettingBase* setting : m_settings)
	{
		setting->reset();
	}
}

namespace
{

yyjson_mut_val* getOrAddObject(yyjson_mut_doc* doc, yyjson_mut_val* parent, uqstl::string_view name)
{
	if (yyjson_mut_val * existing {yyjson_mut_obj_getn(parent, name.data(), name.size())})
	{
		return existing;
	}
	yyjson_mut_val* child {yyjson_mut_obj(doc)};
	yyjson_mut_obj_add(parent, yyjson_mut_strncpy(doc, name.data(), name.size()), child);
	return child;
}

void readObject(const SettingsRegistry& registry, yyjson_val* obj, const uqstl::string& prefix)
{
	yyjson_val* name {};
	yyjson_obj_iter iter {yyjson_obj_iter_with(obj)};
	while ((name = yyjson_obj_iter_next(&iter)) != nullptr)
	{
		yyjson_val* value {yyjson_obj_iter_get_val(name)};
		const uqstl::string key {
			prefix + uqstl::string {yyjson_get_str(name), yyjson_get_len(name)}
		   };

		if (key == "version")
		{
			continue;
		}

		if (SettingBase * setting {registry.find(key)})
		{
			if (!setting->readJson(value))
			{
				uqm::log::warn("Settings: invalid value for '{}'; using the default.", key);
			}
		}
		else if (yyjson_is_obj(value))
		{
			readObject(registry, value, key + ".");
		}
		else
		{
			uqm::log::warn("Settings: unknown key '{}' ignored.", key);
		}
	}
}

} // namespace

uqstl::string SettingsRegistry::toJson() const
{
	yyjson_mut_doc* doc {yyjson_mut_doc_new(nullptr)};
	yyjson_mut_val* root {yyjson_mut_obj(doc)};
	yyjson_mut_doc_set_root(doc, root);
	yyjson_mut_obj_add_sint(doc, root, "version", c_fileVersion);

	for (const SettingBase* setting : m_settings)
	{
		const uqstl::string_view key {setting->key()};
		yyjson_mut_val* parent {root};

		size_t start {0};
		for (size_t dot {key.find('.')}; dot != uqstl::string_view::npos; dot = key.find('.', start))
		{
			parent = getOrAddObject(doc, parent, key.substr(start, dot - start));
			start = dot + 1;
		}

		const uqstl::string_view leaf {key.substr(start)};
		yyjson_mut_obj_add(parent, yyjson_mut_strncpy(doc, leaf.data(), leaf.size()), setting->writeJson(doc));
	}

	size_t length {};
	char* text {yyjson_mut_write(doc, YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_NEWLINE_AT_END, &length)};
	uqstl::string result {
		text ? uqstl::string {text, length}
			: uqstl::string {}
	  };
	free(text);
	yyjson_mut_doc_free(doc);
	return result;
}

bool SettingsRegistry::fromJson(uqstl::string_view json)
{
	yyjson_read_err err {};
	yyjson_doc* doc {yyjson_read_opts(const_cast<char*>(json.data()), json.size(), YYJSON_READ_NOFLAG, nullptr, &err)};
	if (doc == nullptr)
	{
		uqm::log::error("Settings: JSON parse error at byte {}: {}", err.pos, err.msg);
		return false;
	}

	yyjson_val* root {yyjson_doc_get_root(doc)};
	if (!yyjson_is_obj(root))
	{
		uqm::log::error("Settings: the top level of the file must be an object.");
		yyjson_doc_free(doc);
		return false;
	}

	if (yyjson_val* version {yyjson_obj_get(root, "version")}; yyjson_is_int(version) && yyjson_get_sint(version) > c_fileVersion)
	{
		uqm::log::warn("Settings: file version {} is newer than supported version {}.", yyjson_get_sint(version), c_fileVersion);
	}

	readObject(*this, root, {});
	yyjson_doc_free(doc);
	return true;
}

} // namespace uqm
