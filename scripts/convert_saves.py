"""Convert binary save games (uqmsave.NN) to the JSON save format (uqmsave.NN.json).

Reads the MegaMod formats MMV4, MMV3 and MEGA, and the core UQM format UMS1.
The game reads only the JSON format.
"""

import argparse
import json
import re
import struct
import sys
from pathlib import Path

SAVE_FORMAT_VERSION = 1
DEFAULT_SAVE_DIR = Path.home() / ".uqm-megamodern" / "save"
SAVE_NAME = re.compile(r"uqmsave\.\d\d")


def tag(text):
    return struct.unpack("<I", text.encode("ascii"))[0]


CORE_TAG = 0x01534D55
MEGA_TAG = tag("MEGA")
MMV3_TAG = tag("MMV3")
MMV4_TAG = tag("MMV4")
SUMMARY_TAG = tag("Summ")
GLOBAL_STATE_TAG = tag("GlSt")
GAME_STATE_TAG = tag("GmSt")
EVENTS_TAG = tag("Evts")
ENCOUNTERS_TAG = tag("Enct")
RACE_Q_TAG = tag("RacQ")
IP_GRP_Q_TAG = tag("IGpQ")
NPC_SHIP_Q_TAG = tag("NpcQ")
SHIP_Q_TAG = tag("ShpQ")
STOWED_Q_TAG = tag("StwQ")
STAR_TAG = tag("Star")
SCAN_TAG = tag("Scan")
BATTLE_GROUP_TAG = tag("BtGp")
GROUP_LIST_TAG = tag("Grps")

NUM_MODULES = 20
NUM_ELEMENT_CATEGORIES = 8
NUM_MODULE_SLOTS = 16
NUM_DRIVE_SLOTS = 11
NUM_JET_SLOTS = 8
MAX_BUILT_SHIPS = 12
MAX_EXCLUSIVE_DEVICES = 16
MAX_HYPER_SHIPS = 7
SIS_NAME_SIZE = 32
LEGACY_SIS_NAME_SIZE = 16
SAVE_NAME_SIZE = 64
NUM_READ_SPEEDS = 5
NO_LOCATION = 0xFFFF

# Game state values in save order, with their bit counts. (None, N) starts
# revision N of the table, and (None, 0) ends it.
GAME_STATE_BIT_MAP = [
    ("SHOFIXTI_VISITS", 3), ("SHOFIXTI_STACK1", 2), ("SHOFIXTI_STACK2", 3), ("SHOFIXTI_STACK3", 2),
    ("SHOFIXTI_KIA", 1), ("SHOFIXTI_BRO_KIA", 1), ("SHOFIXTI_RECRUITED", 1), ("SHOFIXTI_MAIDENS", 1),
    ("MAIDENS_ON_SHIP", 1), ("BATTLE_SEGUE", 1), ("PLANETARY_LANDING", 1), ("PLANETARY_CHANGE", 1),
    ("SPATHI_VISITS", 3), ("SPATHI_HOME_VISITS", 3), ("FOUND_PLUTO_SPATHI", 2),
    ("SPATHI_SHIELDED_SELVES", 1), ("SPATHI_CREATURES_EXAMINED", 1),
    ("SPATHI_CREATURES_ELIMINATED", 1), ("UMGAH_BROADCASTERS", 1), ("SPATHI_MANNER", 2),
    ("SPATHI_QUEST", 1), ("LIED_ABOUT_CREATURES", 2), ("SPATHI_PARTY", 1), ("KNOW_SPATHI_PASSWORD", 1),
    ("ILWRATH_HOME_VISITS", 3), ("ILWRATH_CHMMR_VISITS", 1), ("ARILOU_SPACE", 1),
    ("ARILOU_SPACE_SIDE", 2), ("ARILOU_SPACE_COUNTER", 4), ("LANDER_SHIELDS", 4), ("MET_MELNORME", 1),
    ("MELNORME_RESCUE_REFUSED", 1), ("MELNORME_RESCUE_COUNT", 3), ("TRADED_WITH_MELNORME", 1),
    ("WHY_MELNORME_PURPLE", 1), ("MELNORME_CREDIT0", 8), ("MELNORME_CREDIT1", 8),
    ("MELNORME_BUSINESS_COUNT", 2), ("MELNORME_YACK_STACK0", 2), ("MELNORME_YACK_STACK1", 2),
    ("MELNORME_YACK_STACK2", 4), ("MELNORME_YACK_STACK3", 3), ("MELNORME_YACK_STACK4", 2),
    ("WHY_MELNORME_BLUE", 1), ("MELNORME_ANGER", 2), ("MELNORME_MIFFED_COUNT", 2),
    ("MELNORME_PISSED_COUNT", 2), ("MELNORME_HATE_COUNT", 2), ("PROBE_MESSAGE_DELIVERED", 1),
    ("PROBE_ILWRATH_ENCOUNTER", 1), ("STARBASE_AVAILABLE", 1), ("STARBASE_VISITED", 1),
    ("RADIOACTIVES_PROVIDED", 1), ("LANDERS_LOST", 1), ("GIVEN_FUEL_BEFORE", 1),
    ("AWARE_OF_SAMATRA", 1), ("YEHAT_CAVALRY_ARRIVED", 1), ("URQUAN_MESSED_UP", 1),
    ("MOONBASE_DESTROYED", 1), ("WILL_DESTROY_BASE", 1), ("WIMBLIS_TRIDENT_ON_SHIP", 1),
    ("GLOWING_ROD_ON_SHIP", 1), ("KOHR_AH_KILLED_ALL", 1), ("STARBASE_YACK_STACK1", 1),
    ("DISCUSSED_PORTAL_SPAWNER", 1), ("DISCUSSED_TALKING_PET", 1), ("DISCUSSED_UTWIG_BOMB", 1),
    ("DISCUSSED_SUN_EFFICIENCY", 1), ("DISCUSSED_ROSY_SPHERE", 1), ("DISCUSSED_AQUA_HELIX", 1),
    ("DISCUSSED_CLEAR_SPINDLE", 1), ("DISCUSSED_ULTRON", 1), ("DISCUSSED_MAIDENS", 1),
    ("DISCUSSED_UMGAH_HYPERWAVE", 1), ("DISCUSSED_BURVIX_HYPERWAVE", 1), ("SYREEN_WANT_PROOF", 1),
    ("PLAYER_HAVING_SEX", 1), ("MET_ARILOU", 1), ("DISCUSSED_TAALO_PROTECTOR", 1),
    ("DISCUSSED_EGG_CASING0", 1), ("DISCUSSED_EGG_CASING1", 1), ("DISCUSSED_EGG_CASING2", 1),
    ("DISCUSSED_SYREEN_SHUTTLE", 1), ("DISCUSSED_VUX_BEAST", 1), ("DISCUSSED_DESTRUCT_CODE", 1),
    ("DISCUSSED_URQUAN_WARP", 1), ("DISCUSSED_WIMBLIS_TRIDENT", 1), ("DISCUSSED_GLOWING_ROD", 1),
    ("ATTACKED_DRUUGE", 1), ("NEW_ALLIANCE_NAME", 2), ("PORTAL_COUNTER", 4),
    ("BURVIXESE_BROADCASTERS", 1), ("BURV_BROADCASTERS_ON_SHIP", 1), ("UTWIG_BOMB", 1),
    ("UTWIG_BOMB_ON_SHIP", 1), ("AQUA_HELIX", 1), ("AQUA_HELIX_ON_SHIP", 1), ("SUN_DEVICE", 1),
    ("SUN_DEVICE_ON_SHIP", 1), ("TAALO_PROTECTOR", 1), ("TAALO_PROTECTOR_ON_SHIP", 1),
    ("SHIP_VAULT_UNLOCKED", 1), ("SYREEN_SHUTTLE", 1), ("PORTAL_KEY", 1), ("PORTAL_KEY_ON_SHIP", 1),
    ("VUX_BEAST", 1), ("VUX_BEAST_ON_SHIP", 1), ("TALKING_PET", 1), ("TALKING_PET_ON_SHIP", 1),
    ("MOONBASE_ON_SHIP", 1), ("KOHR_AH_FRENZY", 1), ("KOHR_AH_VISITS", 2), ("KOHR_AH_BYES", 1),
    ("SLYLANDRO_HOME_VISITS", 3), ("DESTRUCT_CODE_ON_SHIP", 1), ("ILWRATH_VISITS", 3),
    ("ILWRATH_DECEIVED", 1), ("FLAGSHIP_CLOAKED", 1), ("MYCON_VISITS", 3), ("MYCON_HOME_VISITS", 3),
    ("MYCON_AMBUSH", 1), ("MYCON_FELL_FOR_AMBUSH", 1), ("GLOBAL_FLAGS_AND_DATA", 8), ("ORZ_VISITS", 3),
    ("TAALO_VISITS", 3), ("ORZ_MANNER", 2), ("PROBE_EXHIBITED_BUG", 1), ("CLEAR_SPINDLE_ON_SHIP", 1),
    ("URQUAN_VISITS", 3), ("PLAYER_HYPNOTIZED", 1), ("VUX_VISITS", 3), ("VUX_HOME_VISITS", 3),
    ("ZEX_VISITS", 3), ("ZEX_IS_DEAD", 1), ("KNOW_ZEX_WANTS_MONSTER", 1), ("UTWIG_VISITS", 3),
    ("UTWIG_HOME_VISITS", 3), ("BOMB_VISITS", 3), ("ULTRON_CONDITION", 3), ("UTWIG_HAVE_ULTRON", 1),
    ("BOMB_UNPROTECTED", 1), ("TAALO_UNPROTECTED", 1), ("TALKING_PET_VISITS", 3),
    ("TALKING_PET_HOME_VISITS", 3), ("UMGAH_ZOMBIE_BLOBBIES", 1), ("KNOW_UMGAH_ZOMBIES", 1),
    ("ARILOU_VISITS", 3), ("ARILOU_HOME_VISITS", 3), ("KNOW_ARILOU_WANT_WRECK", 1),
    ("ARILOU_CHECKED_UMGAH", 2), ("PORTAL_SPAWNER", 1), ("PORTAL_SPAWNER_ON_SHIP", 1),
    ("UMGAH_VISITS", 3), ("UMGAH_HOME_VISITS", 3), ("MET_NORMAL_UMGAH", 1), ("SYREEN_HOME_VISITS", 3),
    ("SYREEN_SHUTTLE_ON_SHIP", 1), ("KNOW_SYREEN_VAULT", 1), ("EGG_CASE0_ON_SHIP", 1),
    ("SUN_DEVICE_UNGUARDED", 1), ("ROSY_SPHERE_ON_SHIP", 1), ("CHMMR_HOME_VISITS", 3),
    ("CHMMR_EMERGING", 1), ("CHMMR_UNLEASHED", 1), ("CHMMR_BOMB_STATE", 2), ("DRUUGE_DISCLAIMER", 1),
    ("YEHAT_VISITS", 3), ("YEHAT_REBEL_VISITS", 3), ("YEHAT_HOME_VISITS", 3), ("YEHAT_CIVIL_WAR", 1),
    ("YEHAT_ABSORBED_PKUNK", 1), ("YEHAT_SHIP_MONTH", 4), ("YEHAT_SHIP_DAY", 5),
    ("YEHAT_SHIP_YEAR", 5), ("CLEAR_SPINDLE", 1), ("PKUNK_VISITS", 3), ("PKUNK_HOME_VISITS", 3),
    ("PKUNK_SHIP_MONTH", 4), ("PKUNK_SHIP_DAY", 5), ("PKUNK_SHIP_YEAR", 5), ("PKUNK_MISSION", 3),
    ("SUPOX_VISITS", 3), ("SUPOX_HOME_VISITS", 3), ("THRADD_VISITS", 3), ("THRADD_HOME_VISITS", 3),
    ("HELIX_VISITS", 3), ("HELIX_UNPROTECTED", 1), ("THRADD_CULTURE", 2), ("THRADD_MISSION", 3),
    ("DRUUGE_VISITS", 3), ("DRUUGE_HOME_VISITS", 3), ("ROSY_SPHERE", 1), ("SCANNED_MAIDENS", 1),
    ("SCANNED_FRAGMENTS", 1), ("SCANNED_CASTER", 1), ("SCANNED_SPAWNER", 1), ("SCANNED_ULTRON", 1),
    ("ZOQFOT_INFO", 2), ("ZOQFOT_HOSTILE", 1), ("ZOQFOT_HOME_VISITS", 3), ("MET_ZOQFOT", 1),
    ("ZOQFOT_DISTRESS", 2), ("EGG_CASE1_ON_SHIP", 1), ("EGG_CASE2_ON_SHIP", 1),
    ("MYCON_SUN_VISITS", 3), ("ORZ_HOME_VISITS", 3), ("MELNORME_FUEL_PROCEDURE", 1),
    ("MELNORME_TECH_PROCEDURE", 1), ("MELNORME_INFO_PROCEDURE", 1), ("MELNORME_TECH_STACK", 4),
    ("MELNORME_EVENTS_INFO_STACK", 5), ("MELNORME_ALIEN_INFO_STACK", 5),
    ("MELNORME_HISTORY_INFO_STACK", 5), ("RAINBOW_WORLD0", 8), ("RAINBOW_WORLD1", 2),
    ("MELNORME_RAINBOW_COUNT", 4), ("USED_BROADCASTER", 1), ("BROADCASTER_RESPONSE", 1),
    ("IMPROVED_LANDER_SPEED", 1), ("IMPROVED_LANDER_CARGO", 1), ("IMPROVED_LANDER_SHOT", 1),
    ("MET_ORZ_BEFORE", 1), ("YEHAT_REBEL_TOLD_PKUNK", 1), ("PLAYER_HAD_SEX", 1),
    ("UMGAH_BROADCASTERS_ON_SHIP", 1), ("LIGHT_MINERAL_LOAD", 3), ("MEDIUM_MINERAL_LOAD", 3),
    ("HEAVY_MINERAL_LOAD", 3), ("STARBASE_BULLETS", 32), ("STARBASE_MONTH", 4), ("STARBASE_DAY", 5),
    ("CREW_SOLD_TO_DRUUGE0", 8), ("CREW_PURCHASED0", 8), ("CREW_PURCHASED1", 8),
    ("URQUAN_PROTECTING_SAMATRA", 1), ("THRADDASH_BODY_COUNT", 5), ("UTWIG_SUPOX_MISSION", 3),
    ("SPATHI_INFO", 3), ("ILWRATH_INFO", 2), ("ILWRATH_GODS_SPOKEN", 4), ("ILWRATH_WORSHIP", 2),
    ("ILWRATH_FIGHT_THRADDASH", 1), ("READY_TO_CONFUSE_URQUAN", 1), ("URQUAN_HYPNO_VISITS", 1),
    ("MENTIONED_PET_COMPULSION", 1), ("URQUAN_INFO", 2), ("KNOW_URQUAN_STORY", 2), ("MYCON_INFO", 4),
    ("MYCON_RAMBLE", 5), ("KNOW_ABOUT_SHATTERED", 2), ("MYCON_INSULTS", 3), ("MYCON_KNOW_AMBUSH", 1),
    ("SYREEN_INFO", 2), ("KNOW_SYREEN_WORLD_SHATTERED", 1), ("SYREEN_KNOW_ABOUT_MYCON", 1),
    ("TALKING_PET_INFO", 3), ("TALKING_PET_SUGGESTIONS", 3), ("LEARNED_TALKING_PET", 1),
    ("DNYARRI_LIED", 1), ("SHIP_TO_COMPEL", 1), ("ORZ_GENERAL_INFO", 2), ("ORZ_PERSONAL_INFO", 3),
    ("ORZ_ANDRO_STATE", 2), ("REFUSED_ORZ_ALLIANCE", 1), ("PKUNK_MANNER", 2), ("PKUNK_ON_THE_MOVE", 1),
    ("PKUNK_FLEET", 2), ("PKUNK_MIGRATE", 2), ("PKUNK_RETURN", 1), ("PKUNK_WORRY", 2),
    ("PKUNK_INFO", 3), ("PKUNK_WAR", 2), ("PKUNK_FORTUNE", 3), ("PKUNK_MIGRATE_VISITS", 3),
    ("PKUNK_REASONS", 4), ("PKUNK_SWITCH", 1), ("PKUNK_SENSE_VICTOR", 1), ("KOHR_AH_REASONS", 2),
    ("KOHR_AH_PLEAD", 2), ("KOHR_AH_INFO", 2), ("KNOW_KOHR_AH_STORY", 2), ("KOHR_AH_SENSES_EVIL", 1),
    ("URQUAN_SENSES_EVIL", 1), ("SLYLANDRO_PROBE_VISITS", 3), ("SLYLANDRO_PROBE_THREAT", 2),
    ("SLYLANDRO_PROBE_WRONG", 2), ("SLYLANDRO_PROBE_ID", 2), ("SLYLANDRO_PROBE_INFO", 2),
    ("SLYLANDRO_PROBE_EXIT", 2), ("UMGAH_HOSTILE", 1), ("UMGAH_EVIL_BLOBBIES", 1),
    ("UMGAH_MENTIONED_TRICKS", 2), ("BOMB_CARRIER", 1), ("THRADD_MANNER", 1), ("THRADD_INTRO", 2),
    ("THRADD_DEMEANOR", 3), ("THRADD_INFO", 2), ("THRADD_BODY_LEVEL", 2), ("THRADD_MISSION_VISITS", 1),
    ("THRADD_STACK_1", 3), ("THRADD_HOSTILE_STACK_2", 1), ("THRADD_HOSTILE_STACK_3", 1),
    ("THRADD_HOSTILE_STACK_4", 1), ("THRADD_HOSTILE_STACK_5", 1), ("CHMMR_STACK", 2),
    ("ARILOU_MANNER", 2), ("NO_PORTAL_VISITS", 1), ("ARILOU_STACK_1", 2), ("ARILOU_STACK_2", 1),
    ("ARILOU_STACK_3", 2), ("ARILOU_STACK_4", 1), ("ARILOU_STACK_5", 2), ("ARILOU_INFO", 2),
    ("ARILOU_HINTS", 2), ("DRUUGE_MANNER", 1), ("DRUUGE_SPACE_INFO", 2), ("DRUUGE_HOME_INFO", 2),
    ("DRUUGE_SALVAGE", 1), ("KNOW_DRUUGE_SLAVERS", 2), ("FRAGMENTS_BOUGHT", 2), ("ZEX_STACK_1", 2),
    ("ZEX_STACK_2", 2), ("ZEX_STACK_3", 2), ("VUX_INFO", 2), ("VUX_STACK_1", 4), ("VUX_STACK_2", 2),
    ("VUX_STACK_3", 2), ("VUX_STACK_4", 2), ("SHOFIXTI_STACK4", 2), ("YEHAT_REBEL_INFO", 3),
    ("YEHAT_ROYALIST_INFO", 1), ("YEHAT_ROYALIST_TOLD_PKUNK", 1), ("NO_YEHAT_ALLY_HOME", 1),
    ("NO_YEHAT_HELP_HOME", 1), ("NO_YEHAT_INFO", 1), ("NO_YEHAT_ALLY_SPACE", 2),
    ("NO_YEHAT_HELP_SPACE", 2), ("ZOQFOT_KNOW_MASK", 4), ("SUPOX_HOSTILE", 1), ("SUPOX_INFO", 1),
    ("SUPOX_WAR_NEWS", 2), ("SUPOX_ULTRON_HELP", 1), ("SUPOX_STACK1", 3), ("SUPOX_STACK2", 2),
    ("UTWIG_HOSTILE", 1), ("UTWIG_INFO", 1), ("UTWIG_WAR_NEWS", 2), ("UTWIG_STACK1", 3),
    ("UTWIG_STACK2", 2), ("BOMB_INFO", 1), ("BOMB_STACK1", 2), ("BOMB_STACK2", 2),
    ("SLYLANDRO_KNOW_BROKEN", 1), ("PLAYER_KNOWS_PROBE", 1), ("PLAYER_KNOWS_PROGRAM", 1),
    ("PLAYER_KNOWS_EFFECTS", 1), ("PLAYER_KNOWS_PRIORITY", 1), ("SLYLANDRO_STACK1", 3),
    ("SLYLANDRO_STACK2", 1), ("SLYLANDRO_STACK3", 2), ("SLYLANDRO_STACK4", 2), ("SLYLANDRO_STACK5", 1),
    ("SLYLANDRO_STACK6", 1), ("SLYLANDRO_STACK7", 2), ("SLYLANDRO_STACK8", 2), ("SLYLANDRO_STACK9", 2),
    ("SLYLANDRO_KNOW_EARTH", 1), ("SLYLANDRO_KNOW_EXPLORE", 1), ("SLYLANDRO_KNOW_GATHER", 1),
    ("SLYLANDRO_KNOW_URQUAN", 2), ("RECALL_VISITS", 2), ("SLYLANDRO_MULTIPLIER", 3),
    ("KNOW_SPATHI_QUEST", 1), ("KNOW_SPATHI_EVIL", 1), ("BATTLE_PLANET", 8), ("ESCAPE_COUNTER", 8),
    ("CREW_SOLD_TO_DRUUGE1", 8), ("PKUNK_DONE_WAR", 1), ("SYREEN_STACK0", 2), ("SYREEN_STACK1", 2),
    ("SYREEN_STACK2", 2), ("REFUSED_ULTRON_AT_BOMB", 1), ("NO_TRICK_AT_SUN", 1), ("SPATHI_STACK0", 2),
    ("SPATHI_STACK1", 1), ("SPATHI_STACK2", 1), ("ORZ_STACK0", 1), ("ORZ_STACK1", 1),
    ("SHOFIXTI_GRPOFFS", 32), ("ZOQFOT_GRPOFFS", 32), ("MELNORME0_GRPOFFS", 32),
    ("MELNORME1_GRPOFFS", 32), ("MELNORME2_GRPOFFS", 32), ("MELNORME3_GRPOFFS", 32),
    ("MELNORME4_GRPOFFS", 32), ("MELNORME5_GRPOFFS", 32), ("MELNORME6_GRPOFFS", 32),
    ("MELNORME7_GRPOFFS", 32), ("MELNORME8_GRPOFFS", 32), ("URQUAN_PROBE_GRPOFFS", 32),
    ("COLONY_GRPOFFS", 32), ("SAMATRA_GRPOFFS", 32), (None, 1), ("AUTOPILOT_OK", 1),
    ("KNOW_QS_PORTAL", 16), (None, 2), ("SYS_VISITED_00", 32), ("SYS_VISITED_01", 32),
    ("SYS_VISITED_02", 32), ("SYS_VISITED_03", 32), ("SYS_VISITED_04", 32), ("SYS_VISITED_05", 32),
    ("SYS_VISITED_06", 32), ("SYS_VISITED_07", 32), ("SYS_VISITED_08", 32), ("SYS_VISITED_09", 32),
    ("SYS_VISITED_10", 32), ("SYS_VISITED_11", 32), ("SYS_VISITED_12", 32), ("SYS_VISITED_13", 32),
    ("SYS_VISITED_14", 32), ("SYS_VISITED_15", 32), ("KNOW_HOMEWORLD", 18), ("HM_ENCOUNTERS", 9),
    ("RESERVED", 32), (None, 3), ("SYS_PLYR_MARKER_00", 32), ("SYS_PLYR_MARKER_01", 32),
    ("SYS_PLYR_MARKER_02", 32), ("SYS_PLYR_MARKER_03", 32), ("SYS_PLYR_MARKER_04", 32),
    ("SYS_PLYR_MARKER_05", 32), ("SYS_PLYR_MARKER_06", 32), ("SYS_PLYR_MARKER_07", 32),
    ("SYS_PLYR_MARKER_08", 32), ("SYS_PLYR_MARKER_09", 32), ("SYS_PLYR_MARKER_10", 32),
    ("SYS_PLYR_MARKER_11", 32), ("SYS_PLYR_MARKER_12", 32), ("SYS_PLYR_MARKER_13", 32),
    ("SYS_PLYR_MARKER_14", 32), ("SYS_PLYR_MARKER_15", 32), ("LAST_LOCATION_X", 16),
    ("LAST_LOCATION_Y", 16), (None, 4), ("ADV_AUTOPILOT_SAVE_X", 16), ("ADV_AUTOPILOT_SAVE_Y", 16),
    ("ADV_AUTOPILOT_QUASI_X", 16), ("ADV_AUTOPILOT_QUASI_Y", 16), (None, 5), ("SEED_TYPE", 2),
    ("SUPOX_SHIP_MONTH", 4), ("SUPOX_SHIP_DAY", 5), ("SUPOX_SHIP_YEAR", 5), ("UTWIG_SHIP_MONTH", 4),
    ("UTWIG_SHIP_DAY", 5), ("UTWIG_SHIP_YEAR", 5), ("REV_5_PAD", 1), (None, 0),
]


class SaveError(Exception):
    pass


class Reader:
    """Reads little-endian values from a byte buffer.

    'wide' is for the MMV4 saves that the game wrote from March to October 2026.
    Some of their values are wider than usual, and the names have no padding.
    """

    def __init__(self, data, wide):
        self.data = data
        self.pos = 0
        self.wide = wide

    def at_end(self):
        return self.pos >= len(self.data)

    def bytes(self, count):
        if self.pos + count > len(self.data):
            raise SaveError(f"unexpected end of file at byte {self.pos}")
        result = self.data[self.pos:self.pos + count]
        self.pos += count
        return result

    def value(self, fmt):
        size = struct.calcsize(fmt)
        return struct.unpack(fmt, self.bytes(size))[0]

    def u8(self):
        return self.value("<B")

    def u16(self):
        return self.value("<H")

    def s16(self):
        return self.value("<h")

    def u32(self):
        return self.value("<I")

    def size(self):
        return self.value("<Q") if self.wide else self.u32()

    def s32(self):
        return self.value("<i")

    def u8_list(self, count):
        return list(self.bytes(count))

    def u16_list(self, count):
        return [self.u16() for _ in range(count)]

    def string(self, size):
        raw = self.bytes(size).split(b"\0", 1)[0]
        return raw.decode("utf-8", errors="replace")

    def point(self):
        return {"x": self.s16(), "y": self.s16()}

    def shifted16(self):
        """Reads a 16-bit value that was shifted before it was saved."""
        return self.s32() if self.wide else self.s16()

    def shifted_point(self):
        return {"x": self.shifted16(), "y": self.shifted16()}


def cdiv(a, b):
    """Integer division that rounds toward zero, as in C."""
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


# Core UQM stored hyperspace coordinates with other unit factors.
# These give the 1x logical coordinates.
def core_log_x(lx):
    universe = cdiv(lx * 6250 + 49920, 99840)
    return cdiv(universe * 10000 + 312, 625)


def core_log_y(ly):
    universe = 9999 - cdiv(ly * 625 + 6000, 12000)
    return cdiv((9999 - universe) * 10000 + 312, 625)


def bits_for_revision(rev):
    bits = 0
    for name, count in GAME_STATE_BIT_MAP:
        if name is not None:
            bits += count
        elif count == 0 or (rev >= 0 and count > rev):
            break
    return bits


def revision_for_bytes(byte_count):
    rev = 0
    bits = 0
    for name, count in GAME_STATE_BIT_MAP:
        if name is not None:
            bits += count
        elif count == 0 or (bits + 7) >> 3 >= byte_count:
            break
        else:
            rev = count
    return rev if (bits + 7) >> 3 == byte_count else -1


def decode_game_state(data, rev):
    stream = int.from_bytes(data, "little")
    bit_pos = 0
    in_revision = True
    values = {}
    for name, count in GAME_STATE_BIT_MAP:
        if name is not None:
            value = 0
            if in_revision:
                value = (stream >> bit_pos) & ((1 << count) - 1)
                bit_pos += count
            values[name] = value
        elif count == 0:
            break
        elif count > rev:
            in_revision = False
    return values


def read_ship_fragment(r):
    return {
        "captains_name_index": r.u8(),
        "race_id": r.u8(),
        "index": r.u8(),
        "crew_level": r.u16(),
        "max_crew": r.u16(),
        "energy_level": r.u8(),
        "max_energy": r.u8(),
    }


def read_ip_group(r):
    return {
        "group_counter": r.u16(),
        "race_id": r.u8(),
        "sys_loc": r.u8(),
        "task": r.u8(),
        "in_system": r.u8(),
        "dest_loc": r.u8(),
        "orbit_pos": r.u8(),
        "group_id": r.u8(),
        "loc": r.point(),
    }


def read_sis(r, core, legacy_mm, packed_names_size=None):
    """'packed_names_size' is the size of the three names when they have no padding."""
    name_size = LEGACY_SIS_NAME_SIZE if core or legacy_mm == 1 else SIS_NAME_SIZE
    log_x = r.s32()
    log_y = r.s32()
    if core:
        log_x = core_log_x(log_x)
        log_y = core_log_y(log_y)
    sis = {
        "log": {"x": log_x, "y": log_y},
        "ResUnits": r.u32(),
        "FuelOnBoard": r.u32(),
        "CrewEnlisted": r.u16(),
        "TotalElementMass": r.u16(),
        "TotalBioMass": r.u16(),
        "ModuleSlots": r.u8_list(NUM_MODULE_SLOTS),
        "DriveSlots": r.u8_list(NUM_DRIVE_SLOTS),
        "JetSlots": r.u8_list(NUM_JET_SLOTS),
        "NumLanders": r.u8(),
        "ElementAmounts": r.u16_list(NUM_ELEMENT_CATEGORIES),
        "ShipName": "",
        "CommanderName": "",
        "PlanetName": "",
        "Difficulty": 0,
        "Extended": 0,
        "Nomad": 0,
        "Seed": 0,
        "ShipSeed": 0,
    }
    if packed_names_size is None:
        for key in ("ShipName", "CommanderName", "PlanetName"):
            sis[key] = r.string(name_size)
    else:
        sis["PlanetName"] = r.string(packed_names_size)
    if not core:
        sis["Difficulty"] = r.u8()
        sis["Extended"] = r.u8()
        sis["Nomad"] = r.u8()
        sis["Seed"] = r.s32()
        if legacy_mm == 0:
            sis["ShipSeed"] = r.u8()
    return sis


def read_summary_fields(r, core, size):
    summary = {
        "Activity": r.u8(),
        "Flags": r.u8(),
        "day_index": r.u8(),
        "month_index": r.u8(),
        "year_index": r.u16(),
        "MCreditLo": r.u8(),
        "MCreditHi": r.u8(),
    }
    num_ships = r.u8()
    num_devices = r.u8()
    summary["ShipList"] = r.u8_list(MAX_BUILT_SHIPS)[:num_ships]
    summary["DeviceList"] = r.u8_list(MAX_EXCLUSIVE_DEVICES)[:num_devices]
    summary["res_factor"] = 0 if core else r.u8()
    name = r.bytes(size - 160)[:SAVE_NAME_SIZE - 1]
    summary["SaveName"] = name.split(b"\0", 1)[0].decode("utf-8", errors="replace")
    return summary


def read_summary(r):
    magic = r.u32()
    if magic == CORE_TAG:
        core, legacy_mm = True, 0
    elif magic in (MMV4_TAG, MMV3_TAG, MEGA_TAG):
        core, legacy_mm = False, {MMV4_TAG: 0, MEGA_TAG: 1, MMV3_TAG: 2}[magic]
    else:
        raise SaveError("not a UQM save game")
    if r.u32() != SUMMARY_TAG:
        raise SaveError("the summary chunk is missing")
    if r.wide and magic != MMV4_TAG:
        raise SaveError("not a UQM save game")
    size = r.size()
    if size < 160:
        raise SaveError("the summary chunk is too small")

    packed_names_size = None
    if r.wide:
        # The names have no padding, so their total size comes from the
        # position of the next chunk.
        end = r.data.find(struct.pack("<II", GLOBAL_STATE_TAG, 75), r.pos)
        if end < 0:
            raise SaveError("the global state chunk is missing")
        fixed_size = 74 + 8 + 39 + (size - 160)
        packed_names_size = end - r.pos - fixed_size
        if packed_names_size < 0:
            raise SaveError("the summary chunk is too small")
    sis = read_sis(r, core, legacy_mm, packed_names_size)
    summary = read_summary_fields(r, core, size)
    if r.wide and packed_names_size:
        print(f"  The names have no padding. '{sis['PlanetName']}' is now the planet name, "
              "and the ship and commander names are empty. Edit them in the JSON file "
              "if necessary.", file=sys.stderr)

    summary["sis"] = sis
    if core:
        summary["coreSave"] = True
    return summary, core


def read_global_state(r, core):
    if r.u32() != GLOBAL_STATE_TAG or r.u32() != 75:
        raise SaveError("the global state chunk is missing")
    state = {
        "glob_flags": r.u8(),
        "CrewCost": r.u8(),
        "FuelCost": r.u8(),
        "ModuleCost": r.u8_list(NUM_MODULES),
        "ElementWorth": r.u8_list(NUM_ELEMENT_CATEGORIES),
        "CurrentActivity": r.u16(),
        "GameClock": {
            "day_index": r.u8(),
            "month_index": r.u8(),
            "year_index": r.u16(),
            "tick_count": r.s16(),
            "day_in_ticks": r.s16(),
        },
        "autopilot": r.point(),
        "ip_location": r.point(),
        "ShipOrigin": r.shifted_point(),
        "ShipFacing": r.u16(),
        "ip_planet": r.u8(),
        "in_orbit": r.u8(),
        "velocity": {
            "TravelAngle": r.s32() if r.wide else r.u16(),
            "vector": r.shifted_point(),
            "fract": r.shifted_point(),
            "error": r.shifted_point(),
            "incr": r.shifted_point(),
        },
    }

    if r.u32() != GAME_STATE_TAG:
        raise SaveError("the game state chunk is missing")
    size = r.size()
    rev = 0 if core else revision_for_bytes(size)
    byte_count = (bits_for_revision(rev) + 7) >> 3
    if rev < 0 or size < byte_count:
        raise SaveError("the game state chunk is too small")
    flags = decode_game_state(r.bytes(byte_count), rev)
    r.bytes(size - byte_count)

    if rev < 2:
        state["glob_flags"] = NUM_READ_SPEEDS >> 1
    if rev < 3:
        flags["LAST_LOCATION_X"] = NO_LOCATION
        flags["LAST_LOCATION_Y"] = NO_LOCATION
    if rev < 4:
        for name in ("ADV_AUTOPILOT_SAVE_X", "ADV_AUTOPILOT_SAVE_Y",
                     "ADV_AUTOPILOT_QUASI_X", "ADV_AUTOPILOT_QUASI_Y"):
            flags[name] = NO_LOCATION
    return state, flags, rev


def read_race_queue(r, size):
    fleets = []
    for _ in range(size // 30):
        fleets.append({
            "Index": r.u16(),
            "allied_state": r.u16(),
            "days_left": r.u8(),
            "growth_fract": r.u8(),
            "crew_level": r.u16(),
            "max_crew": r.u16(),
            "growth": r.u8(),
            "max_energy": r.u8(),
            "loc": r.point(),
            "actual_strength": r.u16(),
            "known_strength": r.u16(),
            "known_loc": r.point(),
            "growth_err_term": r.u8(),
            "func_index": r.u8(),
            "dest_loc": r.point(),
        })
    return fleets


def read_ship_queue(r, size):
    ships = []
    for _ in range(size // 11):
        r.u16()
        ships.append(read_ship_fragment(r))
    return ships


def read_encounter(r, core):
    encounter = {
        "transition_state": r.s16(),
        "origin": r.point(),
        "radius": r.u16(),
        "loc_pt": r.point(),
        "race_id": r.u8(),
    }
    num_ships = r.u8()
    encounter["flags"] = r.u8()
    ships = []
    for _ in range(MAX_HYPER_SHIPS):
        ships.append({
            "race_id": r.u8(),
            "crew_level": r.u16(),
            "max_crew": r.u16(),
            "max_energy": r.u8(),
        })
    encounter["ships"] = ships[:num_ships]
    log_x = r.s32()
    log_y = r.s32()
    if core:
        log_x = core_log_x(log_x)
        log_y = core_log_y(log_y)
    encounter["log"] = {"x": log_x, "y": log_y}
    return encounter


def read_event(r):
    return {
        "day_index": r.u8(),
        "month_index": r.u8(),
        "year_index": r.u16(),
        "func_index": r.u8(),
    }


def read_group_list(r, size):
    group_list = {"LastEnc": r.u8(), "groups": []}
    for _ in range((size - 1) // 14):
        group = {"race_outer": r.u8()}
        group.update(read_ip_group(r))
        group_list["groups"].append(group)
    return group_list


def read_battle_group(r):
    battle_group = {
        "encounter": r.u32(),
        "current": (r.u32() if r.wide else r.u8()) != 0,
        "star_index": r.u16(),
        "day_index": r.u8(),
        "month_index": r.u8(),
        "year_index": r.u16(),
        "groups": [],
    }
    for _ in range(r.u8()):
        group = {"icon": r.u8(), "ships": []}
        for _ in range(r.u8()):
            ship = {"race_outer": r.u8()}
            ship.update(read_ship_fragment(r))
            group["ships"].append(ship)
        battle_group["groups"].append(group)
    return battle_group


def read_star_desc(r):
    return {
        "star_pt": r.point(),
        "Type": r.u8(),
        "Index": r.u8(),
        "Prefix": r.u8(),
        "Postfix": r.u8(),
    }


def convert(data):
    """Return the JSON save document for the binary save game in 'data'."""
    try:
        return convert_layout(Reader(data, wide=False))
    except SaveError as error:
        try:
            return convert_layout(Reader(data, wide=True))
        except SaveError:
            raise error from None


def convert_layout(r):
    summary, core = read_summary(r)
    state, flags, _ = read_global_state(r, core)

    save = {"version": SAVE_FORMAT_VERSION, "summary": summary,
            "globalState": state, "gameState": flags}
    queues = {}
    battle_groups = []
    while not r.at_end():
        chunk = r.u32()
        size = r.u32()
        start = r.pos
        if chunk == RACE_Q_TAG:
            queues["raceQueue"] = read_race_queue(r, size)
        elif chunk == NPC_SHIP_Q_TAG:
            queues["npcShipQueue"] = read_ship_queue(r, size)
        elif chunk == IP_GRP_Q_TAG:
            queues["ipGroupQueue"] = [read_ip_group(r) for _ in range(size // 13)]
        elif chunk == SHIP_Q_TAG:
            queues["shipQueue"] = read_ship_queue(r, size)
        elif chunk == STOWED_Q_TAG:
            queues["stowedShipQueue"] = read_ship_queue(r, size)
        elif chunk == EVENTS_TAG:
            queues["events"] = [read_event(r) for _ in range(size // 5)]
        elif chunk == ENCOUNTERS_TAG:
            queues["encounters"] = [read_encounter(r, core) for _ in range(size // 65)]
        elif chunk == SCAN_TAG:
            queues["starInfo"] = [r.u32() for _ in range(size // 4)]
        elif chunk == GROUP_LIST_TAG:
            queues["groupList"] = read_group_list(r, size)
        elif chunk == BATTLE_GROUP_TAG:
            battle_groups.append(read_battle_group(r))
        elif chunk == STAR_TAG:
            queues["starDesc"] = read_star_desc(r)
        else:
            print(f"  skipping unknown chunk {chunk:08X} ({size} bytes)", file=sys.stderr)
            r.bytes(size)
        expected_size = size + (3 if r.wide and chunk == BATTLE_GROUP_TAG else 0)
        if r.pos != start + expected_size:
            raise SaveError(f"chunk {chunk:08X} has the wrong size")

    if battle_groups:
        queues["battleGroups"] = battle_groups
    if "starDesc" not in queues:
        raise SaveError("the star chunk is missing")

    order = ["raceQueue", "npcShipQueue", "ipGroupQueue", "shipQueue",
             "stowedShipQueue", "events", "encounters", "starInfo",
             "groupList", "battleGroups", "starDesc"]
    for key in order:
        if key in queues:
            save[key] = queues[key]
    return save


def save_files(paths):
    for path in paths:
        if path.is_dir():
            yield from sorted(p for p in path.iterdir() if SAVE_NAME.fullmatch(p.name))
        else:
            yield path


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("paths", nargs="*", type=Path, default=[DEFAULT_SAVE_DIR],
                        help=f"save files or folders (default: {DEFAULT_SAVE_DIR})")
    parser.add_argument("-o", "--output-dir", type=Path,
                        help="write the JSON files here (default: next to each save)")
    parser.add_argument("-f", "--force", action="store_true",
                        help="replace JSON files that already exist")
    args = parser.parse_args()

    failed = False
    for path in save_files(args.paths):
        out_dir = args.output_dir or path.parent
        out_path = out_dir / (path.name + ".json")
        if out_path.exists() and not args.force:
            print(f"{path}: {out_path.name} exists, skipped (use --force to replace)")
            continue
        try:
            save = convert(path.read_bytes())
        except (OSError, SaveError) as error:
            print(f"{path}: {error}", file=sys.stderr)
            failed = True
            continue
        out_dir.mkdir(parents=True, exist_ok=True)
        out_path.write_text(json.dumps(save, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        print(f"{path} -> {out_path}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
