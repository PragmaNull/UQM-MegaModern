#pragma once
#ifndef UQM_OPTIONS_SETTING_H_
#define UQM_OPTIONS_SETTING_H_

#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "lib/Math2D/Math2D.h"
#include "options/OptionTypes.h"

#include <optional>

struct yyjson_val;
struct yyjson_mut_doc;
struct yyjson_mut_val;

namespace uqm
{

class SettingsRegistry;

// A persisted option. Each Setting registers itself with its owning registry on construction.
class SettingBase
{
public:
	SettingBase(SettingsRegistry& registry, uqgsl::czstring key, uqgsl::czstring description);
	virtual ~SettingBase() = default;

	SettingBase(const SettingBase&) = delete;
	SettingBase& operator=(const SettingBase&) = delete;

	// Dotted path in the settings file, e.g. "graphics.scanlines".
	[[nodiscard]] uqstl::string_view key() const noexcept
	{
		return m_key;
	}

	[[nodiscard]] uqstl::string_view description() const noexcept
	{
		return m_description;
	}

	// Marks the current value as a command-line override if it differs from the default.
	virtual void captureCommandLine() = 0;

	// Returns false if 'val' has the wrong type or fails validation.
	virtual bool readJson(yyjson_val* val) = 0;
	[[nodiscard]] virtual yyjson_mut_val* writeJson(yyjson_mut_doc* doc) const = 0;

	virtual void reset() = 0;

private:
	uqgsl::czstring m_key {};
	uqgsl::czstring m_description {};
};

namespace detail
{
template <typename T>
struct JsonCodec;

template <>
struct JsonCodec<bool>
{
	static yyjson_mut_val* encode(yyjson_mut_doc* doc, bool value);
	static uqstl::optional<bool> decode(yyjson_val* val);
};

template <>
struct JsonCodec<int>
{
	static yyjson_mut_val* encode(yyjson_mut_doc* doc, int value);
	static uqstl::optional<int> decode(yyjson_val* val);
};

template <>
struct JsonCodec<float>
{
	static yyjson_mut_val* encode(yyjson_mut_doc* doc, float value);
	static uqstl::optional<float> decode(yyjson_val* val);
};

template <>
struct JsonCodec<uqstl::string>
{
	static yyjson_mut_val* encode(yyjson_mut_doc* doc, const uqstl::string& value);
	static uqstl::optional<uqstl::string> decode(yyjson_val* val);
};

template <>
struct JsonCodec<Vec2u>
{
	static yyjson_mut_val* encode(yyjson_mut_doc* doc, const Vec2u& value);
	static uqstl::optional<Vec2u> decode(yyjson_val* val);
};

yyjson_mut_val* writeEnum(yyjson_mut_doc* doc, uqstl::string_view name, int64_t value);
uqstl::optional<int64_t> readEnumInt(yyjson_val* val);
uqstl::optional<uqstl::string_view> readEnumName(yyjson_val* val);

// Enums are written by name. Values without a name, such as combined flags, are written as integers.
template <EnumType E>
struct JsonCodec<E>
{
	static yyjson_mut_val* encode(yyjson_mut_doc* doc, E value)
	{
		return writeEnum(doc, magic_enum::enum_name(value), static_cast<int64_t>(value));
	}

	static uqstl::optional<E> decode(yyjson_val* val)
	{
		if (const auto name {readEnumName(val)})
		{
			return magic_enum::enum_cast<E>(*name, magic_enum::case_insensitive);
		}
		if (const auto number {readEnumInt(val)})
		{
			const auto value {static_cast<E>(*number)};
			if constexpr (FlagEnum<E>)
			{
				if ((*number & ~declaredBits<E>()) == 0)
				{
					return value;
				}
			}
			else if (magic_enum::enum_contains(value))
			{
				return value;
			}
		}
		return uqstl::nullopt;
	}
};
} // namespace detail

template <typename T>
class Setting : public OptionT<T>
	, public SettingBase
{
public:
	// Returns true if the value is acceptable.
	using Validator = bool (*)(const T&);

	Setting(SettingsRegistry& registry, uqgsl::czstring key, T defaultValue, uqgsl::czstring description, Validator validator = nullptr)
		: OptionT<T> {defaultValue}
		, SettingBase {registry, key, description}
		, m_default {defaultValue}
		, m_fileValue {defaultValue}
		, m_validator {validator}
	{
	}

	using OptionT<T>::operator=;

	[[nodiscard]] const T& defaultValue() const noexcept
	{
		return m_default;
	}

	void captureCommandLine() override
	{
		if (this->value != m_default)
		{
			m_cliValue = this->value;
		}
	}

	bool readJson(yyjson_val* val) override
	{
		const auto parsed {detail::JsonCodec<T>::decode(val)};
		if (!parsed || (m_validator && !m_validator(*parsed)))
		{
			return false;
		}

		m_fileValue = *parsed;
		if (!m_cliValue)
		{
			this->value = *parsed;
		}
		return true;
	}

	// A command-line override is not saved unless the value was changed again since.
	[[nodiscard]] yyjson_mut_val* writeJson(yyjson_mut_doc* doc) const override
	{
		const bool isCliOverride {m_cliValue && this->value == *m_cliValue};
		return detail::JsonCodec<T>::encode(doc, isCliOverride ? m_fileValue : this->value);
	}

	void reset() override
	{
		this->value = m_default;
		this->set = false;
		m_fileValue = m_default;
		m_cliValue.reset();
	}

private:
	T m_default {};
	T m_fileValue {};
	uqstl::optional<T> m_cliValue {};
	Validator m_validator {};
};

class SettingsRegistry
{
public:
	static constexpr int64_t c_fileVersion {1};

	SettingsRegistry() = default;
	SettingsRegistry(const SettingsRegistry&) = delete;
	SettingsRegistry& operator=(const SettingsRegistry&) = delete;

	void add(SettingBase& setting);

	[[nodiscard]] uqstl::span<SettingBase* const> settings() const noexcept
	{
		return m_settings;
	}

	[[nodiscard]] SettingBase* find(uqstl::string_view key) const noexcept;

	void captureCommandLine();
	void resetAll();

	[[nodiscard]] uqstl::string toJson() const;

	// Returns false if 'json' is not a JSON object. Unknown keys and invalid values are logged and skipped.
	bool fromJson(uqstl::string_view json);

private:
	uqstl::vector<SettingBase*> m_settings {};
};

} // namespace uqm

#endif // UQM_OPTIONS_SETTING_H_
