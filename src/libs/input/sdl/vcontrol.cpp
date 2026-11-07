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

#include "vcontrol.h"

#include <ctype.h>
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "core/thread/Mutex.h"
#include "keynames.h"
#include "libs/reslib.h"
#include "port.h"

/* How many binding slots are allocated at once. */
static constexpr auto PoolChunkSize {64};

/* Total number of key input buckets. SDL scatters key symbols through
 * the entire 32-bit space, so we do not rely on being able to declare
 * an array with one entry per key. */
static constexpr auto KeyboardInputBuckets {512};

struct keybinding
{
	uqstl::atomic<int>* target;
	sdl_key_t keycode;
	struct keypool* parent;
	struct keybinding* next;
};

struct keypool
{
	keybinding pool[PoolChunkSize];
	int remaining;
	struct keypool* next;
};

// The main thread handles input events while the game thread can change the bindings.
static uqm::RecursiveMutex s_vcontrolLock {};

#ifdef HAVE_JOYSTICK

struct axis_type
{
	keybinding *neg, *pos;
	int polarity;
};

struct hat_type
{
	keybinding *left, *right, *up, *down;
	Uint8 last;
};

struct joystick
{
	SDL_Gamepad* stick;
	int numaxes, numbuttons;
	int threshold;
	uqstl::vector<axis_type> axes;
	uqstl::vector<keybinding*> buttons;
};


struct controller_list
{
	joystick gamepad;
	struct controller_list* next;
	SDL_JoystickID instance_id;
};

static controller_list* controller_list_head UQM_GUARDED_BY(s_vcontrolLock) = nullptr;
static int controller_assignments[2] UQM_GUARDED_BY(s_vcontrolLock) = {-1, -1};
static int active_controller_count UQM_GUARDED_BY(s_vcontrolLock) = 0;

struct default_binding
{
	int is_axis_binding;
	int axis;
	int polarity;
	int button;
	uqstl::atomic<int>* target;
	int logical_port;
};

static uqstl::vector<default_binding> default_bindings UQM_GUARDED_BY(s_vcontrolLock) {};

#endif /* HAVE_JOYSTICK */

static keybinding* bindings[KeyboardInputBuckets] UQM_GUARDED_BY(s_vcontrolLock);

static keypool* pool UQM_GUARDED_BY(s_vcontrolLock);

/* Last interesting event */
static int event_ready UQM_GUARDED_BY(s_vcontrolLock);
static SDL_Event last_interesting UQM_GUARDED_BY(s_vcontrolLock);

static keypool* allocate_key_chunk(void) UQM_REQUIRES(s_vcontrolLock)
{
	auto* x {new keypool {}};
	x->remaining = PoolChunkSize;
	x->next = nullptr;
	for (int i {}; i < PoolChunkSize; i++)
	{
		x->pool[i].target = nullptr;
		x->pool[i].keycode = SDLK_UNKNOWN;
		x->pool[i].next = nullptr;
		x->pool[i].parent = x;
	}
	return x;
}

static void free_key_pool(keypool* x)
{
	while (x)
	{
		keypool* next {x->next};
		delete x;
		x = next;
	}
}

#ifdef HAVE_JOYSTICK

static void store_default_binding(int is_axis_binding, int axis, int polarity,
								  int button, uqstl::atomic<int>* target, int logical_port) UQM_REQUIRES(s_vcontrolLock)
{
	default_bindings.push_back(default_binding {is_axis_binding, axis, polarity, button, target, logical_port});
}

static void apply_default_bindings_to_controller(SDL_JoystickID instance_id,
												 int logical_port) UQM_REQUIRES(s_vcontrolLock)
{
	for (const default_binding& entry : default_bindings)
	{
		const default_binding* binding {&entry};

		if (binding->logical_port != logical_port)
		{
			continue;
		}

		if (binding->is_axis_binding)
		{
			VControl_AddJoyAxisBinding(instance_id, binding->axis,
									   binding->polarity, binding->target);
		}
		else
		{
			VControl_AddJoyButtonBinding(instance_id,
										 binding->button, binding->target);
		}
	}

	uqm::log::debug("Applied {} bindings for logical port {} "
					"(instance {})",
					default_bindings.size(), logical_port,
					instance_id);
}

void create_joystick(SDL_JoystickID instance_id)
{
	const uqm::LockGuard lock {s_vcontrolLock};

	if (!SDL_IsGamepad(instance_id))
	{
		return;
	}

	SDL_Gamepad* gamecontroller {SDL_OpenGamepad(instance_id)};
	if (!gamecontroller)
	{
		uqm::log::warn("VControl: Could not initialize "
					   "controller #{}",
					   instance_id);
		return;
	}


	controller_list* current {controller_list_head};
	while (current)
	{
		if (current->instance_id == instance_id)
		{
			uqm::log::warn("Controller instance {} already in list",
						   instance_id);
			SDL_CloseGamepad(gamecontroller);
			return;
		}
		current = current->next;
	}

	int logical_port = -1;
	for (int i = 0; i < 2; i++)
	{
		if (controller_assignments[i] == -1)
		{
			logical_port = i;
			break;
		}
	}

	if (logical_port == -1)
	{
		uqm::log::info("Maximum of 2 controllers already connected, "
					   "ignoring additional controller");
		SDL_CloseGamepad(gamecontroller);
		return;
	}
	controller_assignments[logical_port] = instance_id;
	active_controller_count++;

	controller_list* new_controller {new controller_list {}};
	joystick* x {&new_controller->gamepad};
	x->numaxes = SDL_GAMEPAD_AXIS_COUNT;
	x->numbuttons = SDL_GAMEPAD_BUTTON_COUNT;
	x->axes.assign(x->numaxes, axis_type {nullptr, nullptr, 0});
	x->buttons.assign(x->numbuttons, nullptr);
	x->stick = gamecontroller;
	x->threshold = 10000;

	new_controller->instance_id = instance_id;
	new_controller->next = controller_list_head;
	controller_list_head = new_controller;

	const char* name {SDL_GetGamepadName(gamecontroller)};
	uqm::log::info("VControl opened controller (logical port {}): "
				   "{} (instance {})",
				   logical_port, name ? name : "", instance_id);

	apply_default_bindings_to_controller(instance_id, logical_port);
}

static void destroy_joystick(SDL_JoystickID instance_id) UQM_REQUIRES(s_vcontrolLock)
{
	for (int i = 0; i < 2; i++)
	{
		if (controller_assignments[i] == instance_id)
		{
			controller_assignments[i] = -1;
			active_controller_count--;
			break;
		}
	}

	controller_list** prev {&controller_list_head};
	controller_list* current {controller_list_head};

	while (current)
	{
		if (current->instance_id == instance_id)
		{
			*prev = current->next;

			joystick* x {&current->gamepad};
			if (x->stick)
			{
				SDL_CloseGamepad(x->stick);
				x->stick = nullptr;
			}

			delete current;

			uqm::log::info(
				"Controller instance {} removed", instance_id);
			return;
		}
		prev = &current->next;
		current = current->next;
	}

	uqm::log::warn("Controller instance {} not found for removal",
				   instance_id);
}

#endif /* HAVE_JOYSTICK */

static void key_init(void) UQM_REQUIRES(s_vcontrolLock)
{
	unsigned int i {};
	pool = allocate_key_chunk();
	for (i = 0; i < KeyboardInputBuckets; i++)
	{
		bindings[i] = nullptr;
	}

#ifdef HAVE_JOYSTICK
	for (i = 0; i < 2; i++)
	{
		controller_assignments[i] = -1;
	}
	active_controller_count = 0;
	controller_list_head = nullptr;
	default_bindings.clear();
#else
	active_controller_count = 0;
#endif /* HAVE_JOYSTICK */
}

static void key_uninit(void) UQM_REQUIRES(s_vcontrolLock)
{
	free_key_pool(pool);
	for (unsigned int i {0}; i < KeyboardInputBuckets; i++)
	{
		bindings[i] = nullptr;
	}
	pool = nullptr;

#ifdef HAVE_JOYSTICK
	const controller_list* current = controller_list_head;
	const controller_list* next {};

	while (current)
	{
		next = current->next;
		destroy_joystick(current->instance_id);
		current = next;
	}
	controller_list_head = nullptr;
	active_controller_count = 0;

	default_bindings.clear();
#endif /* HAVE_JOYSTICK */
}

void VControl_Init(void)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	key_init();
}

void VControl_Uninit(void)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	key_uninit();
}

int VControl_SetJoyThreshold(int port, int threshold)
{
	const uqm::LockGuard lock {s_vcontrolLock};
#ifdef HAVE_JOYSTICK
	if (port >= 0)
	{
		controller_list* current = controller_list_head;
		while (current)
		{
			if (current->instance_id == port)
			{
				current->gamepad.threshold = threshold;
				return 0;
			}
			current = current->next;
		}
		return 0;
	}
	else
#else
	(void)port;
	(void)threshold;
#endif /* HAVE_JOYSTICK */
	{
		// log_add (log_Warning, "VControl_SetJoyThreshold passed "
		//	"illegal port {}", port);
		return -1;
	}
}

static void add_binding(keybinding** newptr, uqstl::atomic<int>* target, sdl_key_t keycode) UQM_REQUIRES(s_vcontrolLock)
{
	keybinding* newbinding {};
	keypool* searchbase {};
	int i {};

	/* Acquire a pointer to the keybinding * that we'll be
	 * overwriting.  Along the way, ensure we haven't already
	 * bound this symbol to this target.  If we have, return.*/
	while (*newptr != nullptr)
	{
		if (((*newptr)->target == target)
			&& ((*newptr)->keycode == keycode))
		{
			return;
		}
		newptr = &((*newptr)->next);
	}

	/* Now hunt through the binding pool for a free binding. */

	/* First, find a chunk with free spots in it */

	searchbase = pool;
	while (searchbase->remaining == 0)
	{
		/* If we're completely full, allocate a new chunk */
		if (searchbase->next == nullptr)
		{
			searchbase->next = allocate_key_chunk();
		}
		searchbase = searchbase->next;
	}

	/* Now find a free binding within it */

	newbinding = nullptr;
	for (i = 0; i < PoolChunkSize; i++)
	{
		if (searchbase->pool[i].target == nullptr)
		{
			newbinding = &searchbase->pool[i];
			break;
		}
	}

	/* Sanity check. */
	if (!newbinding)
	{
		uqm::log::warn(
			"add_binding failed to find a free binding slot!");
		return;
	}

	newbinding->target = target;
	newbinding->keycode = keycode;
	newbinding->next = nullptr;
	*newptr = newbinding;
	searchbase->remaining--;
}

static void remove_binding(keybinding** ptr, uqstl::atomic<int>* target, sdl_key_t keycode)
{
	if (!(*ptr))
	{
		/* Nothing bound to symbol; return. */
		return;
	}
	else if (((*ptr)->target == target) && ((*ptr)->keycode == keycode))
	{
		keybinding* todel = *ptr;
		*ptr = todel->next;
		todel->target = nullptr;
		todel->keycode = SDLK_UNKNOWN;
		todel->next = nullptr;
		todel->parent->remaining++;
	}
	else
	{
		keybinding* prev = *ptr;
		while (prev && prev->next != nullptr)
		{
			if (prev->next->target == target)
			{
				keybinding* todel = prev->next;
				prev->next = todel->next;
				todel->target = nullptr;
				todel->keycode = SDLK_UNKNOWN;
				todel->next = nullptr;
				todel->parent->remaining++;
			}
			prev = prev->next;
		}
	}
}

// Replaces the value of the target with func(value), also if another thread changes it.
template <typename Func>
static void updateTarget(uqstl::atomic<int>& target, Func func)
{
	int value {target.load()};
	while (!target.compare_exchange_weak(value, func(value)))
	{
	}
}

static void activate(keybinding* i, sdl_key_t keycode)
{
	while (i != nullptr)
	{
		if (i->keycode == keycode)
		{
			updateTarget(*i->target, [](int v) {
				return (v + 1) | VcontrolStartbit;
			});
		}
		i = i->next;
	}
}

static void deactivate(keybinding* i, sdl_key_t keycode)
{
	while (i != nullptr)
	{
		if (i->keycode == keycode)
		{
			updateTarget(*i->target, [](int v) {
				return (v & VcontrolMask) > 0 ? ((v & VcontrolMask) - 1) | (v & VcontrolStartbit) : v;
			});
		}
		i = i->next;
	}
}

static void event2gesture(SDL_Event* e, VControlGesture* g)
{
	switch (e->type)
	{
		case SDL_EVENT_KEY_DOWN:
			g->type = VControlGestureType::Key;
			g->gesture.key = e->key.key;
			break;
#ifdef HAVE_JOYSTICK
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			g->type = VControlGestureType::JoyAxis;
			g->gesture.axis.port = e->gaxis.which;
			g->gesture.axis.index = e->gaxis.axis;
			g->gesture.axis.polarity = (e->gaxis.value < 0) ? -1 : 1;
			break;
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			g->type = VControlGestureType::JoyButton;
			g->gesture.button.port = e->gbutton.which;
			g->gesture.button.index = e->gbutton.button;
			break;
#endif /* HAVE_JOYSTICK */
		default:
			g->type = VControlGestureType::None;
			break;
	}
}

int VControl_AddGestureBinding(VControlGesture* g, uqstl::atomic<int>* target)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	int result = -1;
	int logical_port {};
	switch (g->type)
	{
		case VControlGestureType::Key:
			result = VControl_AddKeyBinding(g->gesture.key, target);
			break;

		case VControlGestureType::JoyAxis:
#ifdef HAVE_JOYSTICK
			logical_port = g->gesture.axis.port;

			store_default_binding(1, g->gesture.axis.index,
								  (g->gesture.axis.polarity < 0) ? -1 : 1,
								  -1, target, logical_port);

			if (logical_port >= 0 && logical_port < 2 && controller_assignments[logical_port] != -1)
			{
				result = VControl_AddJoyAxisBinding(
					controller_assignments[logical_port],
					g->gesture.axis.index,
					(g->gesture.axis.polarity < 0) ? -1 : 1, target);
			}
			else
			{
				result = 0;
			}
			break;
#endif
		case VControlGestureType::JoyButton:
#ifdef HAVE_JOYSTICK
			logical_port = g->gesture.button.port;

			store_default_binding(0, -1, 0,
								  g->gesture.button.index,
								  target, logical_port);

			if (logical_port >= 0 && logical_port < 2 && controller_assignments[logical_port] != -1)
			{
				result = VControl_AddJoyButtonBinding(
					controller_assignments[logical_port],
					g->gesture.button.index, target);
			}
			else
			{
				result = 0;
			}
			break;
#endif /* HAVE_JOYSTICK */
		case VControlGestureType::None:
			/* Do nothing */
			break;

		default:
			uqm::log::warn("VControl_AddGestureBinding didn't "
						   "understand argument gesture");
			result = -1;
			break;
	}
	return result;
}

void VControl_RemoveGestureBinding(VControlGesture* g, uqstl::atomic<int>* target)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	switch (g->type)
	{
		case VControlGestureType::Key:
			VControl_RemoveKeyBinding(g->gesture.key, target);
			break;

		case VControlGestureType::JoyAxis:
#ifdef HAVE_JOYSTICK
			VControl_RemoveJoyAxisBinding(g->gesture.axis.port,
										  g->gesture.axis.index,
										  (g->gesture.axis.polarity < 0) ? -1 : 1, target);
			break;
#endif /* HAVE_JOYSTICK */
		case VControlGestureType::JoyButton:
#ifdef HAVE_JOYSTICK
			VControl_RemoveJoyButtonBinding(g->gesture.button.port,
											g->gesture.button.index, target);
			break;
#endif /* HAVE_JOYSTICK */
		case VControlGestureType::None:
			break;
		default:
			uqm::log::warn("VControl_RemoveGestureBinding didn't "
						   "understand argument gesture");
			break;
	}
}

int VControl_AddKeyBinding(sdl_key_t symbol, uqstl::atomic<int>* target)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	add_binding(&bindings[symbol % KeyboardInputBuckets], target,
				symbol);
	return 0;
}

void VControl_RemoveKeyBinding(sdl_key_t symbol, uqstl::atomic<int>* target)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	remove_binding(&bindings[symbol % KeyboardInputBuckets], target,
				   symbol);
}

int VControl_AddJoyAxisBinding(int port, int axis, int polarity, uqstl::atomic<int>* target)
{
	const uqm::LockGuard lock {s_vcontrolLock};
#ifdef HAVE_JOYSTICK
	if (port >= 0)
	{
		controller_list* current = controller_list_head;
		while (current)
		{
			if (current->instance_id == port)
			{
				joystick* j = &current->gamepad;

				if ((axis >= 0) && (axis < j->numaxes))
				{
					if (polarity < 0)
					{
						add_binding(&j->axes[axis].neg, target,
									SDLK_UNKNOWN);
					}
					else if (polarity > 0)
					{
						add_binding(&j->axes[axis].pos, target,
									SDLK_UNKNOWN);
					}
					else
					{
						uqm::log::debug("VControl: Attempted to bind "
										"to polarity zero");
						return -1;
					}
				}
				else
				{
					// log_add (log_Debug, "VControl: Attempted to bind to"
					//	" illegal axis {}", axis);
					return -1;
				}
			}
			current = current->next;
		}
	}
	else
#else
	(void)port;
	(void)axis;
	(void)polarity;
	(void)target;
#endif /* HAVE_JOYSTICK */
	{
		// log_add (log_Debug, "VControl: Attempted to bind to illegal "
		//	"port {}", port);
		return -1;
	}
	return 0;
}

void VControl_RemoveJoyAxisBinding(int port, int axis,
								   int polarity, uqstl::atomic<int>* target)
{
	const uqm::LockGuard lock {s_vcontrolLock};
#ifdef HAVE_JOYSTICK
	if (port >= 0)
	{
		controller_list* current = controller_list_head;
		while (current)
		{
			if (current->instance_id == port)
			{
				joystick* j = &current->gamepad;
				if ((axis >= 0) && (axis < j->numaxes))
				{
					if (polarity < 0)
					{
						remove_binding(&j->axes[axis].neg, target,
									   SDLK_UNKNOWN);
					}
					else if (polarity > 0)
					{
						remove_binding(&j->axes[axis].pos, target,
									   SDLK_UNKNOWN);
					}
					else
					{
						uqm::log::debug("VControl: Attempted to "
										"unbind from polarity zero");
					}
				}
				else
				{
					uqm::log::debug("VControl: Attempted to unbind "
									"from illegal axis {}",
									axis);
				}
				return;
			}
			current = current->next;
		}
	}
	else
#else
	(void)port;
	(void)axis;
	(void)polarity;
	(void)target;
#endif /* HAVE_JOYSTICK */
	{
		uqm::log::debug("VControl: Attempted to unbind from illegal "
						"port {}",
						port);
	}
}

int VControl_AddJoyButtonBinding(int port, int button, uqstl::atomic<int>* target)
{
	const uqm::LockGuard lock {s_vcontrolLock};
#ifdef HAVE_JOYSTICK
	if (port >= 0)
	{
		controller_list* current = controller_list_head;
		while (current)
		{
			if (current->instance_id == port)
			{
				joystick* j = &current->gamepad;

				if ((button >= 0) && (button < j->numbuttons))
				{
					add_binding(&j->buttons[button], target,
								SDLK_UNKNOWN);
				}
				else
				{
					// log_add (log_Debug, "VControl: Attempted to bind to"
					//	" illegal button {}", button);
					return -1;
				}
			}
			current = current->next;
		}
		return 0;
	}
	else
#else
	(void)port;
	(void)button;
	(void)target;
#endif /* HAVE_JOYSTICK */
	{
		uqm::log::debug("VControl: Attempted to unbind from illegal "
						"port {}",
						port);
		return -1;
	}
}

void VControl_RemoveJoyButtonBinding(int port, int button, uqstl::atomic<int>* target)
{
	const uqm::LockGuard lock {s_vcontrolLock};
#ifdef HAVE_JOYSTICK
	if (port >= 0)
	{
		controller_list* current = controller_list_head;
		while (current)
		{
			if (current->instance_id == port)
			{
				joystick* j = &current->gamepad;
				if ((button >= 0) && (button < j->numbuttons))
				{
					remove_binding(&j->buttons[button], target,
								   SDLK_UNKNOWN);
				}
				else
				{
					uqm::log::debug("VControl: Attempted to unbind "
									"from illegal button {}",
									button);
				}
				return;
			}
			current = current->next;
		}
	}
	else
#else
	(void)port;
	(void)button;
	(void)target;
#endif /* HAVE_JOYSTICK */
	{
		uqm::log::debug("VControl: Attempted to unbind from illegal "
						"port {}",
						port);
	}
}


void VControl_RemoveAllBindings(void)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	key_uninit();
	key_init();
}

void VControl_ProcessKeyDown(sdl_key_t symbol)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	activate(bindings[symbol % KeyboardInputBuckets], symbol);
}

void VControl_ProcessKeyUp(sdl_key_t symbol)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	deactivate(bindings[symbol % KeyboardInputBuckets], symbol);
}

void VControl_ProcessJoyButtonDown(int port, int button)
{
	const uqm::LockGuard lock {s_vcontrolLock};
#ifdef HAVE_JOYSTICK
	controller_list* current = controller_list_head;
	while (current)
	{
		if (current->instance_id == port)
		{
			activate(current->gamepad.buttons[button], SDLK_UNKNOWN);
			return;
		}
		current = current->next;
	}
#else
	(void)port;
	(void)button;
#endif /* HAVE_JOYSTICK */
}

void VControl_ProcessJoyButtonUp(int port, int button)
{
	const uqm::LockGuard lock {s_vcontrolLock};
#ifdef HAVE_JOYSTICK
	controller_list* current = controller_list_head;
	while (current)
	{
		if (current->instance_id == port)
		{
			deactivate(current->gamepad.buttons[button], SDLK_UNKNOWN);
			return;
		}
		current = current->next;
	}
#else
	(void)port;
	(void)button;
#endif /* HAVE_JOYSTICK */
}

void VControl_ProcessJoyAxis(int port, int axis, int value)
{
	const uqm::LockGuard lock {s_vcontrolLock};
#ifdef HAVE_JOYSTICK
	controller_list* current = controller_list_head;
	while (current)
	{
		if (current->instance_id == port)
		{
			joystick* j = &current->gamepad;
			const int t = j->threshold;

			if (axis < 0 || axis >= j->numaxes)
			{
				return;
			}

			if (value > t)
			{
				if (j->axes[axis].polarity != 1)
				{
					if (j->axes[axis].polarity == -1)
					{
						deactivate(j->axes[axis].neg, SDLK_UNKNOWN);
					}
					j->axes[axis].polarity = 1;
					activate(j->axes[axis].pos, SDLK_UNKNOWN);
				}
			}
			else if (value < -t)
			{
				if (j->axes[axis].polarity != -1)
				{
					if (j->axes[axis].polarity == 1)
					{
						deactivate(j->axes[axis].pos, SDLK_UNKNOWN);
					}
					j->axes[axis].polarity = -1;
					activate(j->axes[axis].neg, SDLK_UNKNOWN);
				}
			}
			else
			{
				if (j->axes[axis].polarity == -1)
				{
					deactivate(j->axes[axis].neg, SDLK_UNKNOWN);
				}
				else if (j->axes[axis].polarity == 1)
				{
					deactivate(j->axes[axis].pos, SDLK_UNKNOWN);
				}
				j->axes[axis].polarity = 0;
			}
			return;
		}
		current = current->next;
	}
#else
	(void)port;
	(void)axis;
	(void)value;
#endif /* HAVE_JOYSTICK */
}


void VControl_ResetInput(void)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	/* Step through every valid entry in the binding pool and zero
	 * them out.  This will probably zero entries multiple times;
	 * oh well, no harm done. */

	const keypool* base = pool;
	while (base != nullptr)
	{
		for (int i {0}; i < PoolChunkSize; i++)
		{
			if (base->pool[i].target)
			{
				*(base->pool[i].target) = 0;
			}
		}
		base = base->next;
	}
}

void VControl_BeginFrame(void)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	/* Step through every valid entry in the binding pool and zero
	 * out the frame-start bit.  This will probably zero entries
	 * multiple times; oh well, no harm done. */

	keypool* base = pool;
	while (base != nullptr)
	{
		for (int i {0}; i < PoolChunkSize; i++)
		{
			if (base->pool[i].target)
			{
				base->pool[i].target->fetch_and(VcontrolMask);
			}
		}
		base = base->next;
	}
}

void VControl_HandleEvent(const SDL_Event* e)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	switch (e->type)
	{
		case SDL_EVENT_KEY_DOWN:
			if (!e->key.repeat)
			{
				VControl_ProcessKeyDown(e->key.key);
				last_interesting = *e;
				event_ready = 1;
			}
			break;
		case SDL_EVENT_KEY_UP:
			VControl_ProcessKeyUp(e->key.key);
			break;

#ifdef HAVE_JOYSTICK
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			VControl_ProcessJoyAxis(e->gaxis.which, e->gaxis.axis,
									e->gaxis.value);
			if ((e->gaxis.value > 15000) || (e->gaxis.value < -15000))
			{
				last_interesting = *e;
				event_ready = 1;
			}
			break;
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			VControl_ProcessJoyButtonDown(e->gbutton.which,
										  e->gbutton.button);
			last_interesting = *e;
			event_ready = 1;
			break;
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
			VControl_ProcessJoyButtonUp(e->gbutton.which,
										e->gbutton.button);
			break;
		case SDL_EVENT_GAMEPAD_ADDED:
			create_joystick(e->gdevice.which);
			break;
		case SDL_EVENT_GAMEPAD_REMOVED:
			destroy_joystick(e->gdevice.which);
			break;
		case SDL_EVENT_GAMEPAD_REMAPPED:
			uqm::log::info("Controller mapping updated");
			break;
#endif /* HAVE_JOYSTICK */

		default:
			break;
	}
}

/* Tracking the last interesting event */

void VControl_ClearGesture(void)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	event_ready = 0;
}

int VControl_GetLastGesture(VControlGesture* g)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	if (event_ready && g != nullptr)
	{
		event2gesture(&last_interesting, g);
	}
	return event_ready;
}

/* Configuration file grammar is as follows:  One command per line, 
 * hashes introduce comments that persist to end of line.  Blank lines
 * are ignored.
 *
 * Terminals are represented here as quoted strings, e.g. "foo" for 
 * the literal string foo.  These are matched case-insensitively.
 * Special terminals are:
 *
 * KEYNAME:  This names a key, as defined in keynames.c.
 * IDNAME:   This is an arbitrary string of alphanumerics, 
 *           case-insensitive, and ending with a colon.  This
 *           names an application-specific control value.
 * NUM:      This is an unsigned integer.
 * EOF:      End of file
 *
 * Nonterminals (the grammar itself) have the following productions:
 * 
 * configline <- IDNAME binding
 *             | "joystick" NUM "threshold" NUM
 *             | "version" NUM
 *
 * binding    <- "key" KEYNAME
 *             | "joystick" NUM joybinding
 *
 * joybinding <- "axis" NUM polarity
 *             | "button" NUM
 *             | "hat" NUM direction
 *
 * polarity   <- "positive" | "negative"
 *
 * dir        <- "up" | "down" | "left" | "right"
 *
 * This grammar is amenable to simple recursive descent parsing;
 * in fact, it's fully LL(1). */

/* Actual maximum line and token sizes are two less than this, since
 * we need space for the \n\0 at the end */
static constexpr auto LineSize {256};
static constexpr auto TokenSize {64};

struct parse_state
{
	char line[LineSize] {};
	char token[TokenSize] {};
	int index {};
	int error {};
	int linenum {};
};

static void next_token(parse_state* state)
{
	int index {};
	int base {};

	state->token[0] = 0;
	/* skip preceding whitespace */
	base = state->index;
	while (state->line[base] && isspace(state->line[base]))
	{
		base++;
	}

	index = 0;
	while (index < (TokenSize - 1) && state->line[base + index]
		   && !isspace(state->line[base + index]))
	{
		state->token[index] = state->line[base + index];
		index++;
	}
	state->token[index] = 0;

	/* If the token was too long, skip ahead until we get to whitespace */
	while (state->line[base + index] && !isspace(state->line[base + index]))
	{
		index++;
	}

	state->index = base + index;
}

static void expected_error(parse_state* state, const char* expected)
{
	uqm::log::warn("VControl: Expected '{}' on config file line {}",
				   expected, state->linenum);
	state->error = 1;
}

static void consume(parse_state* state, const char* expected)
{
	if (strcasecmp(expected, state->token))
	{
		expected_error(state, expected);
	}
	next_token(state);
}

static int consume_keyname(parse_state* state)
{
	const int keysym = VControl_name2code(state->token);
	if (!keysym)
	{
		uqm::log::warn("VControl: Illegal key name '{}' on config "
					   "file line {}",
					   state->token, state->linenum);
		state->error = 1;
	}
	next_token(state);
	return keysym;
}

static int consume_num(parse_state* state)
{
	char* end {};
	const int result = strtol(state->token, &end, 10);
	if (*end != '\0')
	{
		uqm::log::warn(
			"VControl: Expected integer on config line {}",
			state->linenum);
		state->error = 1;
	}
	next_token(state);
	return result;
}

static int consume_polarity(parse_state* state)
{
	int result = 0;
	if (!strcasecmp(state->token, "positive"))
	{
		result = 1;
	}
	else if (!strcasecmp(state->token, "negative"))
	{
		result = -1;
	}
	else
	{
		expected_error(state, "positive' or 'negative");
	}
	next_token(state);
	return result;
}

static Uint8 consume_dir(parse_state* state)
{
	Uint8 result = 0;
	if (!strcasecmp(state->token, "left"))
	{
		result = SDL_HAT_LEFT;
	}
	else if (!strcasecmp(state->token, "right"))
	{
		result = SDL_HAT_RIGHT;
	}
	else if (!strcasecmp(state->token, "up"))
	{
		result = SDL_HAT_UP;
	}
	else if (!strcasecmp(state->token, "down"))
	{
		result = SDL_HAT_DOWN;
	}
	else
	{
		expected_error(state, "left', 'right', 'up' or 'down");
	}
	next_token(state);
	return result;
}

static void parse_joybinding(parse_state* state, VControlGesture* gesture)
{
	consume(state, "joystick");
	const int sticknum {consume_num(state)};
	if (!state->error)
	{
		if (!strcasecmp(state->token, "axis"))
		{
			consume(state, "axis");
			const int axisnum {consume_num(state)};
			if (!state->error)
			{
				const int polarity = consume_polarity(state);
				if (!state->error)
				{
					gesture->type = VControlGestureType::JoyAxis;
					gesture->gesture.axis.port = sticknum;
					gesture->gesture.axis.index = axisnum;
					gesture->gesture.axis.polarity = polarity;
				}
			}
		}
		else if (!strcasecmp(state->token, "button"))
		{
			consume(state, "button");
			const int buttonnum {consume_num(state)};
			if (!state->error)
			{
				gesture->type = VControlGestureType::JoyButton;
				gesture->gesture.button.port = sticknum;
				gesture->gesture.button.index = buttonnum;
			}
		}
		else if (!strcasecmp(state->token, "hat"))
		{
			consume(state, "hat");
			const int hatnum {consume_num(state)};
			if (!state->error)
			{
				const Uint8 dir = consume_dir(state);
				if (!state->error)
				{
					gesture->type = VControlGestureType::JoyHat;
					gesture->gesture.hat.port = sticknum;
					gesture->gesture.hat.index = hatnum;
					gesture->gesture.hat.dir = dir;
				}
			}
		}
		else
		{
			expected_error(state, "axis', 'button', or 'hat");
		}
	}
}

static void parse_gesture(parse_state* state, VControlGesture* gesture)
{
	gesture->type = VControlGestureType::None; /* Default to error */
	if (!strcasecmp(state->token, "key"))
	{
		/* Parse key binding */
		consume(state, "key");
		const int keysym {consume_keyname(state)};
		if (!state->error)
		{
			gesture->type = VControlGestureType::Key;
			gesture->gesture.key = keysym;
		}
	}
	else if (!strcasecmp(state->token, "joystick"))
	{
		parse_joybinding(state, gesture);
	}
	else
	{
		expected_error(state, "key' or 'joystick");
	}
}

void VControl_ParseGesture(VControlGesture* g, const char* spec)
{
	const uqm::LockGuard lock {s_vcontrolLock};
	parse_state ps {};

	uqm::strncpy_safe(ps.line, spec);
	ps.index = ps.error = 0;
	ps.linenum = -1;

	next_token(&ps);
	parse_gesture(&ps, g);
	if (ps.error)
	{
		uqm::log::error("Error parsing {}\n", spec);
	}
}

size_t VControl_DumpGesture(uqstl::span<char> buf, VControlGesture* g)
{
	switch (g->type)
	{
		case VControlGestureType::Key:
			return fmt::format_to_sz_n(buf, "key {}",
									   VControl_code2name(g->gesture.key))
				.size;
		case VControlGestureType::JoyAxis:
			return fmt::format_to_sz_n(buf, "joystick {} axis {} {}",
									   g->gesture.axis.port, g->gesture.axis.index,
									   (g->gesture.axis.polarity > 0) ? "positive" : "negative")
				.size;
		case VControlGestureType::JoyButton:
			return fmt::format_to_sz_n(buf, "joystick {} button {}",
									   g->gesture.button.port, g->gesture.button.index)
				.size;

		default:
			buf[0] = '\0';
			return 0;
	}
}