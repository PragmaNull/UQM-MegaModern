//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <assert.h>

#include "save.h"

#include "build.h"
#include "controls.h"
#include "starmap.h"
#include "encount.h"
#include "gamestr.h"
#include "globdata.h"
#include "options.h"
#include "races.h"
#include "shipcont.h"
#include "setup.h"
#include "state.h"
#include "grpintrn.h"
#include "util.h"
#include "hyper.h"
// for SaveSisHyperState()
#include "planets/planets.h"
// for SaveSolarSysLocation() and tests
#include "libs/inplib.h"
#include "core/log/log.h"
#include "libs/memlib.h"
#include "colors.h"
#include "libs/file/textfile.h"

#include <yyjson.h>

#pragma clang optimize off

// This defines the order and the number of bits in which the game state
// properties are saved.
const GameStateBitMap gameStateBitMap[] = {
	/* Shofixti states */
	{"SHOFIXTI_VISITS",				3 },
	{"SHOFIXTI_STACK1",				2 },
	{"SHOFIXTI_STACK2",				3 },
	{"SHOFIXTI_STACK3",				2 },
	{"SHOFIXTI_KIA",				 1 },
	{"SHOFIXTI_BRO_KIA",			 1 },
	{"SHOFIXTI_RECRUITED",		   1 },

	{"SHOFIXTI_MAIDENS",			 1 }, /* Did you find the babes yet? */
	{"MAIDENS_ON_SHIP",				1 },
	{"BATTLE_SEGUE",				 1 },
	/* Set to 0 in init_xxx_comm() if communications directly
			 * follows an encounter.
			 * Set to 1 in init_xxx_comm() if the player gets to decide
			 * whether to attack or talk.
			 * Set to 1 in communication when battle follows the
			 * communication. It is still valid when uninit_xxx_comm() gets
			 * called after combat or communication.
			 */
	{"PLANETARY_LANDING",			  1 },
	{"PLANETARY_CHANGE",			 1 },
	/* Flag set to 1 when the planet information for the current
			 * world is changed since it was last saved to the starinfo.dat
			 * file. Set when picking up bio, mineral, or energy nodes.
			 * When there's no current world, it should be 0.
			 */

	/* Spathi states */
	{"SPATHI_VISITS",				  3 },
	{"SPATHI_HOME_VISITS",		   3 },
	{"FOUND_PLUTO_SPATHI",		   2 },
	/* 0 - Haven't met Fwiffo.
			 * 1 - Met Fwiffo on Pluto, now talking to him.
			 * 2 - Met Fwiffo on Pluto, after dialog.
			 * 3 - Met Fwiffo, and have reported to the Safe Ones on
			 *     the Spathi moon that he was either killed, or that
			 *     you have him on board.
			 */
	{"SPATHI_SHIELDED_SELVES",	   1 },
	{"SPATHI_CREATURES_EXAMINED",	  1 },
	{"SPATHI_CREATURES_ELIMINATED", 1 },
	{"UMGAH_BROADCASTERS",		   1 },
	{"SPATHI_MANNER",				  2 },
	{"SPATHI_QUEST",				 1 },
	{"LIED_ABOUT_CREATURES",		 2 },
	{"SPATHI_PARTY",				 1 },
	{"KNOW_SPATHI_PASSWORD",		 1 },

	{"ILWRATH_HOME_VISITS",			3 },
	{"ILWRATH_CHMMR_VISITS",		 1 },

	{"ARILOU_SPACE",				 1 },
	/* 0 if the periodically opening QuasiSpace portal is
			 * closed or closing.
			 * 1 if the periodically opening QuasiSpace portal is
			 * open or opening.
			 */
	{"ARILOU_SPACE_SIDE",			  2 },
	/* 0 if in HyperSpace and not just emerged from the
			 * periodically opening QuasiSpace portal.
			 * 1 if in HyperSpace and just emerged from the periodically
			 * QuasiSpace portal (still on the portal).
			 * 2 if in QuasiSpace and just emerged from the periodically
			 * opening portal (still on the portal).
			 * 3 if in QuasiSpace and not just emerged from the
			 * periodically opening portal.
			 */
	{"ARILOU_SPACE_COUNTER",		 4 },
	/* Keeps track of how far the periodically opening QuasiSpace
			 * portal is open. (This determines the image)
			 * 0 <= ARILOU_SPACE_COUNTER <= 9
			 * 0 means totally closed.
			 * 9 means completely open.
			 */

	{"LANDER_SHIELDS",			   4 },

	{"MET_MELNORME",				 1 },
	{"MELNORME_RESCUE_REFUSED",		1 },
	{"MELNORME_RESCUE_COUNT",		  3 },
	{"TRADED_WITH_MELNORME",		 1 },
	{"WHY_MELNORME_PURPLE",			1 },
	{"MELNORME_CREDIT0",			 8 },
	{"MELNORME_CREDIT1",			 8 },
	{"MELNORME_BUSINESS_COUNT",		2 },
	{"MELNORME_YACK_STACK0",		 2 },
	{"MELNORME_YACK_STACK1",		 2 },
	{"MELNORME_YACK_STACK2",		 4 },
	{"MELNORME_YACK_STACK3",		 3 },
	{"MELNORME_YACK_STACK4",		 2 },
	{"WHY_MELNORME_BLUE",			  1 },
	{"MELNORME_ANGER",			   2 },
	{"MELNORME_MIFFED_COUNT",		  2 },
	{"MELNORME_PISSED_COUNT",		  2 },
	{"MELNORME_HATE_COUNT",			2 },

	{"PROBE_MESSAGE_DELIVERED",		1 },
	{"PROBE_ILWRATH_ENCOUNTER",		1 },

	{"STARBASE_AVAILABLE",		   1 },
	{"STARBASE_VISITED",			 1 },
	{"RADIOACTIVES_PROVIDED",		  1 },
	{"LANDERS_LOST",				 1 },
	{"GIVEN_FUEL_BEFORE",			  1 },

	{"AWARE_OF_SAMATRA",			 1 },
	{"YEHAT_CAVALRY_ARRIVED",		  1 },
	{"URQUAN_MESSED_UP",			 1 },

	{"MOONBASE_DESTROYED",		   1 },
	{"WILL_DESTROY_BASE",			  1 },

	{"WIMBLIS_TRIDENT_ON_SHIP",		1 },
	{"GLOWING_ROD_ON_SHIP",			1 },

	{"KOHR_AH_KILLED_ALL",		   1 },

	{"STARBASE_YACK_STACK1",		 1 },

	{"DISCUSSED_PORTAL_SPAWNER",	 1 },
	{"DISCUSSED_TALKING_PET",		  1 },
	{"DISCUSSED_UTWIG_BOMB",		 1 },
	{"DISCUSSED_SUN_EFFICIENCY",	 1 },
	{"DISCUSSED_ROSY_SPHERE",		  1 },
	{"DISCUSSED_AQUA_HELIX",		 1 },
	{"DISCUSSED_CLEAR_SPINDLE",		1 },
	{"DISCUSSED_ULTRON",			 1 },
	{"DISCUSSED_MAIDENS",			  1 },
	{"DISCUSSED_UMGAH_HYPERWAVE",	  1 },
	{"DISCUSSED_BURVIX_HYPERWAVE",  1 },
	{"SYREEN_WANT_PROOF",			  1 },
	{"PLAYER_HAVING_SEX",			  1 },
	{"MET_ARILOU",				   1 },
	{"DISCUSSED_TAALO_PROTECTOR",	  1 },
	{"DISCUSSED_EGG_CASING0",		  1 },
	{"DISCUSSED_EGG_CASING1",		  1 },
	{"DISCUSSED_EGG_CASING2",		  1 },
	{"DISCUSSED_SYREEN_SHUTTLE",	 1 },
	{"DISCUSSED_VUX_BEAST",			1 },
	{"DISCUSSED_DESTRUCT_CODE",		1 },
	{"DISCUSSED_URQUAN_WARP",		  1 },
	{"DISCUSSED_WIMBLIS_TRIDENT",	  1 },
	{"DISCUSSED_GLOWING_ROD",		  1 },

	{"ATTACKED_DRUUGE",				1 },

	{"NEW_ALLIANCE_NAME",			  2 },

	{"PORTAL_COUNTER",			   4 },
	/* Set to 1 when the player opens a QuasiSpace portal.
			 * It will then be increased to 10, at which time
			 * the portal is completely open. (This determines the image).
			 */

	{"BURVIXESE_BROADCASTERS",	   1 },
	{"BURV_BROADCASTERS_ON_SHIP",	  1 },

	{"UTWIG_BOMB",				   1 },
	{"UTWIG_BOMB_ON_SHIP",		   1 },

	{"AQUA_HELIX",				   1 },
	{"AQUA_HELIX_ON_SHIP",		   1 },

	{"SUN_DEVICE",				   1 },
	{"SUN_DEVICE_ON_SHIP",		   1 },

	{"TAALO_PROTECTOR",				1 },
	{"TAALO_PROTECTOR_ON_SHIP",		1 },

	{"SHIP_VAULT_UNLOCKED",			1 },
	{"SYREEN_SHUTTLE",			   1 },

	{"PORTAL_KEY",				   1 },
	{"PORTAL_KEY_ON_SHIP",		   1 },

	{"VUX_BEAST",				   1 },
	{"VUX_BEAST_ON_SHIP",			  1 },

	{"TALKING_PET",					1 },
	{"TALKING_PET_ON_SHIP",			1 },

	{"MOONBASE_ON_SHIP",			 1 },

	{"KOHR_AH_FRENZY",			   1 },
	{"KOHR_AH_VISITS",			   2 },
	{"KOHR_AH_BYES",				 1 },

	{"SLYLANDRO_HOME_VISITS",		  3 },
	{"DESTRUCT_CODE_ON_SHIP",		  1 },

	{"ILWRATH_VISITS",			   3 },
	{"ILWRATH_DECEIVED",			 1 },
	{"FLAGSHIP_CLOAKED",			 1 },

	{"MYCON_VISITS",				 3 },
	{"MYCON_HOME_VISITS",			  3 },
	{"MYCON_AMBUSH",				 1 },
	{"MYCON_FELL_FOR_AMBUSH",		  1 },
	/* Set to 1 when the Mycon have been told about Organon
			 * and are moving towards it.
			 */

	{"GLOBAL_FLAGS_AND_DATA",		  8 },
	/* This state seems to be used to distinguish between different
			 * places where one may have an conversation with an alien.
			 * Like home world, other world, space.
			 * Why this needs 8 bits I don't know. Only specific
			 * combinations of bits seem to be used (0, 1, or all bits).
			 * A closer investigation is desirable. - SvdB
			 * Bit 4 is set when initiating communication with the Ilwrath
			 * 		homeworld by means of a HyperWave Broadcaster.
			 * Bit 5 is set when initiating communication with an Ilwrath
			 * 		ship by means of a HyperWave Broadcaster.
			 * All bits are cleared when communication is over.
			 */

	{"ORZ_VISITS",				   3 },
	{"TAALO_VISITS",				 3 },
	{"ORZ_MANNER",				   2 },

	{"PROBE_EXHIBITED_BUG",			1 },
	{"CLEAR_SPINDLE_ON_SHIP",		  1 },

	{"URQUAN_VISITS",				  3 },
	{"PLAYER_HYPNOTIZED",			  1 },

	{"VUX_VISITS",				   3 },
	{"VUX_HOME_VISITS",				3 },
	{"ZEX_VISITS",				   3 },
	{"ZEX_IS_DEAD",					1 },
	{"KNOW_ZEX_WANTS_MONSTER",	   1 },

	{"UTWIG_VISITS",				 3 },
	{"UTWIG_HOME_VISITS",			  3 },
	{"BOMB_VISITS",					3 },
	{"ULTRON_CONDITION",			 3 },
	/* 0 if the Supox still have the Ultron
			 * 1 if the Captain has the Ultron, completely broken
			 * 2 if the Captain has the Ultron, with 1 fix
			 * 3 if the Captain has the Ultron, with 2 fixes
			 * 4 if the Captain has the Ultron, completely restored
			 * 5 if the Ultron has been returned to the Utwig
			 */
	{"UTWIG_HAVE_ULTRON",			  1 },
	{"BOMB_UNPROTECTED",			 1 },

	{"TAALO_UNPROTECTED",			  1 },

	{"TALKING_PET_VISITS",		   3 },
	{"TALKING_PET_HOME_VISITS",		3 },
	{"UMGAH_ZOMBIE_BLOBBIES",		  1 },
	// The Umgah have come under the influence of the Talking Pet
	{"KNOW_UMGAH_ZOMBIES",		   1 },
	// The Captain is aware that something is up with the Umgah

	{"ARILOU_VISITS",				  3 },
	{"ARILOU_HOME_VISITS",		   3 },
	{"KNOW_ARILOU_WANT_WRECK",	   1 },
	{"ARILOU_CHECKED_UMGAH",		 2 },
	{"PORTAL_SPAWNER",			   1 },
	{"PORTAL_SPAWNER_ON_SHIP",	   1 },

	{"UMGAH_VISITS",				 3 },
	{"UMGAH_HOME_VISITS",			  3 },
	{"MET_NORMAL_UMGAH",			 1 },

	{"SYREEN_HOME_VISITS",		   3 },
	{"SYREEN_SHUTTLE_ON_SHIP",	   1 },
	{"KNOW_SYREEN_VAULT",			  1 },

	{"EGG_CASE0_ON_SHIP",			  1 },
	{"SUN_DEVICE_UNGUARDED",		 1 },

	{"ROSY_SPHERE_ON_SHIP",			1 },
	/* The Rosy Sphere is aboard the flagship, i.e. It has been
			 * acquired from the Druuge, but not yet inserted in the broken
			 * Ultron. cf. ROSY_SPHERE */

	{"CHMMR_HOME_VISITS",			  3 },
	{"CHMMR_EMERGING",			   1 },
	{"CHMMR_UNLEASHED",				1 },
	{"CHMMR_BOMB_STATE",			 2 },
	/* 0 - Nothing is known about the Precursor Bomb.
			 * 1 - The captain knows from the Chmmr that some extremely
			 *     powerful weapon is needed to destroy the Sa-Matra.
			 * 2 - Installation of the precursor bomb has started.
			 * 3 - Left the starbase after installation of the Precursor
			 * bomb.
			 */

	{"DRUUGE_DISCLAIMER",			  1 },

	{"YEHAT_VISITS",				 3 },
	{"YEHAT_REBEL_VISITS",		   3 },
	{"YEHAT_HOME_VISITS",			  3 },
	{"YEHAT_CIVIL_WAR",				1 },
	{"YEHAT_ABSORBED_PKUNK",		 1 },
	{"YEHAT_SHIP_MONTH",			 4 },
	{"YEHAT_SHIP_DAY",			   5 },
	{"YEHAT_SHIP_YEAR",				5 },

	{"CLEAR_SPINDLE",				  1 },
	{"PKUNK_VISITS",				 3 },
	{"PKUNK_HOME_VISITS",			  3 },
	{"PKUNK_SHIP_MONTH",			 4 },
	/* The month in PKUNK_SHIP_YEAR that new ships are available
			 * from the Pkunk. */
	{"PKUNK_SHIP_DAY",			   5 },
	/* The day of the month in PKUNK_SHIP_MONTH in PKUNK_SHIP_YEAR
			 * that new ships are available. */
	{"PKUNK_SHIP_YEAR",				5 },
	/* The year that new ships are available from the Pkunk
			 * (stored as an offset from the year the game starts). */
	{"PKUNK_MISSION",				  3 },

	{"SUPOX_VISITS",				 3 },
	{"SUPOX_HOME_VISITS",			  3 },

	{"THRADD_VISITS",				  3 },
	{"THRADD_HOME_VISITS",		   3 },
	{"HELIX_VISITS",				 3 },
	{"HELIX_UNPROTECTED",			  1 },
	{"THRADD_CULTURE",			   2 },
	{"THRADD_MISSION",			   3 },
	/* 0 if the Thraddash fleet hasn't left the Thraddash home
			 * world.
			 * 1 if the Thraddash are heading towards Kohr-Ah territory.
			 * 2 if the Thraddash are fighting the Kohr-Ah.
			 * 3 if the Thraddash are returning from Kohr-Ah territory.
			 * 4 if the Thraddash fleet is back at the Thraddash home
			 * world.
			 */

	{"DRUUGE_VISITS",				  3 },
	{"DRUUGE_HOME_VISITS",		   3 },
	{"ROSY_SPHERE",					1 },
	/* The play has or has had the Rosy Sphere.
			 * cf. ROSY_SHERE_ON_SHIP */
	{"SCANNED_MAIDENS",				1 },
	{"SCANNED_FRAGMENTS",			  1 },
	{"SCANNED_CASTER",			   1 },
	{"SCANNED_SPAWNER",				1 },
	{"SCANNED_ULTRON",			   1 },

	{"ZOQFOT_INFO",					2 },
	{"ZOQFOT_HOSTILE",			   1 },
	{"ZOQFOT_HOME_VISITS",		   3 },
	{"MET_ZOQFOT",				   1 },
	{"ZOQFOT_DISTRESS",				2 },
	/* 0 if the Zoq-Fot-Pik aren't in distress
			 * 1 if the Zoq-Fot-Pik are under attack by the Kohr-Ah
			 * 2 if the Zoq-Fot-Pik have been destroyed because of this
			 *   attack (not by the Kohr-Ah final victory cleansing)
			 */

	{"EGG_CASE1_ON_SHIP",			  1 },
	{"EGG_CASE2_ON_SHIP",			  1 },
	{"MYCON_SUN_VISITS",			 3 },
	{"ORZ_HOME_VISITS",				3 },

	{"MELNORME_FUEL_PROCEDURE",		1 },
	{"MELNORME_TECH_PROCEDURE",		1 },
	{"MELNORME_INFO_PROCEDURE",		1 },

	{"MELNORME_TECH_STACK",			4 },
	/* MELNORME_TECH_STACK is now unused */
	{"MELNORME_EVENTS_INFO_STACK",  5 },
	{"MELNORME_ALIEN_INFO_STACK",	  5 },
	{"MELNORME_HISTORY_INFO_STACK", 5 },

	{"RAINBOW_WORLD0",			   8 },
	/* Low byte of a bit array, one bit per rainbow world.
			 * Each bit is set if the rainbow world has been visited.
			 * The lowest bit is for the first star in the star_array
			 * with RAINBOW_DEFINED, and so on.
			 */
	{"RAINBOW_WORLD1",			   2 },
	/* High 2 bits of the bit array of which RAINBOW_WORLD0
			 * is the low byte.
			 */
	{"MELNORME_RAINBOW_COUNT",	   4 },
	// The number of rainbow world locations sold to the Melnorme.

	{"USED_BROADCASTER",			 1 },
	{"BROADCASTER_RESPONSE",		 1 },

	{"IMPROVED_LANDER_SPEED",		  1 },
	{"IMPROVED_LANDER_CARGO",		  1 },
	{"IMPROVED_LANDER_SHOT",		 1 },

	{"MET_ORZ_BEFORE",			   1 },
	{"YEHAT_REBEL_TOLD_PKUNK",	   1 },
	{"PLAYER_HAD_SEX",			   1 },
	{"UMGAH_BROADCASTERS_ON_SHIP",  1 },

	{"LIGHT_MINERAL_LOAD",		   3 },
	{"MEDIUM_MINERAL_LOAD",			3 },
	{"HEAVY_MINERAL_LOAD",		   3 },

	{"STARBASE_BULLETS",			 32},

	{"STARBASE_MONTH",			   4 },
	{"STARBASE_DAY",				 5 },

	{"CREW_SOLD_TO_DRUUGE0",		 8 },
	{"CREW_PURCHASED0",				8 },
	{"CREW_PURCHASED1",				8 },

	{"URQUAN_PROTECTING_SAMATRA",	  1 },

	{"THRADDASH_BODY_COUNT",		 5 },

	{"UTWIG_SUPOX_MISSION",			3 },
	/* 0 if the Utwig and Supox fleet haven't left their home
			 * world.
			 * 1 if the U&S are on their way towards the Kohr-Ah
			 * 2 if the U&S are fighting the Kohr-Ah (first 80 days)
			 * 3 does not occur
			 * 4 if the U&S are fighting the Kohr-Ah (second 80 days)
			 * 5 if the U&S are returning home.
			 * 6 if the U&S are back at their home world.
			 */
	{"SPATHI_INFO",					3 },

	{"ILWRATH_INFO",				 2 },
	{"ILWRATH_GODS_SPOKEN",			4 },
	{"ILWRATH_WORSHIP",				2 },
	{"ILWRATH_FIGHT_THRADDASH",		1 },

	{"READY_TO_CONFUSE_URQUAN",		1 },
	{"URQUAN_HYPNO_VISITS",			1 },
	{"MENTIONED_PET_COMPULSION",	 1 },
	{"URQUAN_INFO",					2 },
	{"KNOW_URQUAN_STORY",			  2 },

	{"MYCON_INFO",				   4 },
	{"MYCON_RAMBLE",				 5 },
	{"KNOW_ABOUT_SHATTERED",		 2 },
	/* 0 if the player doesn't known about shattered worlds
			 * 1 if the player has encountered a shattered world
			 * 2 if the player knows that shatterred worlds are caused
			 *   by Mycon deep children.
			 * 3 if the player has told the Syreen that Mycon Deep Children
			 *   cause shattered worlds. Proof doesn't have to be presented
			 *   yet at this time.
			 */
	{"MYCON_INSULTS",				  3 },
	{"MYCON_KNOW_AMBUSH",			  1 },
	/* Set to 1 when the Mycon have been butchered at Organon,
			 * just before the remaining Mycon head back home.
			 */

	{"SYREEN_INFO",					2 },
	{"KNOW_SYREEN_WORLD_SHATTERED", 1 },
	{"SYREEN_KNOW_ABOUT_MYCON",		1 },

	{"TALKING_PET_INFO",			 3 },
	{"TALKING_PET_SUGGESTIONS",		3 },
	{"LEARNED_TALKING_PET",			1 },
	{"DNYARRI_LIED",				 1 },
	/* Set when the Talking Pet tells you his version of their
			 * race's history with the Ur-Quan.
			 * Cleared once you confront him about this lie.
			 */
	{"SHIP_TO_COMPEL",			   1 },

	{"ORZ_GENERAL_INFO",			 2 },
	{"ORZ_PERSONAL_INFO",			  3 },
	{"ORZ_ANDRO_STATE",				2 },
	{"REFUSED_ORZ_ALLIANCE",		 1 },

	{"PKUNK_MANNER",				 2 },
	/* 0 not met the Pkunk
			 * 1 fought the Pkunk, but relations are still salvagable.
			 * 2 hostile relations with the Pkunk, no way back.
			 * 3 friendly relations with the Pkunk
			 */
	{"PKUNK_ON_THE_MOVE",			  1 },
	{"PKUNK_FLEET",					2 },
	{"PKUNK_MIGRATE",				  2 },
	{"PKUNK_RETURN",				 1 },
	{"PKUNK_WORRY",					2 },
	{"PKUNK_INFO",				   3 },
	{"PKUNK_WAR",				   2 },
	{"PKUNK_FORTUNE",				  3 },
	{"PKUNK_MIGRATE_VISITS",		 3 },
	{"PKUNK_REASONS",				  4 },
	{"PKUNK_SWITCH",				 1 },
	{"PKUNK_SENSE_VICTOR",		   1 },

	{"KOHR_AH_REASONS",				2 },
	{"KOHR_AH_PLEAD",				  2 },
	{"KOHR_AH_INFO",				 2 },
	{"KNOW_KOHR_AH_STORY",		   2 },
	{"KOHR_AH_SENSES_EVIL",			1 },
	{"URQUAN_SENSES_EVIL",		   1 },

	{"SLYLANDRO_PROBE_VISITS",	   3 },
	{"SLYLANDRO_PROBE_THREAT",	   2 },
	{"SLYLANDRO_PROBE_WRONG",		  2 },
	{"SLYLANDRO_PROBE_ID",		   2 },
	{"SLYLANDRO_PROBE_INFO",		 2 },
	{"SLYLANDRO_PROBE_EXIT",		 2 },

	{"UMGAH_HOSTILE",				  1 },
	{"UMGAH_EVIL_BLOBBIES",			1 },
	{"UMGAH_MENTIONED_TRICKS",	   2 },

	{"BOMB_CARRIER",				 1 },
	/* 0 when the flagship is not in battle, or it doesn't have the
			 *   enhanced precursor bomb installed.
			 * 1 when the flagship is in battle and the bomb is installed.
			 * This determines whether you can flee (if the warp escape
			 * unit is installed at all), and whether taking the ship into
			 * the Sa-Matra defense structure will trigger the end of the
			 * game.
			 */

	{"THRADD_MANNER",				  1 },
	{"THRADD_INTRO",				 2 },
	{"THRADD_DEMEANOR",				3 },
	{"THRADD_INFO",					2 },
	{"THRADD_BODY_LEVEL",			  2 },
	{"THRADD_MISSION_VISITS",		  1 },
	{"THRADD_STACK_1",			   3 },
	{"THRADD_HOSTILE_STACK_2",	   1 },
	{"THRADD_HOSTILE_STACK_3",	   1 },
	{"THRADD_HOSTILE_STACK_4",	   1 },
	{"THRADD_HOSTILE_STACK_5",	   1 },

	{"CHMMR_STACK",					2 },

	{"ARILOU_MANNER",				  2 },
	{"NO_PORTAL_VISITS",			 1 },
	{"ARILOU_STACK_1",			   2 },
	{"ARILOU_STACK_2",			   1 },
	{"ARILOU_STACK_3",			   2 },
	{"ARILOU_STACK_4",			   1 },
	{"ARILOU_STACK_5",			   2 },
	{"ARILOU_INFO",					2 },
	{"ARILOU_HINTS",				 2 },

	{"DRUUGE_MANNER",				  1 },
	{"DRUUGE_SPACE_INFO",			  2 },
	{"DRUUGE_HOME_INFO",			 2 },
	{"DRUUGE_SALVAGE",			   1 },
	{"KNOW_DRUUGE_SLAVERS",			2 },
	{"FRAGMENTS_BOUGHT",			 2 },

	{"ZEX_STACK_1",					2 },
	{"ZEX_STACK_2",					2 },
	{"ZEX_STACK_3",					2 },

	{"VUX_INFO",					2 },
	{"VUX_STACK_1",					4 },
	{"VUX_STACK_2",					2 },
	{"VUX_STACK_3",					2 },
	{"VUX_STACK_4",					2 },

	{"SHOFIXTI_STACK4",				2 },

	{"YEHAT_REBEL_INFO",			 3 },
	{"YEHAT_ROYALIST_INFO",			1 },
	{"YEHAT_ROYALIST_TOLD_PKUNK",	  1 },
	{"NO_YEHAT_ALLY_HOME",		   1 },
	{"NO_YEHAT_HELP_HOME",		   1 },
	{"NO_YEHAT_INFO",				  1 },
	{"NO_YEHAT_ALLY_SPACE",			2 },
	{"NO_YEHAT_HELP_SPACE",			2 },

	{"ZOQFOT_KNOW_MASK",			 4 },

	{"SUPOX_HOSTILE",				  1 },
	{"SUPOX_INFO",				   1 },
	{"SUPOX_WAR_NEWS",			   2 },
	{"SUPOX_ULTRON_HELP",			  1 },
	{"SUPOX_STACK1",				 3 },
	{"SUPOX_STACK2",				 2 },

	{"UTWIG_HOSTILE",				  1 },
	{"UTWIG_INFO",				   1 },
	{"UTWIG_WAR_NEWS",			   2 },
	{"UTWIG_STACK1",				 3 },
	{"UTWIG_STACK2",				 2 },
	{"BOMB_INFO",				   1 },
	{"BOMB_STACK1",					2 },
	{"BOMB_STACK2",					2 },

	{"SLYLANDRO_KNOW_BROKEN",		  1 },
	{"PLAYER_KNOWS_PROBE",		   1 },
	{"PLAYER_KNOWS_PROGRAM",		 1 },
	{"PLAYER_KNOWS_EFFECTS",		 1 },
	{"PLAYER_KNOWS_PRIORITY",		  1 },
	{"SLYLANDRO_STACK1",			 3 },
	{"SLYLANDRO_STACK2",			 1 },
	{"SLYLANDRO_STACK3",			 2 },
	{"SLYLANDRO_STACK4",			 2 },
	{"SLYLANDRO_STACK5",			 1 },
	{"SLYLANDRO_STACK6",			 1 },
	{"SLYLANDRO_STACK7",			 2 },
	{"SLYLANDRO_STACK8",			 2 },
	{"SLYLANDRO_STACK9",			 2 },
	{"SLYLANDRO_KNOW_EARTH",		 1 },
	{"SLYLANDRO_KNOW_EXPLORE",	   1 },
	{"SLYLANDRO_KNOW_GATHER",		  1 },
	{"SLYLANDRO_KNOW_URQUAN",		  2 },
	{"RECALL_VISITS",				  2 },

	{"SLYLANDRO_MULTIPLIER",		 3 },
	{"KNOW_SPATHI_QUEST",			  1 },
	{"KNOW_SPATHI_EVIL",			 1 },

	{"BATTLE_PLANET",				  8 },
	{"ESCAPE_COUNTER",			   8 },

	{"CREW_SOLD_TO_DRUUGE1",		 8 },
	{"PKUNK_DONE_WAR",			   1 },

	{"SYREEN_STACK0",				  2 },
	{"SYREEN_STACK1",				  2 },
	{"SYREEN_STACK2",				  2 },

	{"REFUSED_ULTRON_AT_BOMB",	   1 },
	{"NO_TRICK_AT_SUN",				1 },

	{"SPATHI_STACK0",				  2 },
	{"SPATHI_STACK1",				  1 },
	{"SPATHI_STACK2",				  1 },

	{"ORZ_STACK0",				   1 },
	{"ORZ_STACK1",				   1 },

	{"SHOFIXTI_GRPOFFS",			 32},
	{"ZOQFOT_GRPOFFS",			   32},
	{"MELNORME0_GRPOFFS",			  32},
	{"MELNORME1_GRPOFFS",			  32},
	{"MELNORME2_GRPOFFS",			  32},
	{"MELNORME3_GRPOFFS",			  32},
	{"MELNORME4_GRPOFFS",			  32},
	{"MELNORME5_GRPOFFS",			  32},
	{"MELNORME6_GRPOFFS",			  32},
	{"MELNORME7_GRPOFFS",			  32},
	{"MELNORME8_GRPOFFS",			  32},
	{"URQUAN_PROBE_GRPOFFS",		 32},
	{"COLONY_GRPOFFS",			   32},
	{"SAMATRA_GRPOFFS",				32},

	/* end rev 0, Core UQM v0.8.0 */
	{nullptr,					   1 },
	/* begin rev 1, MegaMod v0.8.0.85 */

	{"AUTOPILOT_OK",				 1 },

	{"KNOW_QS_PORTAL",			   16},

	/* end rev 1, MegaMod v0.8.0.85 */
	{nullptr,					   2 },
	/* begin rev 2, MegaMod v0.8.1 */

	{"SYS_VISITED_00",			   32},
	{"SYS_VISITED_01",			   32},
	{"SYS_VISITED_02",			   32},
	{"SYS_VISITED_03",			   32},
	{"SYS_VISITED_04",			   32},
	{"SYS_VISITED_05",			   32},
	{"SYS_VISITED_06",			   32},
	{"SYS_VISITED_07",			   32},
	{"SYS_VISITED_08",			   32},
	{"SYS_VISITED_09",			   32},
	{"SYS_VISITED_10",			   32},
	{"SYS_VISITED_11",			   32},
	{"SYS_VISITED_12",			   32},
	{"SYS_VISITED_13",			   32},
	{"SYS_VISITED_14",			   32},
	{"SYS_VISITED_15",			   32},

	{"KNOW_HOMEWORLD",			   18},

	{"HM_ENCOUNTERS",				  9 },

	{"RESERVED",					32},

	/* end rev 2, MegaMod v0.8.1 */
	{nullptr,					   3 },
	/* begin rev 3, MegaMod v0.8.2 */

	{"SYS_PLYR_MARKER_00",		   32},
	{"SYS_PLYR_MARKER_01",		   32},
	{"SYS_PLYR_MARKER_02",		   32},
	{"SYS_PLYR_MARKER_03",		   32},
	{"SYS_PLYR_MARKER_04",		   32},
	{"SYS_PLYR_MARKER_05",		   32},
	{"SYS_PLYR_MARKER_06",		   32},
	{"SYS_PLYR_MARKER_07",		   32},
	{"SYS_PLYR_MARKER_08",		   32},
	{"SYS_PLYR_MARKER_09",		   32},
	{"SYS_PLYR_MARKER_10",		   32},
	{"SYS_PLYR_MARKER_11",		   32},
	{"SYS_PLYR_MARKER_12",		   32},
	{"SYS_PLYR_MARKER_13",		   32},
	{"SYS_PLYR_MARKER_14",		   32},
	{"SYS_PLYR_MARKER_15",		   32},

	{"LAST_LOCATION_X",				16},
	{"LAST_LOCATION_Y",				16},

	/* end rev 3, MegaMod v0.8.2 */
	{nullptr,					   4 },
	/* begin rev 4, MegaMod v0.8.3 */

	{"ADV_AUTOPILOT_SAVE_X",		 16},
	{"ADV_AUTOPILOT_SAVE_Y",		 16},

	{"ADV_AUTOPILOT_QUASI_X",		  16},
	{"ADV_AUTOPILOT_QUASI_Y",		  16},

	/* end rev 4, MegaMod v0.8.3 */
	{nullptr,					   5 },
	/* begin rev 5, MegaMod v0.8.4 */

	{"SEED_TYPE",				   2 },
	{"SUPOX_SHIP_MONTH",			 4 },
	{"SUPOX_SHIP_DAY",			   5 },
	{"SUPOX_SHIP_YEAR",				5 },
	{"UTWIG_SHIP_MONTH",			 4 },
	{"UTWIG_SHIP_DAY",			   5 },
	{"UTWIG_SHIP_YEAR",				5 },
	{"REV_5_PAD",				   1 },

	/* end rev 5, MegaMod v0.8.4 */
	{nullptr,					   0 },
};

namespace
{

template <typename T>
void addInt(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, T value)
{
	if constexpr (uqstl::is_enum_v<T>)
	{
		addInt(doc, obj, key, static_cast<uqstl::underlying_type_t<T>>(value));
	}
	else if constexpr (uqstl::is_signed_v<T>)
	{
		yyjson_mut_obj_add_sint(doc, obj, key, value);
	}
	else
	{
		yyjson_mut_obj_add_uint(doc, obj, key, value);
	}
}

template <typename T>
void addArray(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, uqstl::span<const T> values)
{
	yyjson_mut_val* array {yyjson_mut_obj_add_arr(doc, obj, key)};
	for (const T value : values)
	{
		if constexpr (uqstl::is_signed_v<T>)
		{
			yyjson_mut_arr_add_sint(doc, array, value);
		}
		else
		{
			yyjson_mut_arr_add_uint(doc, array, value);
		}
	}
}

template <typename Point>
void addPoint(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, Point x, Point y)
{
	yyjson_mut_val* point {yyjson_mut_obj_add_obj(doc, obj, key)};
	addInt(doc, point, "x", x);
	addInt(doc, point, "y", y);
}

void addString(yyjson_mut_doc* doc, yyjson_mut_val* obj, const char* key, uqstl::span<const char> buffer)
{
	yyjson_mut_obj_add_strncpy(doc, obj, key, buffer.data(), strnlen(buffer.data(), buffer.size()));
}

void addShipFragment(yyjson_mut_doc* doc, yyjson_mut_val* obj, const SHIP_FRAGMENT& frag)
{
	addInt(doc, obj, "captains_name_index", frag.captains_name_index);
	addInt(doc, obj, "race_id", frag.race_id);
	addInt(doc, obj, "index", frag.index);
	addInt(doc, obj, "crew_level", frag.crew_level);
	addInt(doc, obj, "max_crew", frag.max_crew);
	addInt(doc, obj, "energy_level", frag.energy_level);
	addInt(doc, obj, "max_energy", frag.max_energy);
}

void addIpGroup(yyjson_mut_doc* doc, yyjson_mut_val* obj, const IP_GROUP& group)
{
	addInt(doc, obj, "group_counter", group.group_counter);
	addInt(doc, obj, "race_id", group.race_id);
	addInt(doc, obj, "sys_loc", group.sys_loc);
	addInt(doc, obj, "task", group.task);
	addInt(doc, obj, "in_system", group.in_system);
	addInt(doc, obj, "dest_loc", group.dest_loc);
	addInt(doc, obj, "orbit_pos", group.orbit_pos);
	addInt(doc, obj, "group_id", group.group_id);
	addPoint(doc, obj, "loc", group.loc.x, group.loc.y);
}

void SaveShipQueue(yyjson_mut_doc* doc, yyjson_mut_val* root, const char* key, QUEUE* pQueue)
{
	uint16_t num_links {CountLinks(pQueue)};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, key)};
	HSHIPFRAG hStarShip {GetHeadLink(pQueue)};
	while (num_links--)
	{
		SHIP_FRAGMENT* FragPtr {LockShipFrag(pQueue, hStarShip)};
		const HSHIPFRAG hNextShip {_GetSuccLink(FragPtr)};
		addShipFragment(doc, yyjson_mut_arr_add_obj(doc, list), *FragPtr);
		UnlockShipFrag(pQueue, hStarShip);
		hStarShip = hNextShip;
	}
}

void SaveRaceQueue(yyjson_mut_doc* doc, yyjson_mut_val* root, QUEUE* pQueue)
{
	uint16_t num_links {CountLinks(pQueue)};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "raceQueue")};
	HFLEETINFO hFleet {GetHeadLink(pQueue)};
	while (num_links--)
	{
		FLEET_INFO* FleetPtr {LockFleetInfo(pQueue, hFleet)};
		const HFLEETINFO hNextFleet {_GetSuccLink(FleetPtr)};

		yyjson_mut_val* fleet {yyjson_mut_arr_add_obj(doc, list)};
		addInt(doc, fleet, "Index", GetIndexFromStarShip(pQueue, hFleet));
		addInt(doc, fleet, "allied_state", FleetPtr->allied_state);
		addInt(doc, fleet, "days_left", FleetPtr->days_left);
		addInt(doc, fleet, "growth_fract", FleetPtr->growth_fract);
		addInt(doc, fleet, "crew_level", FleetPtr->crew_level);
		addInt(doc, fleet, "max_crew", FleetPtr->max_crew);
		addInt(doc, fleet, "growth", FleetPtr->growth);
		addInt(doc, fleet, "max_energy", FleetPtr->max_energy);
		addPoint(doc, fleet, "loc", FleetPtr->loc.x, FleetPtr->loc.y);
		addInt(doc, fleet, "actual_strength", FleetPtr->actual_strength);
		addInt(doc, fleet, "known_strength", FleetPtr->known_strength);
		addPoint(doc, fleet, "known_loc", FleetPtr->known_loc.x, FleetPtr->known_loc.y);
		addInt(doc, fleet, "growth_err_term", FleetPtr->growth_err_term);
		addInt(doc, fleet, "func_index", FleetPtr->func_index);
		addPoint(doc, fleet, "dest_loc", FleetPtr->dest_loc.x, FleetPtr->dest_loc.y);

		UnlockFleetInfo(pQueue, hFleet);
		hFleet = hNextFleet;
	}
}

void SaveGroupQueue(yyjson_mut_doc* doc, yyjson_mut_val* root, QUEUE* pQueue)
{
	if (CountLinks(pQueue) == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "ipGroupQueue")};
	HIPGROUP hNextGroup {};
	for (HIPGROUP hGroup {GetHeadLink(pQueue)}; hGroup; hGroup = hNextGroup)
	{
		IP_GROUP* GroupPtr {LockIpGroup(pQueue, hGroup)};
		hNextGroup = _GetSuccLink(GroupPtr);
		addIpGroup(doc, yyjson_mut_arr_add_obj(doc, list), *GroupPtr);
		UnlockIpGroup(pQueue, hGroup);
	}
}

void SaveEncounters(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	uint16_t num_links {CountLinks(&GLOBAL(encounter_q))};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "encounters")};
	HENCOUNTER hEncounter {GetHeadLink(&GLOBAL(encounter_q))};
	while (num_links--)
	{
		ENCOUNTER* EncounterPtr {};
		LockEncounter(hEncounter, &EncounterPtr);
		const HENCOUNTER hNextEncounter {GetSuccEncounter(EncounterPtr)};

		yyjson_mut_val* encounter {yyjson_mut_arr_add_obj(doc, list)};
		addInt(doc, encounter, "transition_state", EncounterPtr->transition_state);
		addPoint(doc, encounter, "origin", EncounterPtr->origin.x, EncounterPtr->origin.y);
		addInt(doc, encounter, "radius", EncounterPtr->radius);
		addPoint(doc, encounter, "loc_pt", EncounterPtr->loc_pt.x, EncounterPtr->loc_pt.y);
		addInt(doc, encounter, "race_id", EncounterPtr->race_id);
		addInt(doc, encounter, "flags", EncounterPtr->flags);

		yyjson_mut_val* ships {yyjson_mut_obj_add_arr(doc, encounter, "ships")};
		const uint16_t numShips {uqstl::min<uint16_t>(EncounterPtr->num_ships, MAX_HYPER_SHIPS)};
		for (uint16_t i {}; i < numShips; ++i)
		{
			const BRIEF_SHIP_INFO& ShipInfo {EncounterPtr->ShipList[i]};
			yyjson_mut_val* ship {yyjson_mut_arr_add_obj(doc, ships)};
			addInt(doc, ship, "race_id", ShipInfo.race_id);
			addInt(doc, ship, "crew_level", ShipInfo.crew_level);
			addInt(doc, ship, "max_crew", ShipInfo.max_crew);
			addInt(doc, ship, "max_energy", ShipInfo.max_energy);
		}

		addPoint(doc, encounter, "log", RES_DESCALE(EncounterPtr->log_x), RES_DESCALE(EncounterPtr->log_y));

		UnlockEncounter(hEncounter);
		hEncounter = hNextEncounter;
	}
}

void SaveEvents(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	uint16_t num_links {CountLinks(&GLOBAL(GameClock.event_q))};
	if (num_links == 0)
	{
		return;
	}

	yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "events")};
	HEVENT hEvent {GetHeadLink(&GLOBAL(GameClock.event_q))};
	while (num_links--)
	{
		EVENT* EventPtr {};
		LockEvent(hEvent, &EventPtr);
		const HEVENT hNextEvent {GetSuccEvent(EventPtr)};

		yyjson_mut_val* event {yyjson_mut_arr_add_obj(doc, list)};
		addInt(doc, event, "day_index", EventPtr->day_index);
		addInt(doc, event, "month_index", EventPtr->month_index);
		addInt(doc, event, "year_index", EventPtr->year_index);
		addInt(doc, event, "func_index", EventPtr->func_index);

		UnlockEvent(hEvent);
		hEvent = hNextEvent;
	}
}

void SaveClockState(yyjson_mut_doc* doc, yyjson_mut_val* obj, const CLOCK_STATE& clock)
{
	yyjson_mut_val* clockObj {yyjson_mut_obj_add_obj(doc, obj, "GameClock")};
	addInt(doc, clockObj, "day_index", clock.day_index);
	addInt(doc, clockObj, "month_index", clock.month_index);
	addInt(doc, clockObj, "year_index", clock.year_index);
	addInt(doc, clockObj, "tick_count", clock.tick_count);
	addInt(doc, clockObj, "day_in_ticks", clock.day_in_ticks);
}

void SaveGameStateFlags(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	yyjson_mut_val* flags {yyjson_mut_obj_add_obj(doc, root, "gameState")};
	for (const GameStateBitMap* bm {gameStateBitMap}; bm->name || bm->numBits; ++bm)
	{
		if (!bm->name)
		{
			continue;
		}

		const uint32_t value {getGameStateUint(bm->name)};
		const uint32_t maxValue {bm->numBits >= 32 ? UINT32_MAX : (uint32_t {1} << bm->numBits) - 1};
		if (value > maxValue)
		{
			uqm::log::error("Game state '{}' ({}) does not fit in {} bits.", bm->name, value, bm->numBits);
		}
		yyjson_mut_obj_add_uint(doc, flags, bm->name, value);
	}
}

void SaveGameState(yyjson_mut_doc* doc, yyjson_mut_val* root, const GAME_STATE& GS)
{
	const uint8_t res_scale {static_cast<uint8_t>(lowByte(GS.CurrentActivity) != IN_INTERPLANETARY ? RESOLUTION_FACTOR : 0)};

	yyjson_mut_val* state {yyjson_mut_obj_add_obj(doc, root, "globalState")};
	addInt(doc, state, "glob_flags", GS.glob_flags);
	addInt(doc, state, "CrewCost", GS.CrewCost);
	addInt(doc, state, "FuelCost", GS.FuelCost);
	addArray<uint8_t>(doc, state, "ModuleCost", GS.ModuleCost);
	addArray<uint8_t>(doc, state, "ElementWorth", GS.ElementWorth);
	addInt(doc, state, "CurrentActivity", GS.CurrentActivity);
	SaveClockState(doc, state, GS.GameClock);
	addPoint(doc, state, "autopilot", GS.autopilot.x, GS.autopilot.y);
	addPoint(doc, state, "ip_location", GS.ip_location.x, GS.ip_location.y);
	addPoint(doc, state, "ShipOrigin", RES_DESCALE(GS.ShipStamp.origin.x), RES_DESCALE(GS.ShipStamp.origin.y));
	addInt(doc, state, "ShipFacing", GS.ShipFacing);
	addInt(doc, state, "ip_planet", GS.ip_planet);
	addInt(doc, state, "in_orbit", GS.in_orbit);

	const VELOCITY_DESC& velocity {GS.velocity};
	yyjson_mut_val* velocityObj {yyjson_mut_obj_add_obj(doc, state, "velocity")};
	addInt(doc, velocityObj, "TravelAngle", velocity.TravelAngle);
	addPoint(doc, velocityObj, "vector", velocity.vector.width >> res_scale, velocity.vector.height >> res_scale);
	addPoint(doc, velocityObj, "fract", velocity.fract.width >> res_scale, velocity.fract.height >> res_scale);
	addPoint(doc, velocityObj, "error", velocity.error.width >> res_scale, velocity.error.height >> res_scale);
	addPoint(doc, velocityObj, "incr", velocity.incr.width >> res_scale, velocity.incr.height >> res_scale);

	SaveGameStateFlags(doc, root);
}

void SaveSisState(yyjson_mut_doc* doc, yyjson_mut_val* obj, const SIS_STATE& SS)
{
	yyjson_mut_val* sis {yyjson_mut_obj_add_obj(doc, obj, "sis")};
	addPoint(doc, sis, "log", RES_DESCALE(SS.log_x), RES_DESCALE(SS.log_y));
	addInt(doc, sis, "ResUnits", SS.ResUnits);
	addInt(doc, sis, "FuelOnBoard", SS.FuelOnBoard);
	addInt(doc, sis, "CrewEnlisted", SS.CrewEnlisted);
	addInt(doc, sis, "TotalElementMass", SS.TotalElementMass);
	addInt(doc, sis, "TotalBioMass", SS.TotalBioMass);
	addArray<uint8_t>(doc, sis, "ModuleSlots", SS.ModuleSlots);
	addArray<uint8_t>(doc, sis, "DriveSlots", SS.DriveSlots);
	addArray<uint8_t>(doc, sis, "JetSlots", SS.JetSlots);
	addInt(doc, sis, "NumLanders", SS.NumLanders);
	addArray<uint16_t>(doc, sis, "ElementAmounts", SS.ElementAmounts);
	addString(doc, sis, "ShipName", SS.ShipName);
	addString(doc, sis, "CommanderName", SS.CommanderName);
	addString(doc, sis, "PlanetName", SS.PlanetName);
	addInt(doc, sis, "Difficulty", SS.Difficulty);
	addInt(doc, sis, "Extended", SS.Extended);
	addInt(doc, sis, "Nomad", SS.Nomad);
	addInt(doc, sis, "Seed", SS.Seed);
	addInt(doc, sis, "ShipSeed", SS.ShipSeed);
}

void SaveSummary(yyjson_mut_doc* doc, yyjson_mut_val* root, const SUMMARY_DESC& Summ)
{
	yyjson_mut_val* summary {yyjson_mut_obj_add_obj(doc, root, "summary")};
	addString(doc, summary, "SaveName", Summ.SaveName);
	addInt(doc, summary, "Activity", Summ.Activity);
	addInt(doc, summary, "Flags", Summ.Flags);
	addInt(doc, summary, "day_index", Summ.day_index);
	addInt(doc, summary, "month_index", Summ.month_index);
	addInt(doc, summary, "year_index", Summ.year_index);
	addInt(doc, summary, "MCreditLo", Summ.MCreditLo);
	addInt(doc, summary, "MCreditHi", Summ.MCreditHi);
	addArray<uint8_t>(doc, summary, "ShipList", uqstl::span {Summ.ShipList}.first(Summ.NumShips));
	addArray<uint8_t>(doc, summary, "DeviceList", uqstl::span {Summ.DeviceList}.first(Summ.NumDevices));
	addInt(doc, summary, "res_factor", Summ.res_factor);
	SaveSisState(doc, summary, Summ.SS);
}

void SaveStarDesc(yyjson_mut_doc* doc, yyjson_mut_val* root, const STAR_DESC& SD)
{
	yyjson_mut_val* star {yyjson_mut_obj_add_obj(doc, root, "starDesc")};
	addPoint(doc, star, "star_pt", SD.star_pt.x, SD.star_pt.y);
	addInt(doc, star, "Type", SD.Type);
	addInt(doc, star, "Index", SD.Index);
	addInt(doc, star, "Prefix", SD.Prefix);
	addInt(doc, star, "Postfix", SD.Postfix);
}

} // namespace

static void
PrepareSummary(SUMMARY_DESC* SummPtr, const char* name)
{
	SummPtr->SS = GlobData.SIS_state;

	SummPtr->Activity = lowByte(GLOBAL(CurrentActivity));
	switch (SummPtr->Activity)
	{
		case IN_HYPERSPACE:
			if (inQuasiSpace())
			{
				SummPtr->Activity = IN_QUASISPACE;
			}
			break;
		case IN_INTERPLANETARY:
			// Get a better planet name for summary
			GetPlanetOrMoonName(SummPtr->SS.PlanetName,
								sizeof(SummPtr->SS.PlanetName));
			if (GET_GAME_STATE(GLOBAL_FLAGS_AND_DATA) == (uint8_t)~0)
			{
				SummPtr->Activity = IN_STARBASE;
			}
			else if (playerInPlanetOrbit())
			{
				SummPtr->Activity = IN_PLANET_ORBIT;
			}
			break;
		case IN_LAST_BATTLE:
			uqm::utf8StringCopy(SummPtr->SS.PlanetName,
								sizeof(SummPtr->SS.PlanetName),
								GAME_STRING(PLANET_NUMBER_BASE + 32)); // Sa-Matra
			break;
	}

	SummPtr->MCreditLo = GET_GAME_STATE(MELNORME_CREDIT0);
	SummPtr->MCreditHi = GET_GAME_STATE(MELNORME_CREDIT1);

	{
		HSHIPFRAG hStarShip, hNextShip;

		for (hStarShip = GetHeadLink(&GLOBAL(built_ship_q)),
			SummPtr->NumShips = 0;
			 hStarShip; hStarShip = hNextShip,
			++SummPtr->NumShips)
		{
			SHIP_FRAGMENT* StarShipPtr;

			StarShipPtr = LockShipFrag(&GLOBAL(built_ship_q), hStarShip);
			hNextShip = _GetSuccLink(StarShipPtr);
			SummPtr->ShipList[SummPtr->NumShips] = StarShipPtr->race_id;
			UnlockShipFrag(&GLOBAL(built_ship_q), hStarShip);
		}
	}

	SummPtr->NumDevices = InventoryDevices(SummPtr->DeviceList,
										   MAX_EXCLUSIVE_DEVICES);

	SummPtr->Flags = GET_GAME_STATE(LANDER_SHIELDS)
				   | (GET_GAME_STATE(IMPROVED_LANDER_SPEED) << (4 + 0))
				   | (GET_GAME_STATE(IMPROVED_LANDER_CARGO) << (4 + 1))
				   | (GET_GAME_STATE(IMPROVED_LANDER_SHOT) << (4 + 2))
				   | ((GET_GAME_STATE(CHMMR_BOMB_STATE) < 2 ? 0 : 1) << (4 + 3));

	SummPtr->day_index = GLOBAL(GameClock.day_index);
	SummPtr->month_index = GLOBAL(GameClock.month_index);
	SummPtr->year_index = GLOBAL(GameClock.year_index);
	SummPtr->SaveName[SAVE_NAME_SIZE - 1] = 0;
	uqm::strncpy_safe(SummPtr->SaveName, name);
	SummPtr->res_factor = RESOLUTION_FACTOR;
}

static void
SaveProblemMessage(STAMP* MsgStamp)
{
#define MAX_MSG_LINES 1
	GFXRECT r = {
		{0, 0},
		{0, 0}
	  };
	uint16_t i;
	TEXT t;
	char* ppStr[MAX_MSG_LINES];

	// TODO: This should probably just use DoPopupWindow()

	ppStr[0] = GAME_STRING(SAVEGAME_STRING_BASE + 2);

	SetContextFont(StarConFont);

	t.baseline.x = t.baseline.y = 0;
	t.align = ALIGN_CENTER;
	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		GFXRECT tr;

		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
		{
			break;
		}
		t.CharCount = (uint16_t)~0;
		TextRect(&t, &tr, nullptr);
		if (i == 0)
		{
			r = tr;
		}
		else
		{
			BoxUnion(&tr, &r, &r);
		}
		t.baseline.y += 11;
	}
	t.baseline.x = (RES_SCALE(ORIG_SIS_SCREEN_WIDTH >> 1)
					- (r.extent.width >> 1))
				 - r.corner.x;
	t.baseline.y = (RES_SCALE(ORIG_SIS_SCREEN_HEIGHT >> 1)
					- (r.extent.height >> 1))
				 - r.corner.y;
	r.corner.x += t.baseline.x - RES_SCALE(4);
	r.corner.y += t.baseline.y - RES_SCALE(4);
	r.extent.width += RES_SCALE(8);
	r.extent.height += RES_SCALE(8);

	*MsgStamp = SaveContextFrame(&r);

	BatchGraphics();
	DrawStarConBox(&r, RES_SCALE(2), SHADOWBOX_MEDIUM_COLOR,
				   SHADOWBOX_DARK_COLOR, true, DKGRAY_COLOR, true, TRANSPARENT);
	SetContextForeGroundColor(
		isPC(uqm::UQMOptions::read().whichFonts) ? WHITE_COLOR : LTGRAY_COLOR);

	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
		{
			break;
		}
		t.CharCount = (uint16_t)~0;
		font_DrawText(&t);
		t.baseline.y += 11;
	}
	UnbatchGraphics();
}

void SaveProblem(void)
{
	STAMP s;
	GFXCONTEXT OldContext;

	OldContext = SetContext(SpaceContext);
	SaveProblemMessage(&s);
	FlushGraphics();

	WaitForAnyButton(true, WAIT_INFINITE, false);

	// Restore the screen under the message
	DrawStamp(&s);
	SetContext(OldContext);
	DestroyDrawable(ReleaseDrawable(s.frame));
}

static void
SaveFlagshipState(void)
{
	if (inHQSpace())
	{
		// Player is in HyperSpace or QuasiSpace.
		SaveSisHyperState();
	}
	else if (playerInSolarSystem())
	{
		SaveSolarSysLocation();
	}
}

static void
SaveStarInfo(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	GAME_STATE_FILE* fp {OpenStateFile(STARINFO_FILE, "rb")};
	if (fp)
	{
		uint32_t flen {LengthStateFile(fp)};
		if (flen % 4)
		{
			uqm::log::warn("Unexpected Star Info length! Expected "
						   "an integral number of DWORDS.\n");
		}
		else
		{
			yyjson_mut_val* list {yyjson_mut_obj_add_arr(doc, root, "starInfo")};
			while (flen)
			{
				uint32_t val {};
				sread_32(fp, &val);
				yyjson_mut_arr_add_uint(doc, list, val);
				flen -= 4;
			}
		}
		CloseStateFile(fp);
	}
}

static void
SaveBattleGroup(yyjson_mut_doc* doc, yyjson_mut_val* list, GAME_STATE_FILE* fp,
				uint32_t encounter_id, uint32_t grpoffs)
{
	GROUP_HEADER h;
	SeekStateFile(fp, grpoffs, SEEK_SET);
	ReadGroupHeader(fp, &h);

	yyjson_mut_val* battleGroup {yyjson_mut_arr_add_obj(doc, list)};
	addInt(doc, battleGroup, "encounter", encounter_id);
	yyjson_mut_obj_add_bool(doc, battleGroup, "current", grpoffs && GLOBAL(BattleGroupRef) == grpoffs);
	addInt(doc, battleGroup, "star_index", h.star_index);
	addInt(doc, battleGroup, "day_index", h.day_index);
	addInt(doc, battleGroup, "month_index", h.month_index);
	addInt(doc, battleGroup, "year_index", h.year_index);

	yyjson_mut_val* groups {yyjson_mut_obj_add_arr(doc, battleGroup, "groups")};
	for (int i {1}; i <= h.NumGroups; ++i)
	{
		uint8_t icon {};
		uint8_t NumShips {};
		SeekStateFile(fp, h.GroupOffset[i], SEEK_SET);
		sread_8(fp, &icon);
		sread_8(fp, &NumShips);

		yyjson_mut_val* group {yyjson_mut_arr_add_obj(doc, groups)};
		addInt(doc, group, "icon", icon);
		yyjson_mut_val* ships {yyjson_mut_obj_add_arr(doc, group, "ships")};
		for (int j {}; j < NumShips; ++j)
		{
			uint8_t race_outer {};
			SHIP_FRAGMENT sf;
			sread_8(fp, &race_outer);
			ReadShipFragment(fp, &sf);

			yyjson_mut_val* ship {yyjson_mut_arr_add_obj(doc, ships)};
			addInt(doc, ship, "race_outer", race_outer);
			addShipFragment(doc, ship, sf);
		}
	}
}

static uint32_t
GetBattleGroupOffset(int encounterIndex)
{
	// The reason for this switch, even though the group offsets are
	// successive, is because GET_GAME_STATE is a #define, which stringizes
	// its argument.
	switch (encounterIndex)
	{
		case 1:
			return GET_GAME_STATE(SHOFIXTI_GRPOFFS);
		case 2:
			return GET_GAME_STATE(ZOQFOT_GRPOFFS);
		case 3:
			return GET_GAME_STATE(MELNORME0_GRPOFFS);
		case 4:
			return GET_GAME_STATE(MELNORME1_GRPOFFS);
		case 5:
			return GET_GAME_STATE(MELNORME2_GRPOFFS);
		case 6:
			return GET_GAME_STATE(MELNORME3_GRPOFFS);
		case 7:
			return GET_GAME_STATE(MELNORME4_GRPOFFS);
		case 8:
			return GET_GAME_STATE(MELNORME5_GRPOFFS);
		case 9:
			return GET_GAME_STATE(MELNORME6_GRPOFFS);
		case 10:
			return GET_GAME_STATE(MELNORME7_GRPOFFS);
		case 11:
			return GET_GAME_STATE(MELNORME8_GRPOFFS);
		case 12:
			return GET_GAME_STATE(URQUAN_PROBE_GRPOFFS);
		case 13:
			return GET_GAME_STATE(COLONY_GRPOFFS);
		case 14:
			return GET_GAME_STATE(SAMATRA_GRPOFFS);
		default:
			uqm::log::warn("SetBattleGroupOffset: invalid encounter"
						   " index.\n");
			return 0;
	}
}

static void
SaveGroups(yyjson_mut_doc* doc, yyjson_mut_val* root)
{
	yyjson_mut_val* battleGroups {};
	GAME_STATE_FILE* fp {OpenStateFile(RANDGRPINFO_FILE, "rb")};
	if (fp && LengthStateFile(fp) > 0)
	{
		GROUP_HEADER h;
		uint8_t lastenc {};
		uint8_t count {};
		ReadGroupHeader(fp, &h);
		SeekStateFile(fp, h.GroupOffset[0], SEEK_SET);
		sread_8(fp, &lastenc);
		sread_8(fp, &count);

		yyjson_mut_val* groupList {yyjson_mut_obj_add_obj(doc, root, "groupList")};
		addInt(doc, groupList, "LastEnc", lastenc);
		yyjson_mut_val* groups {yyjson_mut_obj_add_arr(doc, groupList, "groups")};
		for (int i {}; i < count; ++i)
		{
			uint8_t race_outer {};
			IP_GROUP ip;
			sread_8(fp, &race_outer);
			ReadIpGroup(fp, &ip);

			yyjson_mut_val* group {yyjson_mut_arr_add_obj(doc, groups)};
			addInt(doc, group, "race_outer", race_outer);
			addIpGroup(doc, group, ip);
		}

		battleGroups = yyjson_mut_obj_add_arr(doc, root, "battleGroups");
		SaveBattleGroup(doc, battleGroups, fp, 0, 0);
		CloseStateFile(fp);
	}
	fp = OpenStateFile(DEFGRPINFO_FILE, "rb");
	if (fp && LengthStateFile(fp) > 0)
	{
		for (int encounter_index {1}; encounter_index < 15; encounter_index++)
		{
			const uint32_t grpoffs {GetBattleGroupOffset(encounter_index)};
			if (grpoffs)
			{
				if (!battleGroups)
				{
					battleGroups = yyjson_mut_obj_add_arr(doc, root, "battleGroups");
				}
				SaveBattleGroup(doc, battleGroups, fp, encounter_index, grpoffs);
			}
		}
		CloseStateFile(fp);
	}
}

uqstl::string saveFileName(uint16_t which_game)
{
	return fmt::format("uqmsave.{:02}.json", which_game);
}

bool SaveGame(uint16_t which_game, SUMMARY_DESC* SummPtr, const char* name)
{
	STAR_DESC SD {};
	if (CurStarDescPtr)
	{
		SD = *CurStarDescPtr;
	}

	// XXX: Backup: SaveFlagshipState() overwrites ip_location
	const GFXPOINT pt {GLOBAL(ip_location)};
	SaveFlagshipState();
	if (lowByte(GLOBAL(CurrentActivity)) == IN_INTERPLANETARY
		&& !(GLOBAL(CurrentActivity)
			 & (START_ENCOUNTER | START_INTERPLANETARY)))
	{
		PutGroupInfo(GROUPS_RANDOM, GROUP_SAVE_IP);
	}

	const uqstl::unique_ptr<yyjson_mut_doc, decltype(&yyjson_mut_doc_free)> doc {yyjson_mut_doc_new(nullptr), &yyjson_mut_doc_free};
	yyjson_mut_val* root {yyjson_mut_obj(doc.get())};
	yyjson_mut_doc_set_root(doc.get(), root);
	yyjson_mut_obj_add_uint(doc.get(), root, "version", c_saveFormatVersion);

	PrepareSummary(SummPtr, name);
	SaveSummary(doc.get(), root, *SummPtr);
	SaveGameState(doc.get(), root, GlobData.Game_state);

	// XXX: Restore
	GLOBAL(ip_location) = pt;
	// Only relevant when loading a game and must be cleaned
	GLOBAL(in_orbit) = 0;

	SaveRaceQueue(doc.get(), root, &GLOBAL(avail_race_q));
	// START_INTERPLANETARY is only set when saving from Homeworld
	//   encounter screen. When the game is loaded, the
	//   GenerateOrbitalFunction for the current star system
	//   create the encounter anew and populate the npc queue.
	if (!(GLOBAL(CurrentActivity) & START_INTERPLANETARY))
	{
		if (GLOBAL(CurrentActivity) & START_ENCOUNTER)
		{
			SaveShipQueue(doc.get(), root, "npcShipQueue", &GLOBAL(npc_built_ship_q));
		}
		else if (lowByte(GLOBAL(CurrentActivity)) == IN_INTERPLANETARY)
		{
			// XXX: Technically, this queue does not need to be
			//   saved/loaded at all. IP groups will be reloaded
			//   from group state files. But the original code did,
			//   and so will we until we can prove we do not need to.
			SaveGroupQueue(doc.get(), root, &GLOBAL(ip_group_q));
		}
	}
	SaveShipQueue(doc.get(), root, "shipQueue", &GLOBAL(built_ship_q));
	SaveShipQueue(doc.get(), root, "stowedShipQueue", &GLOBAL(stowed_ship_q));

	SaveEvents(doc.get(), root);
	// Black globes in HyperSpace and QuasiSpace
	SaveEncounters(doc.get(), root);
	SaveStarInfo(doc.get(), root);
	SaveGroups(doc.get(), root);
	SaveStarDesc(doc.get(), root, SD);

	size_t length {};
	const uqstl::unique_ptr<char, decltype(&free)> text {
		yyjson_mut_write(doc.get(), YYJSON_WRITE_PRETTY_TWO_SPACES | YYJSON_WRITE_ALLOW_INVALID_UNICODE | YYJSON_WRITE_NEWLINE_AT_END, &length),
		&free};
	const uqstl::string file {saveFileName(which_game)};
	if (!text || !uqm::writeTextFile(uqm::saveDir(), file.c_str(), {text.get(), length}))
	{
		DeleteResFile(uqm::saveDir(), file.c_str());
		return false;
	}
	return true;
}
