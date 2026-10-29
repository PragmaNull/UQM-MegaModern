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
#include <errno.h>
#include <string.h>
#include "input.h"
#include "../inpintrn.h"
#include "libs/threadlib.h"
#include "libs/input/sdl/vcontrol.h"
#include "libs/input/sdl/keynames.h"
#include "libs/memlib.h"
#include "libs/file.h"
#include "core/log/log.h"
#include "libs/reslib.h"
#include "options.h"
#include "core/string/StringUtils.h"
#include "libs/file/textfile.h"
#include "libs/input/keyconfig.h"

#define KBDBUFSIZE (1 << 8)
static int kbdhead = 0, kbdtail = 0;
static UniChar kbdbuf[KBDBUFSIZE];
static UniChar lastchar;

#if SDL_MAJOR_VERSION == 1
static int num_keys = 0;
static int* kbdstate = nullptr;
// Holds all SDL keys +1 for holding invalid values
#else  // Later versions of SDL use the text input API instead
static bool set_character_mode = false;
// Records whether the UI thread has caught up with game thread
// on this setting
#endif // SDL_MAJOR_VERSION

static volatile int* menu_vec;
static int num_menu;
// The last vector element is the character repeat "key"
// This is only used in SDL1 input but it's mostly harmless everywhere else
#define KEY_MENU_ANY (num_menu - 1)
static volatile int* flight_vec;
static int num_templ;
static int num_flight;

static bool InputInitialized = false;

static bool in_character_mode = false;

static const char* menu_res_names[] = {
	"pause",
	"exit",
	"abort",
	"debug",
	"fullscreen",
	"up",
	"down",
	"left",
	"right",
	"select",
	"cancel",
	"special",
	"pageup",
	"pagedown",
	"home",
	"end",
	"zoomin",
	"zoomout",
	"delete",
	"backspace",
	"editcancel",
	"search",
	"next",
	"togglemap",
	"screenshot",
	"debug_2",
	"debug_3",
	"debug_4",
	nullptr};

static const char* flight_res_names[] = {
	"up",
	"down",
	"left",
	"right",
	"weapon",
	"special",
	"escape",
	"thrust",
	nullptr};

static void
register_menu_controls(int index)
{
	int i;
	char buf[40] {};

	i = 1;
	while (true)
	{
		VCONTROL_GESTURE g;

		fmt::format_to_sz_n(buf, "menu.{}.{}", menu_res_names[index], i);

		if (!res_IsString(buf))
		{
			break;
		}
		VControl_ParseGesture(&g, res_GetString(buf));
		VControl_AddGestureBinding(&g, (int*)&menu_vec[index]);
		i++;
	}
}

static VCONTROL_GESTURE* controls;

static constexpr const char* c_keyConfigFile {"keys.json"};

static uqstl::span<const uqgsl::czstring> flightControlNames()
{
	return {flight_res_names, static_cast<size_t>(num_flight)};
}

// Loads the user key configuration into the "keys." resources.
static bool loadKeyConfiguration()
{
	const auto json {uqm::readTextFile(uqm::configDir(), c_keyConfigFile)};
	if (!json)
	{
		return false;
	}

	const auto templates {uqm::keyConfigFromJson(*json, flightControlNames())};
	if (!templates)
	{
		uqm::moveToBadFile(uqm::configDir(), c_keyConfigFile);
		return false;
	}

	char key[40] {};
	for (size_t templ = 0; templ < templates->size() && templ < static_cast<size_t>(num_templ); ++templ)
	{
		const uqm::KeyTemplate& keyTemplate {(*templates)[templ]};
		fmt::format_to_sz_n(key, "keys.{}.name", templ + 1);
		res_PutString(key, keyTemplate.name.c_str());

		for (size_t control = 0; control < keyTemplate.bindings.size(); ++control)
		{
			const auto& gestures {keyTemplate.bindings[control]};
			for (size_t alternate = 0; alternate < gestures.size() && alternate < MAX_FLIGHT_ALTERNATES; ++alternate)
			{
				fmt::format_to_sz_n(key, "keys.{}.{}.{}", templ + 1, flight_res_names[control], alternate + 1);
				res_PutString(key, gestures[alternate].c_str());
			}
		}
	}
	return true;
}
#define CONTROL_PTR(i, j, k) \
	(controls + ((i) * num_flight + (j)) * MAX_FLIGHT_ALTERNATES + (k))

static void
register_flight_controls(void)
{
	int i, j, k;
	char buf[40] {};


	for (i = 0; i < num_templ; i++)
	{
		/* Copy in name */
		fmt::format_to_sz_n(buf, "keys.{}.name", i + 1);
		if (res_IsString(buf))
		{
			uqm::strncpy_safe(uqm::input_templates[i].name, res_GetString(buf));
		}
		else
		{
			uqm::input_templates[i].name[0] = '\0';
		}
		for (j = 0; j < num_flight; j++)
		{
			for (k = 0; k < MAX_FLIGHT_ALTERNATES; k++)
			{
				VCONTROL_GESTURE* g = CONTROL_PTR(i, j, k);
				fmt::format_to_sz_n(buf, "keys.{}.{}.{}", i + 1,
									flight_res_names[j], k + 1);
				if (!res_IsString(buf))
				{
					g->type = VCONTROL_NONE;
					continue;
				}
				VControl_ParseGesture(g, res_GetString(buf));
				VControl_AddGestureBinding(g,
										   (int*)(flight_vec + i * num_flight + j));
			}
		}
	}
}

static void
initKeyConfig(void)
{
	int i;

	if (!menu_vec || !flight_vec)
	{
		uqm::log::critical("initKeyConfig(): invalid input vectors");
		exit(EXIT_FAILURE);
	}

	controls = (VCONTROL_GESTURE*)HCalloc(sizeof(*controls) * num_templ * num_flight
										  * MAX_FLIGHT_ALTERNATES);

	/* First, load in the menu keys */
	LoadResourceIndex(uqm::contentDir(), "menu.key", "menu.");
	LoadResourceIndex(uqm::configDir(), "override.cfg", "menu.");
	for (i = 0; i < num_menu; i++)
	{
		if (!menu_res_names[i])
		{
			break;
		}
		register_menu_controls(i);
	}

	if (!loadKeyConfiguration())
	{
		LoadResourceIndex(uqm::contentDir(), "uqm.key", "keys.");
	}

	register_flight_controls();

	return;
}

static void
resetKeyboardState(void)
{
#if SDL_MAJOR_VERSION == 1
	memset(kbdstate, 0, sizeof(int) * num_keys);
	menu_vec[KEY_MENU_ANY] = 0;
#endif // SDL_MAJOR_VERSION
}

void TFB_SetInputVectors(int menu[], int num_menu_, int flight[], int num_flight_, int num_templ_)
{
	if (num_menu_ < 0 || num_templ_ < 0 || num_flight_ < 0)
	{
		uqm::log::critical("TFB_SetInputVectors(): invalid vector size");
		exit(EXIT_FAILURE);
	}
	menu_vec = menu;
	num_menu = num_menu_;
	flight_vec = flight;
	num_templ = num_templ_;
	num_flight = num_flight_;
}

#ifdef HAVE_JOYSTICK

static void
initJoystick(void)
{
	int nJoysticks;

#if SDL_MAJOR_VERSION > 1
	char* mapping_db;
	int len;
	size_t base_len;
	const char* slash;
#endif

#if SDL_MAJOR_VERSION > 1
	if ((SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)) == -1)
#else
	if ((SDL_InitSubSystem(SDL_INIT_JOYSTICK)) == -1)
#endif
	{
		uqm::log::critical("Couldn't initialize joystick subsystem: {}",
						   SDL_GetError());
		exit(EXIT_FAILURE);
	}

#if SDL_MAJOR_VERSION > 1
	SDL_GameControllerEventState(SDL_ENABLE);

	base_len = strlen(baseContentPath);
	if (base_len > 0)
	{
		char last_char = baseContentPath[base_len - 1];
		slash = (last_char == '/' || last_char == '\\') ? "" : "/";
	}
	else
	{
		slash = "/";
	}

	static constexpr const char* DbFilenameFormat {"{}{}gamecontrollerdb.txt"};
	len = fmt::formatted_size(DbFilenameFormat, baseContentPath, slash);

	mapping_db = (char*)HMalloc(len + 1);

	fmt::format_to_sz_n(mapping_db, len + 1, DbFilenameFormat,
						baseContentPath, slash);

	if (SDL_GameControllerAddMappingsFromFile(mapping_db) == -1)
	{
		uqm::log::warn("Could not load controller mappings "
					   "from {}: {}",
					   mapping_db, SDL_GetError());
	}
	else
	{
		uqm::log::debug("Loaded controller mappings from {}",
						mapping_db);
	}

	HFree(mapping_db);
#endif
	nJoysticks = SDL_NumJoysticks();
	uqm::log::info("{} joysticks were found.", nJoysticks);

	if (nJoysticks > 0)
	{
		int i;
		uqm::log::info("The names of the joysticks are:");
		for (i = 0; i < nJoysticks; i++)
		{
#if SDL_MAJOR_VERSION > 1
			if (SDL_IsGameController(i))
			{
				uqm::log::info("    {} (controller)",
							   SDL_GameControllerNameForIndex(i));
			}
			else
			{
				uqm::log::info("    {} (joystick)",
							   SDL_JoystickNameForIndex(i));
			}
#else
			uqm::log::info("    {}", SDL_JoystickName(i));
#endif
		}
#if SDL_MAJOR_VERSION == 1
		SDL_JoystickEventState(SDL_ENABLE);
#else
		for (int i = 0; i < nJoysticks; i++)
		{
			create_joystick(i);
		}
#endif
	}
}

#endif /* HAVE_JOYSTICK */

int TFB_InitInput(int driver, int flags)
{
	(void)driver;
	(void)flags;

#if SDL_MAJOR_VERSION == 1
	int signed_num_keys;
	SDL_EnableUNICODE(1);
	(void)SDL_GetKeyState(&signed_num_keys);
	(void)SDL_GetKeyState(&num_keys);
	kbdstate = (int*)HMalloc(sizeof(int) * (num_keys + 1));
#endif

#ifdef HAVE_JOYSTICK
	initJoystick();
#endif

	in_character_mode = false;
	resetKeyboardState();

	/* Prepare the Virtual Controller system. */
	VControl_Init();

	initKeyConfig();

	VControl_ResetInput();
	InputInitialized = true;

	return 0;
}

void TFB_UninitInput(void)
{
	VControl_Uninit();
	HFree(controls);
#if SDL_MAJOR_VERSION == 1
	HFree(kbdstate);
#else
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
#endif
}

void EnterCharacterMode(void)
{
	kbdhead = kbdtail = 0;
	lastchar = 0;
	in_character_mode = true;
	VControl_ResetInput();
}

void ExitCharacterMode(void)
{
	kbdhead = kbdtail = 0;
	lastchar = 0;
	in_character_mode = false;
	VControl_ResetInput();
}

UniChar
GetNextCharacter(void)
{
	UniChar result;
	if (kbdhead == kbdtail)
	{
		return 0;
	}
	result = kbdbuf[kbdhead];
	kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
	return result;
}

UniChar
GetLastCharacter(void)
{
	return lastchar;
}

volatile int MouseButtonDown = 0;

#if 0
static void
ProcessMouseEvent (const SDL_Event *e)
{
	switch (e->type)
	{
	case SDL_MOUSEBUTTONDOWN:
		MouseButtonDown = 1;
		break;
	case SDL_MOUSEBUTTONUP:
		MouseButtonDown = 0;
		break;
	default:
		break;
	}
}
#endif

#if SDL_MAJOR_VERSION == 1

static inline int
is_numpad_char_event(const SDL_Event* Event)
{
	return in_character_mode && (Event->type == SDL_KEYDOWN || Event->type == SDL_KEYUP) && Event->key.keysym.unicode > 0 && /* Printable char */
		   Event->key.keysym.sym >= SDLK_KP0 &&																				 /* Keypad key */
		   Event->key.keysym.sym <= SDLK_KP_PERIOD;
}

void ProcessInputEvent(const SDL_Event* Event)
{
	if (!InputInitialized)
	{
		return;
	}

	// ProcessMouseEvent (Event);

	// In character mode with NumLock on, numpad chars bypass VControl
	// so that menu arrow events are not produced
	if (!is_numpad_char_event(Event))
	{
		VControl_HandleEvent(Event);
	}

	if (Event->type == SDL_KEYDOWN || Event->type == SDL_KEYUP)
	{ // process character input event, if any
		// keysym.sym is an SDLKey type which is an enum and can be signed
		// or unsigned on different platforms; we'll use a guaranteed type
		int k = Event->key.keysym.sym;
		UniChar map_key = Event->key.keysym.unicode;

		if (k < 0 || k > num_keys)
		{
			k = num_keys; // for unknown keys
		}

		if (Event->type == SDL_KEYDOWN)
		{
			int newtail;

			// dont care about the non-printable, non-char
			if (!map_key)
			{
				return;
			}

			kbdstate[k]++;

			newtail = (kbdtail + 1) & (KBDBUFSIZE - 1);
			// ignore the char if the buffer is full
			if (newtail != kbdhead)
			{
				kbdbuf[kbdtail] = map_key;
				kbdtail = newtail;
				lastchar = map_key;
				menu_vec[KEY_MENU_ANY]++;
			}
		}
		else if (Event->type == SDL_KEYUP)
		{
			if (kbdstate[k] == 0)
			{ // something is fishy -- better to reset the
				// repeatable state to avoid big problems
				menu_vec[KEY_MENU_ANY] = 0;
			}
			else
			{
				kbdstate[k]--;
				if (menu_vec[KEY_MENU_ANY] > 0)
				{
					menu_vec[KEY_MENU_ANY]--;
				}
			}
		}
	}
}

#else

static inline int
is_numpad_char_event(const SDL_Event* Event)
{
	return in_character_mode && (Event->type == SDL_KEYDOWN || Event->type == SDL_KEYUP) && (Event->key.keysym.mod & KMOD_NUM) && /* Numlock on */
		   Event->key.keysym.sym >= SDLK_KP_1 &&																				  /* Keypad key */
		   Event->key.keysym.sym <= SDLK_KP_PERIOD;
	// Note that in the SDL2 enumeration 0 comes after 9 and before period
}

void ProcessInputEvent(const SDL_Event* Event)
{
	if (!InputInitialized)
	{
		return;
	}

	if (in_character_mode && !set_character_mode)
	{
		set_character_mode = true;
		SDL_StartTextInput();
	}

	if (!in_character_mode && set_character_mode)
	{
		set_character_mode = false;
		SDL_StopTextInput();
	}

	/* "Block" numpad input when NUM_LOCK is on */
	if (!is_numpad_char_event(Event))
	{
		VControl_HandleEvent(Event);
	}

	if (Event->type == SDL_TEXTINPUT)
	{
		int newtail;
		int i = 0;

		while (Event->text.text[i])
		{
			UniChar map_key = Event->text.text[i++];
			map_key &= 0xFF; /* Interpret as unsigned byte */

			/* Decode any UTF-8 keys */
			if (map_key >= 0xC0 && map_key < 0xE0)
			{
				/* 2-byte UTF-8 */
				map_key = (map_key & 0x1f) << 6;
				map_key |= Event->text.text[i++] & 0x3f;
			}
			else if (map_key >= 0xE0 && map_key < 0xF0)
			{
				/* 3-byte UTF-8 */
				map_key = (map_key & 0x0f) << 6;
				map_key |= Event->text.text[i++] & 0x3f;
				map_key <<= 6;
				map_key |= Event->text.text[i++] & 0x3f;
			}
			else if (map_key >= 0xF0)
			{
				/* Out of the BMP, won't fit in a UniChar */
				/* Use the replacement character instead */
				map_key = 0xFFFD;
				while ((UniChar)Event->text.text[i] > 0x7F)
				{
					++i;
				}
			}

			/* dont care about the non-printable, non-char */
			if (!map_key)
			{
				return;
			}

			newtail = (kbdtail + 1) & (KBDBUFSIZE - 1);

			/* ignore the char if the buffer is full */
			if (newtail != kbdhead)
			{
				kbdbuf[kbdtail] = map_key;
				kbdtail = newtail;
				lastchar = map_key;
			}

			/* Loop back in case there are more chars in the
			 * text input buffer */
		}
	}
}

#endif // SDL_MAJOR_VERSION

void TFB_ResetControls(void)
{
	VControl_ResetInput();
	resetKeyboardState();
	// flush character buffer
	kbdhead = kbdtail = 0;
	lastchar = 0;
}

void InterrogateInputState(int templat, int control, int index, char* buffer,
						   int maxlen)
{
	VCONTROL_GESTURE* g = CONTROL_PTR(templat, control, index);

#if SDL_MAJOR_VERSION > 1
	const char xbx_buttons[SDL_CONTROLLER_BUTTON_MAX][16] =
		{
			"A", "B", "X", "Y", "Back", "Guide", "Start",
			"LStick", "RStick", "LShoulder", "RShoulder",
			"DUp", "DDown", "DLeft", "DRight", "Misc"};

	const char ds4_buttons[SDL_CONTROLLER_BUTTON_MAX][16] =
		{
			STR_CROSS, STR_CIRCLE, STR_SQUARE, STR_TRIANGLE,
			"Share", "PS", "Options", "L3", "R3", "L1", "R1",
			"DUp", "DDown", "DLeft", "DRight", "TouchPad"};

	const char xbx_axes[SDL_CONTROLLER_AXIS_MAX][16] =
		{
			"LStick H", "LStick V", "RStick H", "RStick V",
			"LTrigger", "RTrigger"};

	const char ds4_axes[SDL_CONTROLLER_AXIS_MAX][16] =
		{
			"LStick H", "LStick V", "RStick H", "RStick V",
			"L2", "R2"};
#endif

	if (templat >= num_templ || control >= num_flight
		|| index >= MAX_FLIGHT_ALTERNATES)
	{
		uqm::log::warn("InterrogateInputState(): invalid control index");
		buffer[0] = 0;
		return;
	}

	switch (g->type)
	{
		case VCONTROL_KEY:
			fmt::format_to_sz_n(buffer, maxlen, "{}",
								VControl_code2name(g->gesture.key));
			buffer[maxlen - 1] = 0;
			break;
		case VCONTROL_JOYBUTTON:
			switch (uqm::UQMOptions::read().optControllerType)
			{
#if SDL_MAJOR_VERSION > 1
				case uqm::ControllerType::XBox:
					fmt::format_to_sz_n(buffer, maxlen, "[J{} {}]",
										g->gesture.button.port,
										xbx_buttons[g->gesture.button.index]);
					break;
				case uqm::ControllerType::Playstation:
					fmt::format_to_sz_n(buffer, maxlen, "[J{} {}]",
										g->gesture.button.port,
										ds4_buttons[g->gesture.button.index]);
					break;
#endif
				case uqm::ControllerType::KeyboardMouse:
				default:
					fmt::format_to_sz_n(buffer, maxlen, "[J{} B{}]",
										g->gesture.button.port,
										g->gesture.button.index);
					break;
			}
			break;
		case VCONTROL_JOYAXIS:
			switch (uqm::UQMOptions::read().optControllerType)
			{
#if SDL_MAJOR_VERSION > 1
				case uqm::ControllerType::XBox:
					fmt::format_to_sz_n(buffer, maxlen, "[J{} {}{}]",
										g->gesture.axis.port,
										xbx_axes[g->gesture.axis.index],
										g->gesture.axis.polarity > 0 ? '+' : '-');
					break;
				case uqm::ControllerType::Playstation:
					fmt::format_to_sz_n(buffer, maxlen, "[J{} {}{}]",
										g->gesture.axis.port,
										ds4_axes[g->gesture.axis.index],
										g->gesture.axis.polarity > 0 ? '+' : '-');
					break;
#endif
				case uqm::ControllerType::KeyboardMouse:
				default:
					fmt::format_to_sz_n(buffer, maxlen, "[J{} A{} {}]",
										g->gesture.axis.port,
										g->gesture.axis.index,
										g->gesture.axis.polarity > 0 ? '+' : '-');
					break;
			}

			break;
#if SDL_MAJOR_VERSION == 1
		case VCONTROL_JOYHAT:
			fmt::format_to_sz_n(buffer, maxlen, "[J{} H{} {}]", g->gesture.hat.port,
								g->gesture.hat.index, g->gesture.hat.dir);
			break;
#endif
		default:
			/* Something we don't handle yet */
			buffer[0] = 0;
			break;
	}
	return;
}

void RemoveInputState(int templat, int control, int index)
{
	VCONTROL_GESTURE* g = CONTROL_PTR(templat, control, index);
	char keybuf[40] {};

	if (templat >= num_templ || control >= num_flight
		|| index >= MAX_FLIGHT_ALTERNATES)
	{
		uqm::log::warn("RemoveInputState(): invalid control index");
		return;
	}

	VControl_RemoveGestureBinding(g,
								  (int*)(flight_vec + templat * num_flight + control));
	g->type = VCONTROL_NONE;

	fmt::format_to_sz_n(keybuf, "keys.{}.{}.{}", templat + 1,
						flight_res_names[control], index + 1);
	res_Remove(keybuf);

	return;
}

void RebindInputState(int templat, int control, int index)
{
	VCONTROL_GESTURE g;
	char keybuf[40] {};
	char valbuf[40] {};

	if (templat >= num_templ || control >= num_flight
		|| index >= MAX_FLIGHT_ALTERNATES)
	{
		uqm::log::warn("RebindInputState(): invalid control index");
		return;
	}

	/* Remove the old binding on this spot */
	RemoveInputState(templat, control, index);

	/* Wait for the next interesting bit of user input */
	VControl_ClearGesture();
	while (!VControl_GetLastGesture(&g))
	{
		TaskSwitch();
	}

	/* And now, add the new binding. */
	VControl_AddGestureBinding(&g,
							   (int*)(flight_vec + templat * num_flight + control));
	*CONTROL_PTR(templat, control, index) = g;
	fmt::format_to_sz_n(keybuf, "keys.{}.{}.{}", templat + 1,
						flight_res_names[control], index + 1);
	VControl_DumpGesture({valbuf, sizeof(valbuf)}, &g);
	res_PutString(keybuf, valbuf);
}

void SaveKeyConfiguration(const uqm::io::Directory& path)
{
	uqstl::vector<uqm::KeyTemplate> templates(static_cast<size_t>(num_templ));
	char key[40] {};
	for (size_t templ = 0; templ < templates.size(); ++templ)
	{
		uqm::KeyTemplate& keyTemplate {templates[templ]};
		fmt::format_to_sz_n(key, "keys.{}.name", templ + 1);
		if (res_IsString(key))
		{
			keyTemplate.name = res_GetString(key);
		}

		keyTemplate.bindings.resize(static_cast<size_t>(num_flight));
		for (size_t control = 0; control < keyTemplate.bindings.size(); ++control)
		{
			for (size_t alternate = 0; alternate < MAX_FLIGHT_ALTERNATES; ++alternate)
			{
				fmt::format_to_sz_n(key, "keys.{}.{}.{}", templ + 1, flight_res_names[control], alternate + 1);
				if (res_IsString(key))
				{
					keyTemplate.bindings[control].emplace_back(res_GetString(key));
				}
			}
		}
	}

	if (!uqm::writeTextFile(path, c_keyConfigFile, uqm::keyConfigToJson(templates, flightControlNames())))
	{
		uqm::log::error("Could not write {}.", c_keyConfigFile);
	}
}

void BeginInputFrame(void)
{
	VControl_BeginFrame();
}