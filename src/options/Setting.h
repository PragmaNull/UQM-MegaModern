// Persisted options (Setting<T>) and the registry that saves them as JSON.
#pragma once
#ifndef UQM_OPTIONS_SETTING_H_
#define UQM_OPTIONS_SETTING_H_

#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "lib/Math2D/Math2D.h"
#include "options/OptionTypes.h"

#include <fmt/format.h>
#include <optional>

struct yyjson_val;
struct yyjson_mut_doc;
struct yyjson_mut_val;

namespace uqm
{

class SettingsRegistry;

struct SettingInfo
{
	// Dotted path in the settings file, e.g. "graphics.scanlines".
	uqgsl::czstring key {};
	uqgsl::czstring description {};
	// CLI11 option names, e.g. "-s,--scanlines". Empty if the setting has no command-line option.
	uqgsl::czstring cli {""};
};

// A persisted option. Each Setting registers itself with its owning registry on construction.
class SettingBase
{
public:
	SettingBase(SettingsRegistry& registry, const SettingInfo& info);
	virtual ~SettingBase() = default;

	SettingBase(const SettingBase&) = delete;
	SettingBase& operator=(const SettingBase&) = delete;

	[[nodiscard]] uqstl::string_view key() const noexcept
	{
		return m_info.key;
	}

	[[nodiscard]] uqstl::string_view description() const noexcept
	{
		return m_info.description;
	}

	[[nodiscard]] uqstl::string_view cliNames() const noexcept
	{
		return m_info.cli;
	}

	// Command-line support.
	[[nodiscard]] virtual bool isFlag() const noexcept = 0;
	// Returns false if 'text' is not a valid value.
	virtual bool parseText(uqstl::string_view text) = 0;
	[[nodiscard]] virtual uqstl::string defaultText() const = 0;
	[[nodiscard]] virtual uqstl::string valueText() const = 0;
	// Accepted names for an enum setting. Empty for other types.
	[[nodiscard]] virtual uqstl::vector<uqstl::string> valueNames() const = 0;
	// Value type shown in command-line help for non-enum, non-flag settings.
	[[nodiscard]] virtual uqstl::string_view cliTypeName() const noexcept = 0;

	// Records the current value as set on the command line.
	virtual void markCommandLine() = 0;
	// True if the value was set on the command line and not changed since.
	[[nodiscard]] virtual bool hasCommandLineOverride() const = 0;

	// Returns false if 'val' has the wrong type or fails validation.
	virtual bool readJson(yyjson_val* val) = 0;
	[[nodiscard]] virtual yyjson_mut_val* writeJson(yyjson_mut_doc* doc) const = 0;

	virtual void reset() = 0;

private:
	SettingInfo m_info {};
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

template <EnumType E>
[[nodiscard]] constexpr bool isAcceptedEnumValue(int64_t number)
{
	if constexpr (FlagEnum<E>)
	{
		return (number & ~declaredBits<E>()) == 0;
	}
	else
	{
		return magic_enum::enum_contains(static_cast<E>(number));
	}
}

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
		if (const auto number {readEnumInt(val)}; number && isAcceptedEnumValue<E>(*number))
		{
			return static_cast<E>(*number);
		}
		return uqstl::nullopt;
	}
};

template <typename T>
struct TextCodec
{
	static uqstl::optional<T> fromText(uqstl::string_view text)
	{
		T value {};
		if (parseStr(text, value) != uqstl::errc {})
		{
			return uqstl::nullopt;
		}
		return value;
	}

	static uqstl::string toText(const T& value)
	{
		return fmt::format("{}", value);
	}
};

template <>
struct TextCodec<uqstl::string>
{
	static uqstl::optional<uqstl::string> fromText(uqstl::string_view text)
	{
		return uqstl::string {text};
	}

	static uqstl::string toText(const uqstl::string& value)
	{
		return value;
	}
};

template <>
struct TextCodec<Vec2u>
{
	static uqstl::optional<Vec2u> fromText(uqstl::string_view text);
	static uqstl::string toText(const Vec2u& value);
};

// Enums accept their display name, their enumerator name or a number.
template <EnumType E>
struct TextCodec<E>
{
	static uqstl::optional<E> fromText(uqstl::string_view text)
	{
		for (const E value : magic_enum::enum_values<E>())
		{
			if (compareICase(text, toString(value)) || compareICase(text, magic_enum::enum_name(value)))
			{
				return value;
			}
		}
		int64_t number {};
		if (parseStr(text, number) == uqstl::errc {} && isAcceptedEnumValue<E>(number))
		{
			return static_cast<E>(number);
		}
		return uqstl::nullopt;
	}

	// Uses the display name unless it contains a space; then the enumerator name.
	static uqstl::string toText(E value)
	{
		const uqstl::string_view enumName {magic_enum::enum_name(value)};
		if (enumName.empty())
		{
			return fmt::format("{}", static_cast<int64_t>(value));
		}
		const uqstl::string displayName {toString(value)};
		return displayName.find(' ') == uqstl::string::npos ? displayName : uqstl::string {enumName};
	}

	static uqstl::vector<uqstl::string> names()
	{
		uqstl::vector<uqstl::string> result {};
		for (const E value : magic_enum::enum_values<E>())
		{
			result.emplace_back(toText(value));
		}
		return result;
	}
};

// Only PC and 3DO are valid choices.
template <>
struct TextCodec<EmulationMode>
{
	static uqstl::optional<EmulationMode> fromText(uqstl::string_view text)
	{
		for (const EmulationMode value : {EmulationMode::PC, EmulationMode::Console3DO})
		{
			if (compareICase(text, toString(value)) || compareICase(text, magic_enum::enum_name(value)))
			{
				return value;
			}
		}
		return uqstl::nullopt;
	}

	static uqstl::string toText(EmulationMode value)
	{
		return toString(value);
	}

	static uqstl::vector<uqstl::string> names()
	{
		return {toText(EmulationMode::PC), toText(EmulationMode::Console3DO)};
	}
};

template <>
struct JsonCodec<EmulationMode>
{
	static yyjson_mut_val* encode(yyjson_mut_doc* doc, EmulationMode value)
	{
		return writeEnum(doc, magic_enum::enum_name(value), static_cast<int64_t>(value));
	}

	static uqstl::optional<EmulationMode> decode(yyjson_val* val)
	{
		if (const auto name {readEnumName(val)})
		{
			return TextCodec<EmulationMode>::fromText(*name);
		}
		return uqstl::nullopt;
	}
};

template <typename T>
[[nodiscard]] constexpr uqstl::string_view cliTypeName() noexcept
{
	if constexpr (uqstl::is_same_v<T, int>)
	{
		return "INT";
	}
	else if constexpr (uqstl::is_same_v<T, float>)
	{
		return "FLOAT";
	}
	else if constexpr (uqstl::is_same_v<T, Vec2u>)
	{
		return "WIDTHxHEIGHT";
	}
	else
	{
		return "TEXT";
	}
}
} // namespace detail

template <typename T>
class Setting : public OptionT<T>
	, public SettingBase
{
public:
	// Returns true if the value is acceptable.
	using Validator = bool (*)(const T&);

	Setting(SettingsRegistry& registry, const SettingInfo& info, T defaultValue, Validator validator = nullptr)
		: OptionT<T> {defaultValue}
		, SettingBase {registry, info}
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

	[[nodiscard]] bool isFlag() const noexcept override
	{
		return uqstl::is_same_v<T, bool>;
	}

	bool parseText(uqstl::string_view text) override
	{
		const auto parsed {detail::TextCodec<T>::fromText(text)};
		if (!isAcceptable(parsed))
		{
			return false;
		}
		*this = *parsed;
		return true;
	}

	[[nodiscard]] uqstl::string defaultText() const override
	{
		return detail::TextCodec<T>::toText(m_default);
	}

	[[nodiscard]] uqstl::string valueText() const override
	{
		return detail::TextCodec<T>::toText(this->value);
	}

	[[nodiscard]] uqstl::vector<uqstl::string> valueNames() const override
	{
		if constexpr (EnumType<T>)
		{
			return detail::TextCodec<T>::names();
		}
		else
		{
			return {};
		}
	}

	[[nodiscard]] uqstl::string_view cliTypeName() const noexcept override
	{
		return detail::cliTypeName<T>();
	}

	void markCommandLine() override
	{
		m_cliValue = this->value;
	}

	[[nodiscard]] bool hasCommandLineOverride() const override
	{
		return m_cliValue && this->value == *m_cliValue;
	}

	bool readJson(yyjson_val* val) override
	{
		const auto parsed {detail::JsonCodec<T>::decode(val)};
		if (!isAcceptable(parsed))
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
		return detail::JsonCodec<T>::encode(doc, hasCommandLineOverride() ? m_fileValue : this->value);
	}

	void reset() override
	{
		this->value = m_default;
		this->set = false;
		m_fileValue = m_default;
		m_cliValue.reset();
	}

private:
	[[nodiscard]] bool isAcceptable(const uqstl::optional<T>& value) const
	{
		return value && (!m_validator || m_validator(*value));
	}

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

	void resetAll();

	[[nodiscard]] uqstl::string toJson() const;

	// Returns false if 'json' is not a JSON object. Unknown keys and invalid values are logged and skipped.
	bool fromJson(uqstl::string_view json);

private:
	uqstl::vector<SettingBase*> m_settings {};
};

} // namespace uqm

#endif // UQM_OPTIONS_SETTING_H_
