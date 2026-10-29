// Command-line options generated from the settings registry.
#pragma once
#ifndef UQM_OPTIONS_SETTINGSCLI_H_
#define UQM_OPTIONS_SETTINGSCLI_H_

#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "options/OptionDefs.h"

#include <map>
#include <memory>

namespace CLI
{
class App;
class Option;
} // namespace CLI

namespace uqm
{

struct OptionsStruct;
class SettingBase;

// Adds a command-line option for each setting that has CLI names, and applies the parsed values.
class SettingsCli
{
public:
	SettingsCli(CLI::App& app, OptionsStruct& options);
	~SettingsCli();

	SettingsCli(const SettingsCli&) = delete;
	SettingsCli& operator=(const SettingsCli&) = delete;

	// Returns the option group with 'title'. Creates it on first use.
	CLI::App& group(const uqstl::string& title);

	[[nodiscard]] CLI::Option* optionFor(const SettingBase& setting) const;

	// Call after CLI::App::parse(). Throws CLI::ValidationError if a value is not valid.
	void apply();

private:
	struct Binding;

	CLI::App& m_app;
	OptionsStruct& m_options;
	uqstl::map<uqstl::string, CLI::App*> m_groups {};
	uqstl::vector<uqstl::unique_ptr<Binding>> m_bindings {};
	uqstl::vector<GodModeFlags> m_godModeFlags {};
	uqstl::vector<FuelRangeDisplay> m_fuelRangeFlags {};
};

} // namespace uqm

#endif // UQM_OPTIONS_SETTINGSCLI_H_
