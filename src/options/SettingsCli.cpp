// Command-line options generated from the settings registry.
#include "SettingsCli.h"

#include "options/options.h"

#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include <cassert>

namespace uqm
{

// CLI11 writes into the storage; apply() passes it to the setting.
struct SettingsCli::Binding
{
	SettingBase* setting {};
	CLI::Option* option {};
	bool flagValue {};
	uqstl::string textValue {};
};

namespace
{

uqstl::string groupTitle(uqstl::string_view key)
{
	uqstl::string title {key.substr(0, key.find('.'))};
	if (!title.empty())
	{
		title[0] = static_cast<char>(uqstl::toupper(static_cast<unsigned char>(title[0])));
	}
	return title;
}

uqstl::string choicesTypeName(const uqstl::vector<uqstl::string>& choices)
{
	uqstl::string typeName {};
	for (const uqstl::string& choice : choices)
	{
		typeName += (typeName.empty() ? "{" : ",") + choice;
	}
	return typeName + "}";
}

} // namespace

SettingsCli::SettingsCli(CLI::App& app, OptionsStruct& options)
	: m_app {app}
	, m_options {options}
{
	for (SettingBase* setting : m_options.settings())
	{
		if (setting->cliNames().empty())
		{
			continue;
		}

		auto& binding {*m_bindings.emplace_back(uqstl::make_unique<Binding>())};
		binding.setting = setting;

		CLI::App& target {group(groupTitle(setting->key()))};
		const uqstl::string names {setting->cliNames()};
		const uqstl::string description {setting->description()};
		if (setting->isFlag())
		{
			binding.option = target.add_flag(names, binding.flagValue, description);
		}
		else
		{
			binding.option = target.add_option(names, binding.textValue, description);
			const auto choices {setting->valueNames()};
			binding.option->type_name(choices.empty() ? uqstl::string {setting->cliTypeName()} : choicesTypeName(choices));
		}
		binding.option->default_str(setting->defaultText());
	}

	const CLI::Validator percentToScale {
		[](std::string& value) -> std::string {
			int percent {};
			if (parseStr(value, percent) != uqstl::errc {} || percent < 0 || percent > 100)
			{
				return "Value must be an integer from 0 to 100.";
			}
			value = fmt::format("{}", static_cast<float>(percent) / 100.0f);
			return {};
		},
		"[0 - 100]"};
	for (const Setting<float>* volume : {&m_options.musicVolumeScale, &m_options.sfxVolumeScale, &m_options.speechVolumeScale})
	{
		optionFor(*volume)
			->type_name("INT")
			->transform(percentToScale)
			->default_str(fmt::format("{}", static_cast<int>(volume->defaultValue() * 100)));
	}
	optionFor(m_options.gamma)->check(CLI::Range(0.1f, 10.0f));
	optionFor(m_options.timeDilationPct)->check(CLI::Range(0, 1000));
	optionFor(m_options.nebulaevol)->check(CLI::Range(0, 50));

	group("Cheats")
		.add_option("--precursormode,--godmode", m_godModeFlags, "A comma-separated list of god-mode flags to enable. Can be any combindation")
		->expected(-1)
		->transform(CLI::CheckedTransformer {EnumNames<GodModeFlags>::map<std::string>(), CLI::ignore_case})
		->default_str(m_options.optGodModes.defaultText());
	group("Interface")
		.add_option("--fuelrange", m_fuelRangeFlags, "A comma-separated list of fuel range display flags to enable. Can be any combindation")
		->expected(-1)
		->transform(CLI::CheckedTransformer {EnumNames<FuelRangeDisplay>::pairs<std::string>(), CLI::ignore_case})
		->default_str(m_options.optFuelRange.defaultText());
}

SettingsCli::~SettingsCli() = default;

CLI::App& SettingsCli::group(const uqstl::string& title)
{
	auto it {m_groups.find(title)};
	if (it == m_groups.end())
	{
		it = m_groups.emplace(title, m_app.add_option_group(title)).first;
	}
	return *it->second;
}

CLI::Option* SettingsCli::optionFor(const SettingBase& setting) const
{
	for (const auto& binding : m_bindings)
	{
		if (binding->setting == &setting)
		{
			return binding->option;
		}
	}
	assert(false && "Setting has no command-line option");
	return nullptr;
}

void SettingsCli::apply()
{
	for (const auto& binding : m_bindings)
	{
		if (binding->option->count() == 0)
		{
			continue;
		}

		const uqstl::string text {binding->setting->isFlag() ? (binding->flagValue ? "true" : "false") : binding->textValue};
		if (!binding->setting->parseText(text))
		{
			throw CLI::ValidationError {binding->option->get_name(), fmt::format("Invalid value '{}'.", text)};
		}
		binding->setting->markCommandLine();
	}

	if (optionFor(m_options.stereoSFX)->count() > 0 && m_options.stereoSFX && m_options.soundDriver != AudioDriverType::OpenAL)
	{
		throw CLI::ValidationError {"--stereosfx", "--stereosfx option requires --sounddriver=OpenAL"};
	}

	if (!m_godModeFlags.empty())
	{
		GodModeFlags flags {GodModeFlags::None};
		for (const GodModeFlags flag : m_godModeFlags)
		{
			flags |= flag;
		}
		m_options.optGodModes = flags;
		m_options.optGodModes.markCommandLine();
	}

	if (!m_fuelRangeFlags.empty())
	{
		FuelRangeDisplay flags {FuelRangeDisplay::Normal};
		for (const FuelRangeDisplay flag : m_fuelRangeFlags)
		{
			flags |= flag;
		}
		m_options.optFuelRange = flags;
		m_options.optFuelRange.markCommandLine();
	}
}

} // namespace uqm
