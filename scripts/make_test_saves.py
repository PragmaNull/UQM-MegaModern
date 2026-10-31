"""Make the test saves in test/data/saves from interplanetary-sol.json.

Each save changes the state of the base save to reach a part of the save
format that the base save does not use. The values are made up, but the
summary must agree with the state, because the game makes the summary again
from the state when it saves.
"""

import copy
import datetime
import json
from pathlib import Path

SAVES_DIR = Path(__file__).resolve().parent.parent / "test" / "data" / "saves"
BASE_SAVE = SAVES_DIR / "interplanetary-sol.json"

# ACTIVITY values from uqm/globdata.h.
IN_HYPERSPACE = 3
IN_INTERPLANETARY = 4
IN_QUASISPACE = 6
IN_PLANET_ORBIT = 7
IN_STARBASE = 8
START_ENCOUNTER = 1 << 10

# RACE_ID values from uqm/races.h.
ORZ, PKUNK, SPATHI, SUPOX, UTWIG, YEHAT = 3, 4, 6, 7, 9, 11
ILWRATH, MYCON, UMGAH, URQUAN = 14, 15, 17, 18


def ship(race_id, crew, energy, captain=0, index=0):
    return {"captains_name_index": captain, "race_id": race_id, "index": index,
            "crew_level": crew, "max_crew": crew, "energy_level": 0, "max_energy": energy}


def stamp_random_groups(save):
    """Date the random battle group as a save in interplanetary space does: one week after the game clock."""
    clock = save["globalState"]["GameClock"]
    date = datetime.date(clock["year_index"], clock["month_index"], clock["day_index"]) + datetime.timedelta(days=7)
    group = save["battleGroups"][0]
    group.update({"day_index": date.day, "month_index": date.month, "year_index": date.year})


def summary_flags(flags):
    """The lander upgrades and the bomb state, as the summary of a save keeps them."""
    return (flags["LANDER_SHIELDS"]
            | flags["IMPROVED_LANDER_SPEED"] << 4
            | flags["IMPROVED_LANDER_CARGO"] << 5
            | flags["IMPROVED_LANDER_SHOT"] << 6
            | (flags["CHMMR_BOMB_STATE"] >= 2) << 7)


def set_activity(save, activity, summary_activity):
    save["globalState"]["CurrentActivity"] = activity
    save["summary"]["Activity"] = summary_activity


def in_hyperspace(save):
    set_activity(save, IN_HYPERSPACE, IN_HYPERSPACE)
    sis = save["summary"]["sis"]
    sis["log"] = {"x": 52100, "y": 97700}
    sis["PlanetName"] = ""
    state = save["globalState"]
    state["autopilot"] = {"x": 2400, "y": 3900}
    state["ShipFacing"] = 5
    state["in_orbit"] = 0
    state["velocity"] = {"TravelAngle": 12, "vector": {"x": 3, "y": -7}, "fract": {"x": 120, "y": 64},
                         "error": {"x": 1, "y": 2}, "incr": {"x": 1, "y": -1}}
    save["starDesc"] = {"star_pt": {"x": 0, "y": 0}, "Type": 0, "Index": 0, "Prefix": 0, "Postfix": 0}


def hyperspace(save):
    in_hyperspace(save)
    save["encounters"] = [
        {"transition_state": 0, "origin": {"x": 5130, "y": 9820}, "radius": 1000,
         "loc_pt": {"x": 5130, "y": 9820}, "race_id": ILWRATH, "flags": 0,
         "ships": [{"race_id": ILWRATH, "crew_level": 22, "max_crew": 22, "max_energy": 16}] * 3,
         "log": {"x": 51300, "y": 98200}},
        {"transition_state": 0, "origin": {"x": 5300, "y": 9600}, "radius": 1000,
         "loc_pt": {"x": 5280, "y": 9640}, "race_id": UMGAH, "flags": 0,
         "ships": [{"race_id": UMGAH, "crew_level": 10, "max_crew": 10, "max_energy": 30}],
         "log": {"x": 52800, "y": 96400}},
    ]


def quasispace(save):
    in_hyperspace(save)
    set_activity(save, IN_HYPERSPACE, IN_QUASISPACE)
    save["gameState"]["ARILOU_SPACE_SIDE"] = 3
    save["summary"]["sis"]["log"] = {"x": 5000, "y": 5000}


def hyperspace_encounter(save):
    hyperspace(save)
    set_activity(save, START_ENCOUNTER | IN_HYPERSPACE, IN_HYPERSPACE)
    save["npcShipQueue"] = [ship(ILWRATH, 22, 16), ship(ILWRATH, 22, 16), ship(ILWRATH, 22, 16)]


def starbase(save):
    set_activity(save, START_ENCOUNTER | IN_INTERPLANETARY, IN_STARBASE)
    save["gameState"]["GLOBAL_FLAGS_AND_DATA"] = 255
    save["stowedShipQueue"] = [ship(PKUNK, 8, 12, captain=3), ship(SPATHI, 30, 10, captain=7)]


def interplanetary_groups(save):
    set_activity(save, IN_INTERPLANETARY, IN_INTERPLANETARY)
    save["ipGroupQueue"] = [
        {"group_counter": 0, "race_id": SPATHI, "sys_loc": 4, "task": 1, "in_system": 1,
         "dest_loc": 4, "orbit_pos": 2, "group_id": 1, "loc": {"x": -400, "y": 210}},
        {"group_counter": 3, "race_id": SPATHI, "sys_loc": 4, "task": 2, "in_system": 1,
         "dest_loc": 0, "orbit_pos": 5, "group_id": 2, "loc": {"x": 120, "y": -330}},
    ]
    # The save writes the groups in interplanetary space to the group list too.
    save["groupList"]["groups"] = [{"race_outer": g["race_id"], **g} for g in save["ipGroupQueue"]]


def planet_orbit(save):
    # Orbit of Earth (planet 3). 2 would be its moon, which has the starbase.
    set_activity(save, IN_INTERPLANETARY, IN_PLANET_ORBIT)
    save["globalState"]["ip_planet"] = 3
    save["globalState"]["in_orbit"] = 1


def late_game(save):
    fleet = [ship(race, 20, 20, captain=i, index=i)
             for i, race in enumerate([ORZ, PKUNK, SPATHI, SUPOX, UTWIG, YEHAT] * 2)]
    save["shipQueue"] = fleet
    summary = save["summary"]
    summary["ShipList"] = [s["race_id"] for s in fleet]
    summary["year_index"] = 2158
    summary["month_index"] = 7
    summary["day_index"] = 14
    save["globalState"]["GameClock"].update({"year_index": 2158, "month_index": 7, "day_index": 14})
    sis = summary["sis"]
    sis.update({"ResUnits": 48000, "FuelOnBoard": 16000, "CrewEnlisted": 400, "TotalElementMass": 900,
                "TotalBioMass": 45, "NumLanders": 6})
    sis["ElementAmounts"] = [100, 120, 90, 150, 80, 200, 110, 50]
    flags = save["gameState"]
    for name, value in {"MELNORME_CREDIT0": 120, "MELNORME_CREDIT1": 3, "SHOFIXTI_VISITS": 2,
                        "SPATHI_VISITS": 5, "UTWIG_BOMB_ON_SHIP": 1, "SUN_DEVICE_ON_SHIP": 1,
                        "AQUA_HELIX_ON_SHIP": 1, "LANDER_SHIELDS": 3, "IMPROVED_LANDER_SPEED": 1}.items():
        flags[name] = value
    summary["MCreditLo"] = 120
    summary["MCreditHi"] = 3
    summary["Flags"] = summary_flags(flags)
    # The devices that the game finds from the flags above.
    summary["DeviceList"] = [2, 3, 5, 27]
    stamp_random_groups(save)


VARIANTS = {
    "hyperspace": hyperspace,
    "quasispace": quasispace,
    "hyperspace-encounter": hyperspace_encounter,
    "starbase": starbase,
    "interplanetary-groups": interplanetary_groups,
    "planet-orbit": planet_orbit,
    "late-game": late_game,
}


def main():
    base = json.loads(BASE_SAVE.read_text(encoding="utf-8"))
    for name, change in VARIANTS.items():
        save = copy.deepcopy(base)
        save["summary"]["SaveName"] = name
        change(save)
        path = SAVES_DIR / f"{name}.json"
        path.write_text(json.dumps(save, indent=2) + "\n", encoding="utf-8")
        print(f"wrote {path}")


if __name__ == "__main__":
    main()
