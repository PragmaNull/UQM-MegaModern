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

#pragma once
#ifndef UQM_ELEMENT_H
#define UQM_ELEMENT_H

#include "colors.h"
#include "core/types/enum.h"
#include "displist.h"
#include "libs/gfxlib.h"
#include "units.h"
#include "velocity.h"

namespace uqm
{
inline constexpr auto NormalLife {1};

using ElementHandle = LinkHandle;
struct Starship;

// Bits for ElementFlags:
// bits 0 and 1 are now available
enum class ElementFlags : uint16_t
{
	None = 0,
	PlayerShip = 1 << 2,
	// The Element is a player controlable ship, and not some bullet,
	// crew, asteroid, fighter, etc. This does not mean that the ship
	// is actually controlled by a human; it may be a computer.

	Appearing = 1 << 3,
	Disappearing = 1 << 4,
	Changing = 1 << 5,
	// The Element's graphical representation has changed.

	Nonsolid = 1 << 6,
	Collision = 1 << 7,
	IgnoreSimilar = 1 << 8,
	DefyPhysics = 1 << 9,

	FiniteLife = 1 << 10,

	PreProcess = 1 << 11,
	// PreProcess() is to be called for the Element.
	PostProcess = 1 << 12,

	IgnoreVelocity = 1 << 13,
	CrewObject = 1 << 14,
	BackgroundObject = 1 << 15,
};
ENUM_BITWISE_OPS(ElementFlags)
// The BackgroundObject flag existed originally but wasn't used.
// It can now be used for objects that never influence the state
// of other elements; elements that have this flag set are not
// included in the checksum used for netplay games.
// It can be used for graphical mods that don't impede netplay.


inline constexpr auto HyperjumpLife {15};

inline constexpr auto NumExplosionFrames {12};

inline constexpr auto GameSoundPriority {2};

enum class ViewState
{
	Stable,
	Scroll,
	Change
};


inline constexpr auto NoPrim {PrimitiveType::Count};

struct ElementState
{
	GfxPoint location;
	struct
	{
		FrameRef frame;
		FrameRef* farray;
	} image;
};


struct Element;

typedef void(ElementProcessFunc)(Element* ElementPtr);
typedef void(ElementCollisionFunc)(Element* ElementPtr0, GfxPoint* pPt0,
								   Element* ElementPtr1, GfxPoint* pPt1);

// Any physical object in the simulation.
struct Element
{
	// QueueLink elements; must be first
	ElementHandle pred, succ;

	ElementProcessFunc* preprocess_func;
	ElementProcessFunc* postprocess_func;
	ElementCollisionFunc* collision_func;
	ElementProcessFunc* death_func;

	// Player this Element belongs to
	// -1: neutral (planets, asteroids, crew, etc.)
	//  0: Melee: bottom player; Full-game: the human player
	//  1: Melee: top player;    Full-game: the NPC opponent
	int16_t playerNr;

	ElementFlags state_flags;
	union
	{
		uint16_t life_span;
		uint16_t scan_node; /* Planetside: scan type and node id */
	};
	union
	{
		uint16_t crew_level;
		uint16_t hit_points;
		uint16_t facing; /* Planetside: lava-spot direction of travel */
		uint16_t cycle;
		/* Planetside: lightning cycle length */
	};
	union
	{
		uint8_t mass_points;
		/* Planetside:
				 * - for living bio: Index in CreatureData, possibly OR'ed
				 *   with CreatureAware
				 * - for canned bio: value of creature
				 */
		// TODO: Use a different name for Planetside bio, like
		// uint8_t bio_state;
	};
	union
	{
		uint8_t turn_wait;
		uint8_t sys_loc; /* IP flagship: location in system */
	};
	union
	{
		uint8_t thrust_wait;
		uint8_t blast_offset;
		uint8_t next_turn; /* Battle: animation interframe for some elements */
	};
	uint8_t colorCycleIndex;
	// Melee: used to cycle ion trails and warp shadows, and
	//        to cycle the ship color when fleeing.

	VelocityDesc velocity;
	IntersectInfo IntersectControl;
	uint16_t PrimIndex;
	ElementState current, next;

	void* pParent;
	// The ship this Element belongs to.
	ElementHandle hTarget;
};

inline constexpr auto NeutralPlayerNum {-1};

static inline bool elementsOfSamePlayer(Element* ElementPtr0, Element* ElementPtr1)
{
	return (bool)(ElementPtr0->playerNr == ElementPtr1->playerNr);
}

extern Queue disp_q;
// The maximum number of elements is chosen to provide a slight margin.
// Currently, it is maximum *known used* in Melee + 30
inline constexpr auto MaxDisplayElements {150};

inline constexpr auto MaxDisplayPrims {330};
extern uint16_t DisplayFreeList;
extern Primitive DisplayArray[MaxDisplayPrims];

inline uint16_t AllocDisplayPrim()
{
	const uint16_t prim {DisplayFreeList};
	DisplayFreeList = GetSuccLink(GetPrimLinks(&DisplayArray[DisplayFreeList]));
	return prim;
}
inline void FreeDisplayPrim(uint16_t p)
{
	SetPrimLinks(&DisplayArray[p], EndOfList, DisplayFreeList);
	DisplayFreeList = p;
}

inline void GetElementStarShip(const Element* e, Starship** ppsd) { *ppsd = static_cast<Starship*>(e->pParent); }
inline void SetElementStarShip(Element* e, Starship* psd) { e->pParent = psd; }

inline constexpr auto MaxCrewSize {42};
inline constexpr auto MaxEnergySize {42};
inline constexpr auto MaxShipMass {10};
constexpr auto GravityMass(const auto& m) { return ((m) > MaxShipMass * 10); }
inline auto GravityThreshold() { return (uint16_t)ResScale(255); }

constexpr auto ObjectCloaked(const auto& eptr) { return (GetPrimType(&DisplayArray[(eptr)->PrimIndex]) >= PrimitiveType::Count || (GetPrimType(&DisplayArray[(eptr)->PrimIndex]) == PrimitiveType::StampFill && sameColor(GetPrimColor(&DisplayArray[(eptr)->PrimIndex]), BlackColor))); }


extern ElementHandle AllocElement(void);
extern void FreeElement(ElementHandle hElement);
constexpr auto PutElement(const auto& h) { return PutQueue(&disp_q, h); }
constexpr auto InsertElement(const auto& h, const auto& i) { return InsertQueue(&disp_q, h, i); }
constexpr auto GetHeadElement() { return GetHeadLink(&disp_q); }
constexpr auto GetTailElement() { return GetTailLink(&disp_q); }
inline void LockElement(ElementHandle h, Element** ppe) { *ppe = reinterpret_cast<Element*>(LockLink(&disp_q, h)); }
constexpr auto UnlockElement(const auto& h) { return UnlockLink(&disp_q, h); }
constexpr auto GetPredElement(const auto& l) { return _GetPredLink(l); }
constexpr auto GetSuccElement(const auto& l) { return _GetSuccLink(l); }
extern void RemoveElement(LinkHandle hLink);

// XXX: The following functions should not really be here
extern void spawn_planet(void);
extern void spawn_asteroid(Element* ElementPtr);
extern void do_damage(Element* ElementPtr, int16_t damage);
extern void crew_preprocess(Element* ElementPtr);
extern void crew_collision(Element* ElementPtr0, GfxPoint* pPt0,
						   Element* ElementPtr1, GfxPoint* pPt1);
extern void AbandonShip(Element* ShipPtr, Element* TargetPtr,
						uint16_t crew_loss);
extern bool TimeSpaceMatterConflict(Element* ElementPtr);
extern uint16_t PlotIntercept(Element* ElementPtr0,
							  Element* ElementPtr1, uint16_t max_turns, uint16_t margin_of_error);

extern void InitGalaxy(void);
extern void SetStarPoint(GfxPoint pt, uint16_t i);
extern void MoveGalaxy(ViewState view_state, int32_t dx, int32_t dy);

extern bool CalculateGravity(Element* ElementPtr);

} // namespace uqm

#endif // UQM_ELEMENT_H
