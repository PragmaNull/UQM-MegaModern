// Converts SuperMelee sides and teams to and from the melee.json format.
#include "meleeconfig.h"

#include "core/log/log.h"
#include "core/string/StringUtils.h"

#include <yyjson.h>

#include <cstdlib>

namespace uqm
{

namespace
{

constexpr int64_t c_meleeConfigVersion {1};
constexpr uqstl::string_view c_shipPrefix {"MELEE_"};

// Ship names are the enumerator names without the "MELEE_" prefix, e.g. "ANDROSYNTH".
uqstl::string_view shipName(MeleeShip ship)
{
	uqstl::string_view name {magic_enum::enum_name(ship)};
	if (name.starts_with(c_shipPrefix))
	{
		name.remove_prefix(c_shipPrefix.size());
	}
	return name;
}

uqstl::optional<MeleeShip> shipFromName(uqstl::string_view name)
{
	for (int index = 0; index < NUM_MELEE_SHIPS; ++index)
	{
		const auto ship {static_cast<MeleeShip>(index)};
		if (compareICase(name, shipName(ship)))
		{
			return ship;
		}
	}
	return uqstl::nullopt;
}

uqstl::optional<PlayerControlFlags> controlFromNames(yyjson_val* names)
{
	if (!yyjson_is_arr(names))
	{
		return uqstl::nullopt;
	}

	PlayerControlFlags control {PlayerControlFlags::None};
	size_t index {};
	size_t count {};
	yyjson_val* name {};
	yyjson_arr_foreach(names, index, count, name)
	{
		if (!yyjson_is_str(name))
		{
			return uqstl::nullopt;
		}
		const auto flag {magic_enum::enum_cast<PlayerControlFlags>(uqstl::string_view {yyjson_get_str(name), yyjson_get_len(name)}, magic_enum::case_insensitive)};
		if (!flag)
		{
			return uqstl::nullopt;
		}
		control |= *flag;
	}
	return control;
}

uqstl::optional<MeleeSideConfig> readSide(yyjson_val* sideObj)
{
	const auto control {controlFromNames(yyjson_obj_get(sideObj, "control"))};
	yyjson_val* team {yyjson_obj_get(sideObj, "team")};
	yyjson_val* name {yyjson_obj_get(team, "name")};
	yyjson_val* ships {yyjson_obj_get(team, "ships")};
	if (!control || !yyjson_is_str(name) || !yyjson_is_arr(ships))
	{
		return uqstl::nullopt;
	}

	MeleeSideConfig side {};
	side.control = *control;
	side.teamName.assign(yyjson_get_str(name), yyjson_get_len(name));

	size_t index {};
	size_t count {};
	yyjson_val* ship {};
	yyjson_arr_foreach(ships, index, count, ship)
	{
		if (yyjson_is_null(ship))
		{
			side.ships.push_back(MELEE_NONE);
			continue;
		}
		if (!yyjson_is_str(ship))
		{
			return uqstl::nullopt;
		}
		const auto parsed {shipFromName({yyjson_get_str(ship), yyjson_get_len(ship)})};
		if (!parsed)
		{
			return uqstl::nullopt;
		}
		side.ships.push_back(*parsed);
	}
	return side;
}

} // namespace

uqstl::string meleeConfigToJson(uqstl::span<const MeleeSideConfig> sides)
{
	yyjson_mut_doc* doc {yyjson_mut_doc_new(nullptr)};
	yyjson_mut_val* root {yyjson_mut_obj(doc)};
	yyjson_mut_doc_set_root(doc, root);
	yyjson_mut_obj_add_sint(doc, root, "version", c_meleeConfigVersion);

	yyjson_mut_val* sideArray {yyjson_mut_obj_add_arr(doc, root, "sides")};
	for (const MeleeSideConfig& side : sides)
	{
		yyjson_mut_val* sideObj {yyjson_mut_arr_add_obj(doc, sideArray)};

		yyjson_mut_val* control {yyjson_mut_obj_add_arr(doc, sideObj, "control")};
		for (const PlayerControlFlags flag : magic_enum::enum_values<PlayerControlFlags>())
		{
			if (flag != PlayerControlFlags::None && testFlag(side.control, flag))
			{
				const uqstl::string_view name {magic_enum::enum_name(flag)};
				yyjson_mut_arr_add_strncpy(doc, control, name.data(), name.size());
			}
		}

		yyjson_mut_val* team {yyjson_mut_obj_add_obj(doc, sideObj, "team")};
		yyjson_mut_obj_add_strncpy(doc, team, "name", side.teamName.data(), side.teamName.size());
		yyjson_mut_val* ships {yyjson_mut_obj_add_arr(doc, team, "ships")};
		for (const MeleeShip ship : side.ships)
		{
			if (ship == MELEE_NONE || ship >= NUM_MELEE_SHIPS)
			{
				yyjson_mut_arr_add_null(doc, ships);
				continue;
			}
			const uqstl::string_view name {shipName(ship)};
			yyjson_mut_arr_add_strncpy(doc, ships, name.data(), name.size());
		}
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

uqstl::optional<uqstl::vector<MeleeSideConfig>> meleeConfigFromJson(uqstl::string_view json)
{
	yyjson_read_err err {};
	yyjson_doc* doc {yyjson_read_opts(const_cast<char*>(json.data()), json.size(), YYJSON_READ_NOFLAG, nullptr, &err)};
	if (doc == nullptr)
	{
		uqm::log::error("Melee configuration: JSON parse error at byte {}: {}", err.pos, err.msg);
		return uqstl::nullopt;
	}

	uqstl::optional<uqstl::vector<MeleeSideConfig>> result {};
	yyjson_val* sideArray {yyjson_obj_get(yyjson_doc_get_root(doc), "sides")};
	if (yyjson_is_arr(sideArray))
	{
		result.emplace();
		size_t index {};
		size_t count {};
		yyjson_val* sideObj {};
		yyjson_arr_foreach(sideArray, index, count, sideObj)
		{
			auto side {readSide(sideObj)};
			if (!side)
			{
				result.reset();
				break;
			}
			result->push_back(uqstl::move(*side));
		}
	}

	if (!result)
	{
		uqm::log::error("Melee configuration: the file does not have the expected structure.");
	}
	yyjson_doc_free(doc);
	return result;
}

} // namespace uqm
