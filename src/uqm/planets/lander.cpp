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

#include "lander.h"

#include "lifeform.h"
#include "scan.h"
#include "../cons_res.h"
#include "../controls.h"
#include "../colors.h"
#include "../process.h"
#include "../units.h"
#include "../gamestr.h"
#include "../nameref.h"
#include "../resinst.h"
#include "../setup.h"
#include "../setupmenu.h"
#include "../sounds.h"
#include "../element.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/mathlib.h"
#include "core/log/log.h"
#include "options.h"
#include "uqm/menustat.h"
#include "../util.h"
#include "uqm/random/UQMRandom.h"

//define SPIN_ON_LAUNCH to let the planet spin while
// the lander animation is playing
#define SPIN_ON_LAUNCH

// PlanetSideRate governs how fast the lander,
// bio and planet effects will be
// We're using the 3DO speed, which is 35 FPS
// The PC speed was 30 FPS.
// Remember that all values need to evenly divide
// GameTicksPerSecond.
// I did my own testing using DOSBox
// comparing speeds frame-by-frame and I came
// up with 42 FPS. Can be changed later if it
// causes any trouble.
static constexpr auto PlanetSideRate {(c_gameSecond / 42)};


// This is a derived type from INPUT_STATE_DESC.
typedef struct LanderInputState LanderInputState;
struct LanderInputState
{
	// Fields required by DoInput()
	bool (*InputFunc)(LanderInputState* pMS);

	bool Initialized;
	TimeCount NextTime;
	// Frame rate control
};

FRAME LanderFrame[8];
uint8_t LanderUpgradesFlag;
static SOUND_REF LanderSounds;
MUSIC_REF LanderMusic;
static GFXCONTEXT PCLanderContext;
static MUSIC_REF OrbitMusic[NumOrbitThemes];

LIFEFORM_DESC CreatureData[] =
	{
		{SpeedMotionless | DangerHarmless,							   uqm::makeByte(1, 1) },
		// Roto-Dendron
		{SpeedMotionless | DangerHarmless,							   uqm::makeByte(6, 1) },
		// Macrocillia
		{SpeedMotionless | DangerWeak,								   uqm::makeByte(3, 1) },
		// Splort Wort
		{SpeedMotionless | DangerNormal,								 uqm::makeByte(5, 3) },
		// Whackin' Bush
		{SpeedMotionless | DangerHarmless,							   uqm::makeByte(2, 10)},
		// Slot Machine Tree
		{BehaviorUnpredictable | SpeedSlow | DangerHarmless,
		 uqm::makeByte(1,																2) },
		// Neon Worm
		{BehaviorFlee | AwarenessMedium | SpeedSlow | DangerHarmless,
		 uqm::makeByte(8,																5) },
		// Stiletto Urchin
		{BehaviorHunt | AwarenessLow | SpeedSlow | DangerWeak,
		 uqm::makeByte(2,																2) },
		// Deluxe Blob
		{BehaviorUnpredictable | SpeedSlow | DangerNormal,
		 uqm::makeByte(3,																8) },
		// Glowing Medusa
		{BehaviorHunt | AwarenessMedium | SpeedSlow | DangerMonstrous,
		 uqm::makeByte(10,															   15)},
		// Carousel Beast
		{BehaviorHunt | AwarenessMedium | SpeedMedium | DangerWeak,
		 uqm::makeByte(3,																3) },
		// Mysterious Bees
		{BehaviorFlee | AwarenessMedium | SpeedMedium | DangerHarmless,
		 uqm::makeByte(2,																1) },
		// Hopping Blobby
		{BehaviorUnpredictable | SpeedMedium | DangerWeak,
		 uqm::makeByte(2,																2) },
		// Blood Monkey
		{BehaviorHunt | AwarenessHigh | SpeedMedium | DangerNormal,
		 uqm::makeByte(4,																6) },
		// Yompin Yiminy
		{BehaviorUnpredictable | SpeedMedium | DangerMonstrous,
		 uqm::makeByte(9,																12)},
		// Amorphous Trandicula
		{BehaviorHunt | AwarenessHigh | SpeedFast | DangerWeak,
		 uqm::makeByte(3,																1) },
		// Crazy Weasel
		{BehaviorFlee | AwarenessHigh | SpeedFast | DangerHarmless,
		 uqm::makeByte(1,																1) },
		// Merry Whumpet
		{BehaviorHunt | AwarenessLow | SpeedFast | DangerNormal,
		 uqm::makeByte(7,																8) },
		// Fungal Squid
		{BehaviorFlee | AwarenessHigh | SpeedFast | DangerWeak,
		 uqm::makeByte(15,															   2) },
		// Penguin Cyclops
		{BehaviorFlee | AwarenessLow | SpeedFast | DangerWeak,
		 uqm::makeByte(1,																1) },
		// Chicken
		{BehaviorUnpredictable | SpeedSlow | DangerWeak,				 uqm::makeByte(6, 2) },
		// Bubble Vine
		{BehaviorFlee | AwarenessHigh | SpeedSlow | DangerWeak,
		 uqm::makeByte(4,																2) },
		// Bug-Eyed Bait
		{SpeedMotionless | DangerWeak,								   uqm::makeByte(8, 5) },
		// Goo Burger

		{SpeedMotionless | DangerMonstrous,								uqm::makeByte(1, 1) },
		// Evil One
		{BehaviorUnpredictable | SpeedSlow | DangerHarmless,
		 uqm::makeByte(1,																1) }, // ? was 0, 1
							   // Brainbox Bulldozers
		{BehaviorHunt | AwarenessHigh | SpeedFast | DangerMonstrous,
		 uqm::makeByte(15,															   15)},
		// Zex's Beauty
};

extern PRIM_LINKS DisplayLinks;

static constexpr auto DamageCycle {6};
// XXX: There are actually only 9 explosion images.
//   The last frame is drawn twice.
static constexpr auto ExplosionLife {10};
// How long to wait after the lander explodes, so that the full
// gravity of the player's situation sinks in
static constexpr auto ExplosionWait {(GameTicksPerSecond * 2)};
#define EXPLOSION_WAIT_FRAMES (TimePeriod {ExplosionWait} / PlanetSideRate)
// The actual number of frame that the explosion and wait takes is:
//   ExplosionLife * 3 + EXPLOSION_WAIT_FRAMES

static constexpr auto DeathExplosion {0};

// TODO: redefine these in terms of GFXCONTEXT width/height
static auto SurfaceWidth() { return SisScreenWidth(); }
static auto SurfaceHeight() { return (SisScreenHeight() - MapHeight() - MapBorderHeight()); }


static constexpr auto KillCrew {(1 << 5)};
static constexpr auto AddAtEnd {(1 << 4)};


static auto LanderSpeedDenom() { return (isPC(uqm::UQMOptions::read().landerStyle) ? 14 : 10); }

static uint8_t lander_flags;
static GFXPOINT curLanderLoc;
static int crew_left;
static int shieldHit;
// which shield was hit, assuming it helped
static int damage_index;
// number of lander damage frames left
static int explosion_index;
// lander explosion progression. Semantics are similar to an
// inverse of ELEMENT.life_span
static int turn_wait;
// thus named for similar semantics to ELEMENT.turn_wait
static int weapon_wait;
// semantics similar to STARSHIP.weapon_counter

// TODO: We may want to make the PLANETSIDE_DESC fields into static vars
static PLANETSIDE_DESC* planetSideDesc;

EXTENT MapSurface;

static constexpr auto OnTheGround {0};

GFXCONTEXT
CreatePCLanderContext(void)
{
	GFXRECT r;

	// PCLanderContext rect is relative to SpaceContext
	GFXCONTEXT oldContext {SetContext(SpaceContext)};
	GetContextClipRect(&r);

	GFXCONTEXT context {CreateContext("PCLanderContext")};
	SetContext(context);
	SetContextFGFrame(Screen);
	r.corner.x += (r.extent.width - MapWidth())
				+ (SafeX() * 2 + SafeNumScl(1));
	r.corner.y += r.extent.height - MapHeight();
	r.extent.width = ResScale(UqmMapWidth - Sc2MapWidth) - SisOrgX()
				   + SafePos(1);
	r.extent.height = MapHeight();
	SetContextClipRect(&r);

	SetContext(oldContext);

	return context;
}

GFXCONTEXT
GetPCLanderContext(bool* owner)
{
	// TODO: Make GFXCONTEXT ref-counted
	if (PCLanderContext)
	{
		if (owner)
		{
			*owner = false;
		}
	}
	else
	{
		if (owner)
		{
			*owner = true;
		}
		PCLanderContext = CreatePCLanderContext();
	}
	return PCLanderContext;
}

void DestroyPCLanderContext(void)
{
	if (PCLanderContext)
	{
		DestroyContext(PCLanderContext);
		PCLanderContext = nullptr;
	}
}


static Color
DamageColorCycle(Color c, uint16_t i)
{
	const static Color damage_tab[DamageCycle + 1] =
		LANDER_DAMAGE_COLOR_TABLE;

	if (i)
	{
		c = damage_tab[i];
	}
	else if (sameColor(c, damage_tab[0]))
	{
		c = damage_tab[6];
	}
	else if (sameColor(c, damage_tab[6]))
	{
		c = damage_tab[5];
	}
	else if (sameColor(c, damage_tab[5]))
	{
		c = damage_tab[4];
	}
	else if (sameColor(c, damage_tab[4]))
	{
		c = damage_tab[3];
	}
	else if (sameColor(c, damage_tab[3]))
	{
		c = damage_tab[2];
	}
	else if (sameColor(c, damage_tab[2]))
	{
		c = damage_tab[1];
	}
	else
	{
		c = damage_tab[0];
	}

	return c;
}

static HELEMENT AddGroundDisaster(uint16_t which_disaster);

void object_animation(ELEMENT* ElementPtr)
{
	uint16_t frame_index {};
	uint16_t angle {};

	PRIMITIVE* pPrim {&DisplayArray[ElementPtr->PrimIndex]};
	if (GetPrimType(pPrim) == STAMPFILL_PRIM
		&& !((ElementPtr->state_flags & FiniteLife)
			 && ElementPtr->mass_points == EARTHQUAKE_DISASTER))
	{
		Color c;

		c = DamageColorCycle(GetPrimColor(pPrim), 0);
		if (sameColor(c, WhiteColor))
		{
			SetPrimType(pPrim, STAMP_PRIM);
			if (ElementPtr->hit_points == 0)
			{
				ZeroVelocityComponents(&ElementPtr->velocity);
				pPrim->Object.Stamp.frame =
					SetAbsFrameIndex(pPrim->Object.Stamp.frame, 0);

				PlaySound(
					GetSound(LanderSounds, LIFEFORM_CANNED),
					NotPositional(), nullptr, GameSoundPriority);
			}
		}

		SetPrimColor(pPrim, c);
	}

	frame_index = GetFrameIndex(pPrim->Object.Stamp.frame) + 1;
	if (uqm::lowNibble(ElementPtr->turn_wait))
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		ElementPtr->turn_wait += uqm::highNibble(ElementPtr->turn_wait);

		pPrim->Object.Stamp.frame =
			IncFrameIndex(pPrim->Object.Stamp.frame);

		if (ElementPtr->state_flags & FiniteLife)
		{
			/* A natural disaster */
			if (ElementPtr->mass_points == DeathExplosion)
			{ // Lander explosion
				++explosion_index;
				if (explosion_index >= ExplosionLife)
				{ // XXX: The last frame is drawn twice
					pPrim->Object.Stamp.frame =
						DecFrameIndex(pPrim->Object.Stamp.frame);
				}
			}
			else if (ElementPtr->mass_points == EARTHQUAKE_DISASTER)
			{
				//int16_t s; unused
				const int16_t frame_amount = 13;


				// XXX: Was 0x8000 the background flag on 3DO?
				/*if (frame_index >= (frame_amount)) unused
					s = 0;
				else
					s = ((frame_amount + 1)- frame_index) >> 1;

				SetPrimColor (pPrim,
						BuildColor (
							0x8000 | MakeRgb15 (0x1F, 0x1F, 0x1F), s));*/

				SetPrimColor(pPrim, WhiteColor);
				if (frame_index == (frame_amount))
				{
					PlaySound(
						GetSound(LanderSounds,
								 EARTHQUAKE_DISASTER),
						NotPositional(), nullptr, GameSoundPriority);
				}
			}

			if (ElementPtr->mass_points == LAVASPOT_DISASTER
				&& frame_index == 5
				&& uqm::random().next() % 100 < 90)
			{
				HELEMENT hLavaElement {};

				/* Change lava-spot direction of travel */
				hLavaElement = AddGroundDisaster(LAVASPOT_DISASTER);
				if (hLavaElement)
				{
					ELEMENT* LavaElementPtr {};

					angle = FacingToAngle(ElementPtr->facing);
					LockElement(hLavaElement, &LavaElementPtr);
					LavaElementPtr->next.location =
						ElementPtr->next.location;
					LavaElementPtr->next.location.x +=
						Cosine(angle, ResScale(4));
					LavaElementPtr->next.location.y +=
						Sine(angle, ResScale(4));
					if (LavaElementPtr->next.location.y < 0)
					{
						LavaElementPtr->next.location.y = 0;
					}
					else if (LavaElementPtr->next.location.y >= (MapHeight() << MagShift))
					{
						LavaElementPtr->next.location.y =
							(MapHeight() << MagShift) - 1;
					}
					if (LavaElementPtr->next.location.x < 0)
					{
						LavaElementPtr->next.location.x +=
							ScaledMapWidth() << MagShift;
					}
					else
					{
						LavaElementPtr->next.location.x %=
							ScaledMapWidth() << MagShift;
					}
					LavaElementPtr->facing = NormalizeFacing(
						ElementPtr->facing + (uqm::random().next() % 3 - 1));
					UnlockElement(hLavaElement);
				}
			}
		}
		else if (!(frame_index & 3) && ElementPtr->hit_points)
		{
			uint8_t index {};
			uint16_t speed {};

			index = ElementPtr->mass_points & ~CreatureAware;
			speed = CreatureData[index].Attributes & SpeedMask;
			if (speed)
			{
				int16_t dx {};
				int16_t dy {};

				dx = curLanderLoc.x - ElementPtr->next.location.x;
				if (dx < 0 && dx < -(ScaledMapWidth() << (MagShift - 1)))
				{
					dx += ScaledMapWidth() << MagShift;
				}
				else if (dx > (ScaledMapWidth() << (MagShift - 1)))
				{
					dx -= ScaledMapWidth() << MagShift;
				}
				dy = curLanderLoc.y - ElementPtr->next.location.y;
				angle = ARCTAN(dx, dy);
				if (dx < 0)
				{
					dx = -dx;
				}
				if (dy < 0)
				{
					dy = -dy;
				}

				if (dx >= MapSurface.width || dy >= MapSurface.width
					|| dx * dx + dy * dy
						   >= MapSurface.width * MapSurface.width)
				{
					ElementPtr->mass_points &= ~CreatureAware;
				}
				else if (!(ElementPtr->mass_points & CreatureAware))
				{
					uint8_t DetectPercent {};

					DetectPercent = (((uint8_t)(CreatureData[index].Attributes
												& AwarenessMask)
									  >> AwarenessShift)
									 + 1)
								  * (30 / 6);
					// XXX: Shouldn't this be dependent on
					// PlanetSideRate somehow? And why is it
					// written as '30 / 6' instead of 5? Does the
					// 30 specify the (PC) framerate? That doesn't
					// make sense; I would expect it to be in the
					// denominator. And even then, it wouldn't give
					// the same results with different frame rates,
					// as repeating 'random(x / 30)' 30 times
					// doesn't generally have the same result as
					// repeating 'random(x / 35)' 25 times. - SvdB
					if (uqm::random().next() % 100 < DetectPercent)
					{
						ElementPtr->thrust_wait = 0;
						ElementPtr->mass_points |= CreatureAware;
					}
				}

				if (ElementPtr->next.location.y == 0
					|| ElementPtr->next.location.y == (MapHeight() << MagShift) - 1)
				{
					ElementPtr->thrust_wait = 0;
				}

				const uint16_t old_angle {GetVelocityTravelAngle(&ElementPtr->velocity)};
				if (ElementPtr->thrust_wait)
				{
					--ElementPtr->thrust_wait;
					angle = old_angle;
				}
				else if (!(ElementPtr->mass_points & CreatureAware)
						 || (CreatureData[index].Attributes
							 & BehaviorMask)
								== BehaviorUnpredictable)
				{
					uint16_t rand_val {};

					rand_val = uqm::random().next();
					angle = NormalizeAngle(uqm::lowByte(rand_val));
					ElementPtr->thrust_wait =
						(uqm::highByte(rand_val) >> 2) + 10;
				}
				else if ((CreatureData[index].Attributes
						  & BehaviorMask)
						 == BehaviorFlee)
				{
					if (ElementPtr->next.location.y == 0
						|| ElementPtr->next.location.y == (MapHeight() << MagShift) - 1)
					{
						if (angle & (HalfCircle - 1))
						{
							angle = HalfCircle - angle;
						}
						else if (old_angle == Quadrant
								 || old_angle == (FullCircle - Quadrant))
						{
							angle = old_angle;
						}
						else
						{
							angle = ((uqm::random().next() & 1)
									 * HalfCircle)
								  - Quadrant;
						}
						ElementPtr->thrust_wait = 5;
					}
					angle = NormalizeAngle(angle + HalfCircle);
				}

				switch (speed)
				{
					case SpeedSlow:
						speed = WorldToVelocity(2 * 1) >> 2;
						break;
					case SpeedMedium:
						speed = WorldToVelocity(2 * 1) >> 1;
						break;
					case SpeedFast:
						speed = WorldToVelocity(2 * 1) * 9 / 10;
						break;
				}
				speed = ResScale(speed);

				SetVelocityComponents(&ElementPtr->velocity,
									  Cosine(angle, speed), Sine(angle, speed));
			}
		}
	}

	if ((ElementPtr->state_flags & FiniteLife)
		&& ElementPtr->mass_points == DeathExplosion
		&& GetSuccLink(DisplayLinks) != ElementPtr->PrimIndex)
	{
		lander_flags |= AddAtEnd;
	}
}

static auto NumCrewCols() { return (is3DO(uqm::UQMOptions::read().landerStyle) ? 6 : 3); }
static auto NumCrewRows() { return (is3DO(uqm::UQMOptions::read().landerStyle) ? 2 : 4); }

static void
DeltaLanderCrew(int16_t crew_delta, uint16_t which_disaster)
{
	STAMP s;
	GFXCONTEXT OldContext {};

	if (crew_delta > 0)
	{
		// Filling up the crew bar when landing.
		crew_delta = crew_left;
		crew_left += 1;

		s.frame = SetAbsFrameIndex(LanderFrame[0], 55);
	}
	else /* if (crew_delta < 0) */
	{
		if (crew_left < 1)
		{
			return; // irrelevant -- all dead
		}

		shieldHit = uqm::gameState().flag(uqm::GameFlag::LANDER_SHIELDS);
		shieldHit &= 1 << which_disaster;
		if (!shieldHit || uqm::random().next() % 100 >= 95)
		{ // No shield, or it did not help
			if (!testFlag(uqm::UQMOptions::read().optGodModes, uqm::GodModeFlags::NoDamage))
			{
				shieldHit = 0;
				--crew_left;
			}
			else
			{
				shieldHit = 1;
			}
		}

		damage_index = DamageCycle;
		if (shieldHit)
		{
			return;
		}

		crew_delta = crew_left;
		s.frame = SetAbsFrameIndex(LanderFrame[0], 56);

		PlaySound(GetSound(LanderSounds, LANDER_INJURED),
				  NotPositional(), nullptr, GameSoundPriority);
	}

	if (is3DO(uqm::UQMOptions::read().landerStyle))
	{
		s.origin.x = ResScale(11) + (ResScale(6) * (crew_delta % NumCrewCols()));
		s.origin.y = ResScale(35) - (ResScale(6) * (crew_delta / NumCrewCols()));
		OldContext = SetContext(RadarContext);
	}
	else
	{
		s.origin.x = ResScale(6) + (ResScale(6) * (crew_delta % NumCrewCols()));
		s.origin.y = ResScale(39) - (ResScale(6) * (crew_delta / NumCrewCols()));
		OldContext = SetContext(PCLanderContext);
	}

	DrawStamp(&s);
	SetContext(OldContext);
}

static void
FillLanderHold(PLANETSIDE_DESC* pPSD, uint16_t scan, uint16_t NumRetrieved)
{
	uint16_t start_count {};
	uint16_t tmpholdint {};
	STAMP s;
	GFXCONTEXT OldContext {};

	PlaySound(GetSound(LanderSounds, LANDER_PICKUP),
			  NotPositional(), nullptr, GameSoundPriority);

	if (scan == BIOLOGICAL_SCAN)
	{
		start_count = pPSD->BiologicalLevel;

		s.frame = SetAbsFrameIndex(
			(isPC(uqm::UQMOptions::read().landerStyle) ? LanderFrame[7] : LanderFrame[0]), 41);

		pPSD->BiologicalLevel += NumRetrieved;
	}
	else
	{
		start_count = pPSD->ElementLevel;
		pPSD->ElementLevel += NumRetrieved;
		if (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_CARGO))
		{
			start_count >>= 1;
			NumRetrieved = (pPSD->ElementLevel >> 1) - start_count;
		}

		s.frame = SetAbsFrameIndex(
			(isPC(uqm::UQMOptions::read().landerStyle) ? LanderFrame[7] : LanderFrame[0]), 43);
	}

	tmpholdint =
		((start_count + NumRetrieved) * MaxHoldBars() / MaxScrounged())
		- ((start_count * MaxHoldBars() / MaxScrounged())
		   + (NumRetrieved * MaxHoldBars() / MaxScrounged()));
	start_count = start_count * MaxHoldBars() / MaxScrounged();
	NumRetrieved = (NumRetrieved * MaxHoldBars() / MaxScrounged())
				 + tmpholdint;

	s.origin.x = 0;
	s.origin.y = -(int)ResScale(start_count);
	if (!(start_count & 1))
	{
		s.frame = IncFrameIndex(s.frame);
	}

	if (isPC(uqm::UQMOptions::read().landerStyle))
	{
		OldContext = SetContext(PCLanderContext);
	}
	else
	{
		OldContext = SetContext(RadarContext);
	}

	while (NumRetrieved--)
	{
		if (start_count++ & 1)
		{
			s.frame = IncFrameIndex(s.frame);
		}
		else
		{
			s.frame = DecFrameIndex(s.frame);
		}
		DrawStamp(&s);
		s.origin.y -= ResScale(1);
	}
	SetContext(OldContext);
}

// returns true iff the node was picked up.
static bool
pickupNode(PLANETSIDE_DESC* pPSD, uint16_t NumRetrieved,
		   ELEMENT* ElementPtr, const INTERSECT_CONTROL* LanderControl,
		   const INTERSECT_CONTROL* ElementControl, uint16_t Scan)
{
	char ch {};
	uint16_t Max {};

	uint16_t* Amount {&pPSD->BiologicalLevel};
	Max = MaxScrounged();
	uint8_t EType {ElementPtr->thrust_wait};
	uint16_t Offset {BIOLOGICAL_STRING_BASE};

	if (Scan != BIOLOGICAL_SCAN)
	{
		Amount = &pPSD->ElementLevel;
		Max = pPSD->MaxElementLevel;
		EType = ElementPtr->turn_wait;
		Offset = ELEMENTS_STRING_BASE;
	}

	// JMS: The rest of partially scavenged minerals stay on the surface.
	bool PartialPickup {false};

	if (*Amount >= Max)
	{
		// Lander full
		PlaySound(GetSound(LanderSounds, LANDER_FULL),
				  NotPositional(), nullptr, GameSoundPriority);
		return false;
	}

	if (*Amount + NumRetrieved > Max)
	{
		int16_t which_node {};
		const uint16_t oldsize = ElementPtr->mass_points;

		// Deposit could only be picked up partially.
		NumRetrieved = (uint16_t)(Max - *Amount);

		if (Scan != BIOLOGICAL_SCAN && uqm::UQMOptions::read().partialPickup)
		{ // JMS: Subtract the scavenged kilotons from the mineral
			// deposit. The rest will stay on the surface.
			ElementPtr->mass_points -= NumRetrieved;

			// JMS: This makes the mineral deposit subtraction keep
			// in effect even after leaving & re-entering the planet.
			which_node = uqm::highByte(ElementPtr->scan_node) - 1;
			pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[MINERAL_SCAN][which_node] =
				NumRetrieved;

			// JMS: If the deposit was large and its amount now equates to
			// a smaller deposit, change its graphics.
			if ((oldsize > 22 && ElementPtr->mass_points <= 22)
				|| (oldsize > 15 && ElementPtr->mass_points <= 15))
			{
				PRIMITIVE* pPrim = &DisplayArray[ElementPtr->PrimIndex];
				uint8_t gfx_index_change = 0;

				if (oldsize > 22 && ElementPtr->mass_points <= 15)
				{
					gfx_index_change = 2;
				}
				else
				{
					gfx_index_change = 1;
				}

				// Change the scan screen gfx.
				ElementPtr->current.image.frame = SetRelFrameIndex(
					ElementPtr->current.image.frame,
					(2 - gfx_index_change));
				ElementPtr->next.image.frame =
					ElementPtr->current.image.frame;

				// Notify the engine that the scan screen gfx should be
				// updated.
				ElementPtr->state_flags |= Changing;
				uqm::gameState().setFlag(uqm::GameFlag::PLANETARY_CHANGE, static_cast<uint32_t>(1));

				// Change the surface screen gfx.
				pPrim->Object.Stamp.frame = SetRelFrameIndex(
					pPrim->Object.Stamp.frame, -gfx_index_change);
			}

			PartialPickup = true;
		}

		if (Scan == BIOLOGICAL_SCAN && uqm::UQMOptions::read().partialPickup)
		{
			// Lander full
			PlaySound(GetSound(LanderSounds, LANDER_FULL),
					  NotPositional(), nullptr, GameSoundPriority);
			return false;
		}
	}

	FillLanderHold(pPSD, Scan, NumRetrieved);

	if (Scan != BIOLOGICAL_SCAN)
	{
		const uint8_t NumNodesGrabbed = pPSD->NodeData.NumNodesGrabbed;

		pPSD->ElementAmounts[ElementCategory(EType)] += NumRetrieved;

		pPSD->NodeData.NodeType[NumNodesGrabbed] = EType;
		pPSD->NodeData.NodeAmounts[NumNodesGrabbed] = NumRetrieved;
		pPSD->NodeData.NumNodesGrabbed++;
	}

	pPSD->NumFrames = NumTextFrames;
	const auto fmtResult = fmt::format_to_n(pPSD->AmountBuf, sizeof(pPSD->AmountBuf) - 1, "{}", NumRetrieved);
	*fmtResult.out = '\0';
	const char* pStr {GameString(EType + Offset)};

	pPSD->MineralText[0].baseline.x = (MapSurface.width >> 1)
									+ (ElementControl->EndPoint.x - LanderControl->EndPoint.x);
	pPSD->MineralText[0].baseline.y = (MapSurface.height >> 1)
									+ (ElementControl->EndPoint.y - LanderControl->EndPoint.y);
	pPSD->MineralText[0].CharCount = (uint16_t)~0;
	pPSD->MineralText[1].pStr = pStr;

	while ((ch = *pStr++) && ch != ' ')
		;
	if (ch == '\0')
	{
		pPSD->MineralText[1].CharCount = (uint16_t)~0;
		pPSD->MineralText[2].CharCount = 0;
	}
	else /* ch == ' ' */
	{
		// Name contains a space. Print over
		// two lines.
		pPSD->MineralText[1].CharCount = (uint16_t)uqm::utf8StringCount(
			uqstl::string_view {pPSD->MineralText[1].pStr, static_cast<size_t>(pStr - 1 - pPSD->MineralText[1].pStr)});
		pPSD->MineralText[2].pStr = pStr;
		pPSD->MineralText[2].CharCount = (uint16_t)~0;
	}

	// JMS
	return (PartialPickup ? false : true);
}

static void
ExplodeCritter(ELEMENT* ElementPtr)
{
	int16_t temp_which_node {};

	HELEMENT hExplosionElement {AllocElement()};
	if (hExplosionElement)
	{
		ELEMENT* ExplosionElementPtr {};
		LockElement(hExplosionElement, &ExplosionElementPtr);

		ExplosionElementPtr->mass_points = DeathExplosion;
		ExplosionElementPtr->state_flags = FiniteLife;
		ExplosionElementPtr->playerNr = PS_NON_PLAYER;
		ExplosionElementPtr->next.location = ElementPtr->next.location;
		ExplosionElementPtr->preprocess_func = object_animation;
		ExplosionElementPtr->turn_wait = uqm::makeByte(2, 2);
		ExplosionElementPtr->life_span = ExplosionLife
									   * (uqm::lowNibble(ExplosionElementPtr->turn_wait));

		SetPrimType(&DisplayArray[ExplosionElementPtr->PrimIndex],
					STAMP_PRIM);
		DisplayArray[ExplosionElementPtr->PrimIndex].Object.Stamp.frame =
			SetAbsFrameIndex(LanderFrame[0], 46);

		// JMS: This keeps track of the explosion frames. Normally
		// explosion occurs only once (lander explodes). If we don't zero
		// this variable here, the explosion anim can run only once
		// properly and would get stuck in the last frame after that on all
		// the subsequent explosions.
		explosion_index = 0;

		UnlockElement(hExplosionElement);
		InsertElement(hExplosionElement, GetHeadElement());

		PlaySound(GetSound(LanderSounds, LANDER_DESTROYED),
				  NotPositional(), nullptr, GameSoundPriority + 1);

		ElementPtr->state_flags |= Disappearing;
		// JMS: Delete the critter frame
		ElementPtr->mass_points = 0;
		// JMS: Make sure critter/explosion doesn't give biodata.

		// JMS: This marks the exploded critter "collected". (even though
		// there was no biodata to collect). This ensures the critter isn't
		// resurrected when visiting the planet next time.
		temp_which_node = uqm::highByte(ElementPtr->scan_node) - 1;
		pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[BIOLOGICAL_SCAN] |=
			(1L << temp_which_node);
		// Mark this bio blip's state as "collected".
		//
		//pSolarSysState->CurNode = (uint16_t)~0;
		// GenerateLifeForms will update the states of ALL
		// bio-blips when run.
		//callGenerateForScanType (pSolarSysState,
		//		pSolarSysState->pOrbitalDesc, &pSolarSysState->CurNode,
		//		BIOLOGICAL_SCAN);
		// Re-run GenerateLifeForms so the changed state takes
		// effect
		uqm::gameState().setFlag(uqm::GameFlag::PLANETARY_CHANGE, static_cast<uint32_t>(1));
		// Save the changes to the file containing the states of
		// all lifeforms.
	}
}

static void
shotCreature(ELEMENT* ElementPtr, uint8_t value,
			 INTERSECT_CONTROL* LanderControl, PRIMITIVE* pPrim)
{
	if (ElementPtr->hit_points == 0)
	{
		// Creature is already canned.
		return;
	}

	--ElementPtr->hit_points;
	if (ElementPtr->hit_points == 0)
	{
		// Brainbox bulldozers (Tractors at moon) explode.
		if ((ElementPtr->mass_points & ~CreatureAware) == 24)
		{
			ExplodeCritter(ElementPtr);
		}
		else
		{ // Can other creatures.
			// Stash the type of creature in the
			// thrust_wait field.  It seems to be unused
			// by the game for anything at this point
			ElementPtr->thrust_wait =
				ElementPtr->mass_points & ~CreatureAware;
			ElementPtr->mass_points = value;
			DisplayArray[ElementPtr->PrimIndex].Object.Stamp.frame =
				pSolarSysState->PlanetSideFrame[0];
		}
	}
	else if (CreatureData[ElementPtr->mass_points & ~CreatureAware]
				 .Attributes
			 & SpeedMask)
	{
		uint16_t angle {};

		angle = FacingToAngle(GetFrameIndex(
								  LanderControl->IntersectStamp.frame)
							  - AngleToFacing(FullCircle));
		DeltaVelocityComponents(&ElementPtr->velocity,
								Cosine(angle, WorldToVelocity(ResScale(1))),
								Sine(angle, WorldToVelocity(ResScale(1))));
		ElementPtr->thrust_wait = 0;
		ElementPtr->mass_points |= CreatureAware;
	}

	SetPrimType(pPrim, STAMPFILL_PRIM);
	SetPrimColor(pPrim, WhiteColor);

	PlaySound(GetSound(LanderSounds, LANDER_HITS),
			  NotPositional(), nullptr, GameSoundPriority);
}

static void
DrawRadarArea(void)
{
	DrawRadarBorder();
	DrawMineralHelpers();
}

static void
CheckObjectCollision(uint16_t index)
{
	INTERSECT_CONTROL LanderControl;
	DRAWABLE LanderHandle {};
	PRIMITIVE* pPrim {};
	const PRIMITIVE* pLanderPrim {};
	PLANETSIDE_DESC* pPSD = planetSideDesc;

	if (index != EndOfList)
	{
		pLanderPrim = &DisplayArray[index];
		LanderControl.IntersectStamp = pLanderPrim->Object.Stamp;
		index = GetPredLink(GetPrimLinks(pLanderPrim));
	}
	else
	{
		pLanderPrim = 0;
		LanderControl.IntersectStamp.origin.x = MapSurface.width >> 1;
		LanderControl.IntersectStamp.origin.y = MapSurface.height >> 1;
		LanderControl.IntersectStamp.frame = LanderFrame[0];
		index = GetSuccLink(DisplayLinks);
	}

	LanderControl.EndPoint = LanderControl.IntersectStamp.origin;
	LanderHandle =
		GetFrameParentDrawable(LanderControl.IntersectStamp.frame);

	for (; index != EndOfList; index = GetPredLink(GetPrimLinks(pPrim)))
	{
		INTERSECT_CONTROL ElementControl;
		HELEMENT hElement {};
		HELEMENT hNextElement {};

		pPrim = &DisplayArray[index];
		ElementControl.IntersectStamp = pPrim->Object.Stamp;
		ElementControl.EndPoint = ElementControl.IntersectStamp.origin;

		if (GetFrameParentDrawable(ElementControl.IntersectStamp.frame)
			== LanderHandle)
		{
			CheckObjectCollision(index);
			continue;
		}

		if (!DrawablesIntersect(&LanderControl,
								&ElementControl, MaxTimeValue))
		{
			continue;
		}

		for (hElement = GetHeadElement(); hElement;
			 hElement = hNextElement)
		{
			ELEMENT* ElementPtr {};

			LockElement(hElement, &ElementPtr);
			hNextElement = GetSuccElement(ElementPtr);

			if (&DisplayArray[ElementPtr->PrimIndex] == pLanderPrim)
			{
				ElementPtr->state_flags |= Disappearing;
				UnlockElement(hElement);
				continue;
			}

			if (&DisplayArray[ElementPtr->PrimIndex] != pPrim
				|| ElementPtr->playerNr != PS_NON_PLAYER)
			{
				UnlockElement(hElement);
				continue;
			}

			{
				uint16_t NumRetrieved {};
				int16_t which_node {};

				uint16_t scan {uqm::lowByte(ElementPtr->scan_node)};
				if (pLanderPrim == 0)
				{
					/* Collision of lander with another object */
					if (crew_left == 0 || pPSD->InTransit)
					{
						break;
					}

					if (ElementPtr->state_flags & FiniteLife)
					{
						/* A natural disaster */
						scan = ElementPtr->mass_points;
						switch (scan)
						{
							case EARTHQUAKE_DISASTER:
							case LAVASPOT_DISASTER:
								if (uqm::random().next() % 100 < 25)
								{
									DeltaLanderCrew(-1, scan);
								}
								break;
						}

						UnlockElement(hElement);
						continue;
					}
					else if (scan == ENERGY_SCAN)
					{
						// noop; handled by generation funcs, see below
						DrawRadarArea();
					}
					else if (scan == BIOLOGICAL_SCAN
							 && ElementPtr->hit_points)
					{
						const uint8_t danger_vals[] =
							{
								0, 6, 13, 26};
						const int creatureIndex = ElementPtr->mass_points
												& ~CreatureAware;
						const int dangerLevel =
							(CreatureData[creatureIndex].Attributes & DangerMask) >> DangerShift;

						if (uqm::random().next() % 128 < danger_vals[dangerLevel])
						{
							PlaySound(GetSound(
										  LanderSounds, BIOLOGICAL_DISASTER),
									  NotPositional(), nullptr,
									  GameSoundPriority);
							DeltaLanderCrew(-1, BIOLOGICAL_DISASTER);
						}
						UnlockElement(hElement);
						continue;
					}

					NumRetrieved = ElementPtr->mass_points;
				}
				else if (ElementPtr->state_flags & FiniteLife)
				{
					/* Collision of a stun bolt with a natural disaster */
					UnlockElement(hElement);
					continue;
				}
				else
				{
					uint8_t value {};

					if (scan == ENERGY_SCAN)
					{
						/* Collision of a stun bolt with an energy node */
						UnlockElement(hElement);
						break;
					}

					if (scan == BIOLOGICAL_SCAN
						&& (value = uqm::lowNibble(CreatureData[ElementPtr->mass_points
																& ~CreatureAware]
													   .ValueAndHitPoints)))
					{
						// Collision of a stun bolt with a viable creature
						shotCreature(ElementPtr, value, &LanderControl,
									 pPrim);
						UnlockElement(hElement);
						break;
					}

					NumRetrieved = 0;
				}

				if (NumRetrieved)
				{
					switch (scan)
					{
						case ENERGY_SCAN:
							break;
						case MINERAL_SCAN:
						case BIOLOGICAL_SCAN:
							if (!pickupNode(pPSD, NumRetrieved,
											ElementPtr, &LanderControl,
											&ElementControl, scan))
							{
								continue;
							}
							break;
					}
				}

				which_node = uqm::highByte(ElementPtr->scan_node) - 1;
				if (callPickupForScanType(pSolarSysState,
										  pSolarSysState->pOrbitalDesc, which_node, scan))
				{ // Node retrieved, remove from the surface
					setNodeRetrieved(&pSolarSysState->SysInfo.PlanetInfo,
									 scan, which_node);
					uqm::gameState().setFlag(uqm::GameFlag::PLANETARY_CHANGE, static_cast<uint32_t>(1));
					ElementPtr->state_flags |= Disappearing;
				}
				UnlockElement(hElement);
			}
		}
	}
}

static constexpr auto NumCycles {8};

static void
lightning_process(ELEMENT* ElementPtr)
{
	PRIMITIVE* pPrim {&DisplayArray[ElementPtr->PrimIndex]};
	if (uqm::lowNibble(ElementPtr->turn_wait))
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		uint16_t num_frames {};

		num_frames = GetFrameCount(pPrim->Object.Stamp.frame) - 7;
		if (GetFrameIndex(pPrim->Object.Stamp.frame) >= num_frames)
		{
			/* Advance to the next surface strike effect frame */
			// XXX: This is unused, we never get here
			pPrim->Object.Stamp.frame =
				IncFrameIndex(pPrim->Object.Stamp.frame);
		}
		else
		{
			int16_t s {};
			const static Color color_tab[NumCycles] =
				LIGHTNING_COLOR_TABLE;

			s = ElementPtr->life_span;
			if (s > NumCycles - 1)
			{
				s = NumCycles - 1;
			}
			SetPrimColor(pPrim, color_tab[s]);

			if (ElementPtr->mass_points == LIGHTNING_DISASTER)
			{
				/* This one always strikes the lander and can hurt */
				if (crew_left && uqm::random().next() % 100 < 10
					&& !planetSideDesc->InTransit)
				{
					lander_flags |= KillCrew;
				}

				ElementPtr->next.location = curLanderLoc;
			}

			pPrim->Object.Stamp.frame =
				SetAbsFrameIndex(pPrim->Object.Stamp.frame,
								 uqm::random().next() % num_frames);
		}

		ElementPtr->turn_wait += uqm::highNibble(ElementPtr->turn_wait);
	}

	if (GetSuccLink(DisplayLinks) != ElementPtr->PrimIndex)
	{
		lander_flags |= AddAtEnd;
	}
}

static void
AddLightning(void)
{
	HELEMENT hLightningElement {AllocElement()};
	if (hLightningElement)
	{
		uint32_t rand_val {};
		ELEMENT* LightningElementPtr {};

		LockElement(hLightningElement, &LightningElementPtr);

		LightningElementPtr->playerNr = PS_NON_PLAYER;
		LightningElementPtr->state_flags = FiniteLife;
		LightningElementPtr->preprocess_func = lightning_process;
		if (uqm::random().next() % 100 >= 25)
		{
			LightningElementPtr->mass_points = 0; /* harmless */
		}
		else
		{
			LightningElementPtr->mass_points = LIGHTNING_DISASTER;
		}

		rand_val = uqm::random().next();
		LightningElementPtr->life_span = 10 + (uqm::highWord(rand_val) % 10) + 1;
		LightningElementPtr->next.location.x = (curLanderLoc.x
												+ ((ScaledMapWidth() << MagShift)
												   - ((SurfaceWidth() >> 1) - 6))
												+ (chooseIfHd(static_cast<uint32_t>(uqm::lowByte(rand_val)), rand_val)
												   % (SurfaceWidth() - ResScale(12))))
											 % (ScaledMapWidth() << MagShift);
		LightningElementPtr->next.location.y = (curLanderLoc.y
												+ ((MapHeight() << MagShift) - ((SurfaceHeight() >> 1) - 6))
												+ (chooseIfHd(static_cast<uint32_t>(uqm::highByte(rand_val)), rand_val)
												   % (SurfaceHeight() - ResScale(12))))
											 % (MapHeight() << MagShift);

		LightningElementPtr->cycle = LightningElementPtr->life_span;

		if (!IsHd())
		{
			SetPrimType(&DisplayArray[LightningElementPtr->PrimIndex],
						STAMPFILL_PRIM);
			SetPrimColor(&DisplayArray[LightningElementPtr->PrimIndex],
						 WhiteColor);
		}
		else
		{
			SetPrimType(&DisplayArray[LightningElementPtr->PrimIndex],
						STAMP_PRIM);
		}

		DisplayArray[LightningElementPtr->PrimIndex].Object.Stamp.frame =
			LanderFrame[2];

		UnlockElement(hLightningElement);

		PutElement(hLightningElement);

		PlaySound(GetSound(LanderSounds, LIGHTNING_DISASTER),
				  NotPositional(), nullptr, GameSoundPriority);
	}
}

static HELEMENT
AddGroundDisaster(uint16_t which_disaster)
{
	HELEMENT hGroundDisasterElement {AllocElement()};
	if (hGroundDisasterElement)
	{
		ELEMENT* GroundDisasterElementPtr {};

		LockElement(hGroundDisasterElement, &GroundDisasterElementPtr);

		PRIMITIVE* pPrim {&DisplayArray[GroundDisasterElementPtr->PrimIndex]};
		GroundDisasterElementPtr->mass_points = which_disaster;
		GroundDisasterElementPtr->playerNr = PS_NON_PLAYER;
		GroundDisasterElementPtr->state_flags = FiniteLife;
		GroundDisasterElementPtr->preprocess_func = object_animation;

		const uint32_t rand_val {uqm::random().next()};
		GroundDisasterElementPtr->next.location.x = (curLanderLoc.x
													 + ((ScaledMapWidth() << MagShift)
														- (SurfaceWidth() * 3 / 8))
													 + (uqm::lowWord(rand_val) % (SurfaceWidth() * 3 / 4)))
												  % (ScaledMapWidth() << MagShift);
		GroundDisasterElementPtr->next.location.y = (curLanderLoc.y
													 + ((MapHeight() << MagShift) - (SurfaceHeight() * 3 / 8))
													 + (uqm::highWord(rand_val) % (SurfaceHeight() * 3 / 4)))
												  % (MapHeight() << MagShift);


		if (which_disaster == EARTHQUAKE_DISASTER)
		{
			if (!IsHd())
			{
				SetPrimType(pPrim, STAMPFILL_PRIM);
			}
			else
			{
				SetPrimType(pPrim, STAMP_PRIM);
			}

			pPrim->Object.Stamp.frame = LanderFrame[1];
			GroundDisasterElementPtr->turn_wait = uqm::makeByte(2, 2);
		}
		else /* if (which_disaster == LAVASPOT_DISASTER) */
		{
			SetPrimType(pPrim, STAMP_PRIM);
			GroundDisasterElementPtr->facing =
				NormalizeFacing(uqm::random().next());
			pPrim->Object.Stamp.frame = LanderFrame[3];
			GroundDisasterElementPtr->turn_wait = uqm::makeByte(0, 0);
		}
		GroundDisasterElementPtr->life_span =
			GetFrameCount(pPrim->Object.Stamp.frame)
				* (uqm::lowNibble(GroundDisasterElementPtr->turn_wait) + 1)
			- 1;

		UnlockElement(hGroundDisasterElement);

		PutElement(hGroundDisasterElement);
	}

	return (hGroundDisasterElement);
}

// This function replaces the ELEMENT manipulations typically done by
// PreProcess() and PostProcess() in process.c. Lander code does not
// call RedrawQueue() & Co and thus does not reap the benefits (or curses,
// depending how you look at it) of automatic flags processing.
static void
BuildObjectList(void)
{
	GFXPOINT org;
	HELEMENT hElement {};
	HELEMENT hNextElement {};
	const PLANETSIDE_DESC* pPSD = planetSideDesc;

	DisplayLinks = uqm::makeDword(EndOfList, EndOfList);

	lander_flags &= ~KillCrew;

	const uint32_t rand_val {uqm::random().next()};
	if (uqm::lowByte(uqm::highWord(rand_val)) < pPSD->FireChance)
	{
		AddGroundDisaster(LAVASPOT_DISASTER);
		PlaySound(GetSound(LanderSounds, LAVASPOT_DISASTER),
				  NotPositional(), nullptr, GameSoundPriority);
	}

	if (uqm::highByte(uqm::lowWord(rand_val)) < pPSD->TectonicsChance)
	{
		AddGroundDisaster(EARTHQUAKE_DISASTER);
	}

	if (uqm::lowByte(uqm::lowWord(rand_val)) < pPSD->WeatherChance)
	{
		AddLightning();
	}

	org = curLanderLoc;
	for (hElement = GetHeadElement();
		 hElement; hElement = hNextElement)
	{
		int16_t dx {};
		int16_t dy {};
		ELEMENT* ElementPtr {};

		LockElement(hElement, &ElementPtr);

		if (ElementPtr->life_span == 0
			|| (ElementPtr->state_flags & Disappearing))
		{
			hNextElement = GetSuccElement(ElementPtr);
			UnlockElement(hElement);
			RemoveElement(hElement);
			FreeElement(hElement);
			continue;
		}
		else if (ElementPtr->state_flags & FiniteLife)
		{
			--ElementPtr->life_span;
		}

		lander_flags &= ~AddAtEnd;

		if (ElementPtr->preprocess_func)
		{
			(*ElementPtr->preprocess_func)(ElementPtr);
		}

		GetNextVelocityComponents(&ElementPtr->velocity, &dx, &dy, 1);
		if (dx || dy)
		{
			ElementPtr->next.location.x += dx;
			ElementPtr->next.location.y += dy;
			/* if not lander's shot */
			if (ElementPtr->playerNr != PS_HUMAN_PLAYER)
			{
				if (ElementPtr->next.location.y < 0)
				{
					ElementPtr->next.location.y = 0;
				}
				else if (ElementPtr->next.location.y
						 >= (MapHeight() << MagShift))
				{
					ElementPtr->next.location.y =
						(MapHeight() << MagShift) - 1;
				}
			}
			if (ElementPtr->next.location.x < 0)
			{
				ElementPtr->next.location.x +=
					ScaledMapWidth() << MagShift;
			}
			else
			{
				ElementPtr->next.location.x %=
					ScaledMapWidth() << MagShift;
			}

			// XXX: Appearing flag is set by scan.c for scanned blips
			if (ElementPtr->state_flags & Appearing)
			{ // Update the location of a moving object on the scan map
				ElementPtr->current.location.x =
					ElementPtr->next.location.x >> MagShift;
				ElementPtr->current.location.y =
					ElementPtr->next.location.y >> MagShift;
			}
		}

		{
			PRIMITIVE* pPrim {&DisplayArray[ElementPtr->PrimIndex]};
			pPrim->Object.Stamp.origin.x =
				ElementPtr->next.location.x
				- org.x + (MapSurface.width >> 1);
			if (pPrim->Object.Stamp.origin.x >= (ScaledMapWidth() << MagShift)
													- (MapSurface.width * 3 / 2))
			{
				pPrim->Object.Stamp.origin.x -=
					ScaledMapWidth() << MagShift;
			}
			else if (pPrim->Object.Stamp.origin.x <= -((ScaledMapWidth() << MagShift)
													   - (MapSurface.width * 3 / 2)))
			{
				pPrim->Object.Stamp.origin.x +=
					ScaledMapWidth() << MagShift;
			}

			pPrim->Object.Stamp.origin.y =
				ElementPtr->next.location.y
				- org.y + (MapSurface.height >> 1);

			if (lander_flags & AddAtEnd)
			{
				InsertPrim(&DisplayLinks, ElementPtr->PrimIndex,
						   EndOfList);
			}
			else
			{
				InsertPrim(&DisplayLinks, ElementPtr->PrimIndex,
						   GetPredLink(DisplayLinks));
			}
		}

		hNextElement = GetSuccElement(ElementPtr);
		UnlockElement(hElement);
	}
}

static void
ScrollPlanetSide(int16_t dx, int16_t dy, int landingOffset)
{
	GFXPOINT new_pt;
	STAMP lander_s;
	STAMP shadow_s;
	STAMP shield_s;
	GFXCONTEXT OldContext {};

	new_pt.y = curLanderLoc.y + dy;
	if (new_pt.y < 0)
	{
		new_pt.y = 0;
		dy = new_pt.y - curLanderLoc.y;
		dx = 0;
		ZeroVelocityComponents(&uqm::flagship().velocity);
	}
	else if (new_pt.y > (MapHeight() << MagShift) - 1)
	{
		new_pt.y = (MapHeight() << MagShift) - 1;
		dy = new_pt.y - curLanderLoc.y;
		dx = 0;
		ZeroVelocityComponents(&uqm::flagship().velocity);
	}

	new_pt.x = curLanderLoc.x + dx;
	if (new_pt.x < 0)
	{
		new_pt.x += ScaledMapWidth() << MagShift;
	}
	else if (new_pt.x >= ScaledMapWidth() << MagShift)
	{
		new_pt.x -= ScaledMapWidth() << MagShift;
	}

	curLanderLoc = new_pt;

	if (is3DO(uqm::UQMOptions::read().landerStyle))
	{
		OldContext = SetContext(PlanetContext);
	}
	else
	{
		OldContext = SetContext(RadarContext);
	}

	BatchGraphics();

	// Display planet area, accounting for horizontal wrapping if
	// near the edges.
	{
		STAMP s;

		ClearDrawable();
		s.origin.x = -new_pt.x + (MapSurface.width >> 1);
		s.origin.y = -new_pt.y + (MapSurface.height >> 1);
		s.frame = pSolarSysState->Orbit.TopoZoomFrame;

		DrawStamp(&s);
		s.origin.x += ScaledMapWidth() << MagShift;
		DrawStamp(&s);
		s.origin.x -= ScaledMapWidth() << (MagShift + 1);
		DrawStamp(&s);
	}

	BuildObjectList();

	DrawBatch(DisplayArray, DisplayLinks, 0);

	// Draw the lander while is still alive and keep drawing for a few
	// frames while it is exploding
	if (crew_left || damage_index || explosion_index < 3)
	{
		lander_s.origin.x = MapSurface.width >> 1;
		lander_s.origin.y = (MapSurface.height >> 1) + landingOffset;
		lander_s.frame = LanderFrame[0];

		if (landingOffset != OnTheGround)
		{ // Landing, draw a shadow
			shadow_s.origin.x = lander_s.origin.y + (MapSurface.width >> 1)
							  - (MapSurface.height >> 1); //2;
			shadow_s.origin.y = lander_s.origin.y;
			shadow_s.frame = lander_s.frame;
			SetContextForeGroundColor(BlackColor);
			DrawFilledStamp(&shadow_s);
		}

		if (damage_index == 0)
		{ // No damage -- normal lander
			DrawStamp(&lander_s);
		}
		else if (shieldHit)
		{ // Was protected by a shield
			--damage_index;
			if (damage_index > 0)
			{
				shield_s.origin = lander_s.origin;
				shield_s.frame = SetEquFrameIndex(
					LanderFrame[4], lander_s.frame);

				// XXX: Shouldn't this color-cycle with damage_index?
				//   damage_index is used, but only as a VGA index!
				/*SetContextForeGroundColor (BuildColor (
						MakeRgb15 (0x1F, 0x1F, 0x1F) | 0x8000,
						damage_index));*/
				SetContextForeGroundColor(
					BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F),
							   damage_index));
				DrawFilledStamp(&shield_s);
			}
			DrawStamp(&lander_s);
		}
		else
		{ // Direct hit, no shield
			--damage_index;
			SetContextForeGroundColor(
				DamageColorCycle(BlackColor, damage_index));
			DrawFilledStamp(&lander_s);
		}
	}

	if (landingOffset == OnTheGround && crew_left
		&& GetPredLink(DisplayLinks) != EndOfList)
	{
		CheckObjectCollision(EndOfList);
	}

	{
		PLANETSIDE_DESC* pPSD = planetSideDesc;
		if (pPSD->NumFrames)
		{
			--pPSD->NumFrames;
			SetContextForeGroundColor(
				pPSD->ColorCycle[pPSD->NumFrames >> 1]);

			if (isPC(uqm::UQMOptions::read().landerStyle))
			{
				pPSD->MineralText[0].baseline.x = RadarWidth() >> 1;
				pPSD->MineralText[0].baseline.y = ResScale(8);
				// original value
			}
			else
			{
				pPSD->MineralText[0].baseline.x -= dx;
				pPSD->MineralText[0].baseline.y -= dy;
			}
			font_DrawText(&pPSD->MineralText[0]);
			pPSD->MineralText[1].baseline.x =
				pPSD->MineralText[0].baseline.x;
			pPSD->MineralText[1].baseline.y =
				pPSD->MineralText[0].baseline.y + ResScale(7);
			font_DrawText(&pPSD->MineralText[1]);
			pPSD->MineralText[2].baseline.x =
				pPSD->MineralText[1].baseline.x;
			pPSD->MineralText[2].baseline.y =
				pPSD->MineralText[1].baseline.y + ResScale(7);
			font_DrawText(&pPSD->MineralText[2]);
		}
	}

	RedrawSurfaceScan(&new_pt);

	if (lander_flags & KillCrew)
	{
		DeltaLanderCrew(-1, LIGHTNING_DISASTER);
	}

	if (isPC(uqm::UQMOptions::read().landerStyle))
	{
		DrawRadarBorder();
		RotatePlanetSphere(true, nullptr);
	}


	UnbatchGraphics();

	SetContext(OldContext);
}

static constexpr auto AnimFrameRate {(c_gameSecond / 30)};

static void
animationInterframe(TimeCount* TimeIn, uint16_t periods)
{

	while (periods > 0)
	{
		RotatePlanetSphere(true, nullptr);

		if (GetTimeCounter() >= *TimeIn + AnimFrameRate)
		{
			*TimeIn = GetTimeCounter();
			periods--;
		}
	}
}

static void
AnimateLaunch(FRAME farray, bool isLanding)
{
	GFXRECT r;
	STAMP s;
	static TimeCount NextTime;
	static TimeCount psNextTime;
	TimeCount Now;

	SetContext(PlanetContext);

	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = 0;
	r.extent.height = 0;
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = farray;

	uint16_t num_frames {GetFrameCount(s.frame)};
	NextTime = GetTimeCounter() + (c_gameSecond / 22);
	psNextTime = GetTimeCounter() + PlanetSideRate;
	while (num_frames > 0)
	{
		RotatePlanetSphere(true, &s);

		Now = GetTimeCounter();

		if (!isLanding && isPC(uqm::UQMOptions::read().landerStyle) && Now >= psNextTime)
		{
			// 10 to clear the lander off of the screen
			ScrollPlanetSide(0, 0,
							 -(MapSurface.height / 2 + ResScale(10)));
			psNextTime = Now + PlanetSideRate;
		}

		if (Now >= NextTime)
		{
			NextTime = Now + (c_gameSecond / 22);
			num_frames--;

			GetFrameRect(s.frame, &r);
			s.frame = IncFrameIndex(s.frame);

			RepairBackRect(&r);
			DrawDefaultPlanetSphere();

			if (num_frames == 0)
			{
				break;
			}

			DrawStamp(&s);
		}
	}
}

static void
AnimateLanderWarmup(void)
{
	int16_t num_crew {};
	STAMP s;
	//GFXCONTEXT OldContext; unused
	TimeCount TimeIn = GetTimeCounter();

	if (is3DO(uqm::UQMOptions::read().landerStyle))
	{
		SetContext(RadarContext);
	}
	else
	{
		SetContext(PCLanderContext);
	}

	s.origin.x = 0;
	s.origin.y = 0;

	s.frame = SetAbsFrameIndex(
		is3DO(uqm::UQMOptions::read().landerStyle) ? LanderFrame[0] : LanderFrame[7],
		(AngleToFacing(FullCircle) << 1) + 1);

	DrawStamp(&s);

	animationInterframe(&TimeIn, 2);

	for (num_crew = 0; num_crew < (NumCrewCols() * NumCrewRows())
					   && uqm::gameState().sis().CrewEnlisted;
		 ++num_crew)
	{
		animationInterframe(&TimeIn, 1);

		--uqm::gameState().sis().CrewEnlisted;

		DrawMineralHelpers();
		if (!uqm::UQMOptions::read().submenu)
		{
			DeltaSISGauges(UndefinedDelta, 0, 0);
		}

		DeltaLanderCrew(1, 0);
	}

	animationInterframe(&TimeIn, 2);

	if (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_SHOT))
	{
		s.frame = SetAbsFrameIndex(s.frame, 58);
	}
	else
	{
		s.frame = SetAbsFrameIndex(s.frame,
								   (AngleToFacing(FullCircle) << 1) + 2);
	}
	DrawStamp(&s);

	animationInterframe(&TimeIn, 2);

	if (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_SPEED))
	{
		s.frame = SetAbsFrameIndex(s.frame, 57);
	}
	else
	{
		s.frame = SetAbsFrameIndex(s.frame,
								   (AngleToFacing(FullCircle) << 1) + 3);
		DrawStamp(&s);

		animationInterframe(&TimeIn, 2);

		s.frame = IncFrameIndex(s.frame);
	}
	DrawStamp(&s);

	if (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_CARGO))
	{
		animationInterframe(&TimeIn, 2);

		s.frame = SetAbsFrameIndex(s.frame, 59);
		DrawStamp(&s);
	}

	animationInterframe(&TimeIn, 2);

	PlaySound(GetSound(LanderSounds, LANDER_DEPARTS),
			  NotPositional(), nullptr, GameSoundPriority + 1);
}

static void
InitPlanetSide(GFXPOINT pt)
{
	// Adjust landing location by a random jitter.
	const auto RandomMiss = [&] {
		return ResScale(64);
	};
	// Jitter the X landing point.
	pt.x -= RandomMiss() - uqm::random().next() % (RandomMiss() << 1);
	if (pt.x < 0)
	{
		pt.x += (ScaledMapWidth() << MagShift);
	}
	else if (pt.x >= (ScaledMapWidth() << MagShift))
	{
		pt.x -= (ScaledMapWidth() << MagShift);
	}

	// Jitter the Y landing point.
	pt.y -= RandomMiss() - uqm::random().next() % (RandomMiss() << 1);
	if (pt.y < 0)
	{
		pt.y = 0;
	}
	else if (pt.y >= (MapHeight() << MagShift))
	{
		pt.y = (MapHeight() << MagShift) - 1;
	}

	if (isPC(uqm::UQMOptions::read().landerStyle))
	{
		SetContext(RadarContext);
		SetContextBackGroundColor(BlackColor);
		ClearSISRect(ClearSisRadar);
		MapSurface = MAKE_EXTENT(RadarWidth(), RadarHeight());

		DrawMineralHelpers();
	}
	else
	{
		SetContext(PlanetContext);
		MapSurface = MAKE_EXTENT(SurfaceWidth(), SurfaceHeight());
	}

	curLanderLoc = pt;

	if (isPC(uqm::UQMOptions::read().whichFonts))
	{
		SetContextFont(TinyFont);
	}
	else
	{
		SetContextFont(TinyFontBold);
	}

	{
		GFXRECT r;

		GetContextClipRect(&r);

		SetTransitionSource(&r);
		BatchGraphics();

		{
			STAMP s;

			// Note - This code is the same as in ScrollPlanetSize,
			// Display planet area, accounting for horizontal wrapping if
			// near the edges.
			ClearDrawable();
			s.origin.x = -pt.x + (MapSurface.width >> 1);
			s.origin.y = -pt.y + (MapSurface.height >> 1);
			s.frame = pSolarSysState->Orbit.TopoZoomFrame;
			DrawStamp(&s);
			s.origin.x += ScaledMapWidth() << MagShift;
			DrawStamp(&s);
			s.origin.x -= ScaledMapWidth() << (MagShift + 1);
			DrawStamp(&s);

			DrawRadarArea();

			if (!isPC(uqm::UQMOptions::read().landerStyle))
			{
				ScreenTransition(uqm::UQMOptions::read().scrTrans, &r);
			}
		}
		UnbatchGraphics();
	}


	uqm::gameState().setFlag(uqm::GameFlag::PLANETARY_LANDING, static_cast<uint32_t>(1));
}

static auto ShuttleFireWait() { return (isPC(uqm::UQMOptions::read().landerStyle) ? 10 : 14); }

static void
LanderFire(int16_t facing)
{
	int16_t wdx {};
	int16_t wdy {};
	ELEMENT* WeaponElementPtr {};
	uint16_t angle {};

	HELEMENT hWeaponElement {AllocElement()};
	if (hWeaponElement == nullptr)
	{
		return;
	}

	LockElement(hWeaponElement, &WeaponElementPtr);

	WeaponElementPtr->playerNr = PS_HUMAN_PLAYER;
	WeaponElementPtr->mass_points = 1;
	WeaponElementPtr->life_span = 12;
	WeaponElementPtr->state_flags = FiniteLife;
	WeaponElementPtr->next.location = curLanderLoc;

	SetPrimType(&DisplayArray[WeaponElementPtr->PrimIndex], STAMP_PRIM);
	DisplayArray[WeaponElementPtr->PrimIndex].Object.Stamp.frame =
		SetAbsFrameIndex(LanderFrame[0],
						 /* shot images immediately follow the lander images */
						 facing + AngleToFacing(FullCircle));

	if (CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_UP]
		|| CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_THRUST])
	{
		GetCurrentVelocityComponents(&uqm::flagship().velocity, &wdx, &wdy);
	}
	else
	{
		wdx = 0;
		wdy = 0;
	}

	angle = FacingToAngle(facing);
	SetVelocityComponents(
		&WeaponElementPtr->velocity,
		Cosine(angle, WorldToVelocity(ResScale(2 * 3))) + wdx,
		Sine(angle, WorldToVelocity(ResScale(2 * 3))) + wdy);

	UnlockElement(hWeaponElement);

	InsertElement(hWeaponElement, GetHeadElement());

	PlaySound(GetSound(LanderSounds, LANDER_SHOOTS),
			  NotPositional(), nullptr, GameSoundPriority);
}

static void
put_node(ELEMENT* ElementPtr)
{
	ELEMENT* NodeElementPtr {};
	NODE_INFO info;

	HELEMENT hNodeElement {AllocElement()};
	if (!hNodeElement)
	{
		return;
	}

	callGenerateForScanType(pSolarSysState,
							pSolarSysState->pOrbitalDesc, ElementPtr->cycle,
							MINERAL_SCAN, &info);

	setNodeNotRetrieved(&pSolarSysState->SysInfo.PlanetInfo,
						MINERAL_SCAN, ElementPtr->cycle);

	ElementPtr->next.location.x >>= MagShift;
	ElementPtr->next.location.y >>= MagShift;

	CustomMineralDeposit(&info, ElementPtr->turn_wait,
						 ElementPtr->thrust_wait, ElementPtr->next.location);

	LockElement(hNodeElement, &NodeElementPtr);

	NodeElementPtr->scan_node =
		uqm::makeWord(MINERAL_SCAN, ElementPtr->cycle + 1);
	NodeElementPtr->playerNr = PS_NON_PLAYER;
	NodeElementPtr->current.location = info.loc_pt;

	SetPrimType(&DisplayArray[NodeElementPtr->PrimIndex], STAMP_PRIM);

	NodeElementPtr->turn_wait = info.type;

	// JMS: Partially scavenged energy blips won't return
	// anymore to original size after leaving planet.
	NodeElementPtr->mass_points = uqm::highByte(info.density)
								- pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[MINERAL_SCAN][ElementPtr->cycle];

	NodeElementPtr->current.image.frame = SetAbsFrameIndex(
		MiscDataFrame, (NumScandotTransitions * 2)
						   + ElementCategory(info.type) * 5);
	NodeElementPtr->next.image.frame = SetRelFrameIndex(
		NodeElementPtr->current.image.frame,
		uqm::lowByte(info.density) + 1);

	DisplayArray[NodeElementPtr->PrimIndex].Object.Stamp.frame =
		IncFrameIndex(NodeElementPtr->next.image.frame);

	NodeElementPtr->next.location.x =
		NodeElementPtr->current.location.x << MagShift;
	NodeElementPtr->next.location.y =
		NodeElementPtr->current.location.y << MagShift;

	NodeElementPtr->state_flags |= Appearing;
	uqm::gameState().setFlag(uqm::GameFlag::PLANETARY_CHANGE, static_cast<uint32_t>(1));

	UnlockElement(hNodeElement);

	PutElement(hNodeElement);
}

static void
spawn_node(ELEMENT* ElementPtr)
{
	if (ElementPtr->next.location.y < 0)
	{
		ElementPtr->next.location.y = 0;
		ZeroVelocityComponents(&ElementPtr->velocity);
	}
	if (ElementPtr->next.location.y > (MapHeight() << MagShift) - 1)
	{
		ElementPtr->next.location.y = (MapHeight() << MagShift) - 1;
		ZeroVelocityComponents(&ElementPtr->velocity);
	}

	if (ElementPtr->life_span > 0)
	{
		ELEMENT* NodeElementPtr {};
		int16_t arc {};
		const uint16_t half_dist = ElementPtr->life_span >> 1;

		HELEMENT hNodeElement {AllocElement()};
		if (!hNodeElement)
		{
			return;
		}

		LockElement(hNodeElement, &NodeElementPtr);

		arc = (abs(abs(half_dist - ElementPtr->life_span)
				   - half_dist))
		   << 1;

		NodeElementPtr->playerNr = PS_NON_PLAYER;
		NodeElementPtr->mass_points = 1;
		NodeElementPtr->life_span = 1;
		NodeElementPtr->state_flags = FiniteLife | Nonsolid;
		NodeElementPtr->next.location.x = ElementPtr->next.location.x;
		NodeElementPtr->next.location.y = ElementPtr->next.location.y - arc;

		SetPrimType(&DisplayArray[NodeElementPtr->PrimIndex], STAMP_PRIM);
		DisplayArray[NodeElementPtr->PrimIndex].Object.Stamp.frame =
			IncFrameIndex(ElementPtr->next.image.frame);

		UnlockElement(hNodeElement);
		InsertElement(hNodeElement, GetHeadElement());

		return;
	}

	ZeroVelocityComponents(&ElementPtr->velocity);

	put_node(ElementPtr);
}

static void
LobMineralNode(uint16_t which_node, uint8_t type, const uint16_t amount)
{
	uint16_t dist {};
	uint16_t deposit_quality_fine {};
	uint16_t deposit_quality_gross {};
	ELEMENT* NodeElementPtr {};

	HELEMENT hNodeElement {AllocElement()};
	if (hNodeElement == nullptr)
	{
		return;
	}

	const uint16_t angle {(uint16_t)uqm::random().next()};
	dist = ((uint16_t)uqm::random().next() % 8) + 4;

	deposit_quality_fine = amount * 10;
	if (deposit_quality_fine < 150)
	{
		deposit_quality_gross = 0;
	}
	else if (deposit_quality_fine < 225)
	{
		deposit_quality_gross = 1;
	}
	else
	{
		deposit_quality_gross = 2;
	}

	LockElement(hNodeElement, &NodeElementPtr);

	NodeElementPtr->playerNr = PS_NON_PLAYER;
	NodeElementPtr->mass_points = 1;
	NodeElementPtr->life_span = dist;
	NodeElementPtr->state_flags = FiniteLife | Nonsolid | Appearing;
	NodeElementPtr->next.location = curLanderLoc;
	NodeElementPtr->cycle = which_node;
	NodeElementPtr->turn_wait = type;
	NodeElementPtr->thrust_wait = amount;
	NodeElementPtr->preprocess_func = spawn_node;

	SetPrimType(&DisplayArray[NodeElementPtr->PrimIndex], STAMPFILL_PRIM);
	SetPrimColor(&DisplayArray[NodeElementPtr->PrimIndex], BlackColor);
	NodeElementPtr->current.image.frame = SetAbsFrameIndex(
		MiscDataFrame, (NumScandotTransitions * 2)
						   + ElementCategory(type) * 5);

	NodeElementPtr->next.image.frame = SetRelFrameIndex(
		NodeElementPtr->current.image.frame,
		uqm::lowByte(uqm::makeWord(deposit_quality_gross, amount)) + 1);
	DisplayArray[NodeElementPtr->PrimIndex].Object.Stamp.frame =
		IncFrameIndex(NodeElementPtr->next.image.frame);

	SetVelocityVector(&NodeElementPtr->velocity, ResScale(dist >> 1), angle);

	UnlockElement(hNodeElement);

	InsertElement(hNodeElement, GetHeadElement());
}

static void
ScatterDeposits(void)
{
	uint16_t numDeposits {};
	uint8_t NumNodesGrabbed {};
	const PLANETSIDE_DESC* pPSD = planetSideDesc;

	if (!uqm::UQMOptions::read().scatterElements)
	{
		return;
	}

	if (!pPSD->NodeData.NumNodesGrabbed)
	{
		return;
	}

	NumNodesGrabbed = pPSD->NodeData.NumNodesGrabbed;

	numDeposits = callGenerateForScanType(pSolarSysState,
										  pSolarSysState->pOrbitalDesc, GenerateAll, MINERAL_SCAN, nullptr);

	for (uint16_t i {numDeposits}; i < (numDeposits + NumNodesGrabbed); i++)
	{
		const uint8_t affix = i - numDeposits;
		const double penalty = (double)uqm::rangeMinMax(25, 75, uqm::random().next()) / 100;

		if (pPSD->NodeData.NodeAmounts[affix] * penalty < 1)
		{
			continue;
		}

		LobMineralNode(i, pPSD->NodeData.NodeType[affix],
					   pPSD->NodeData.NodeAmounts[affix] * penalty);
	}
}

static bool
LanderExplosion(void)
{
	ELEMENT* ExplosionElementPtr {};

	HELEMENT hExplosionElement {AllocElement()};
	if (!hExplosionElement)
	{
		return false;
	}

	LockElement(hExplosionElement, &ExplosionElementPtr);

	ExplosionElementPtr->playerNr = PS_HUMAN_PLAYER;
	ExplosionElementPtr->mass_points = DeathExplosion;
	ExplosionElementPtr->state_flags = FiniteLife;
	ExplosionElementPtr->next.location = curLanderLoc;
	ExplosionElementPtr->preprocess_func = object_animation;
	// Animation advances every 3rd frame
	ExplosionElementPtr->turn_wait = uqm::makeByte(2, 2);
	ExplosionElementPtr->life_span = ExplosionLife
								   * (uqm::lowNibble(ExplosionElementPtr->turn_wait) + 1);

	SetPrimType(&DisplayArray[ExplosionElementPtr->PrimIndex],
				STAMP_PRIM);
	DisplayArray[ExplosionElementPtr->PrimIndex].Object.Stamp.frame =
		SetAbsFrameIndex(LanderFrame[0], 46);

	UnlockElement(hExplosionElement);

	InsertElement(hExplosionElement, GetHeadElement());

	PlaySound(GetSound(LanderSounds, LANDER_DESTROYED),
			  NotPositional(), nullptr, GameSoundPriority + 1);

	ScatterDeposits();

	return true;
}

static bool
DoPlanetSide(LanderInputState* pMS)
{
	int16_t dx = 0;
	int16_t dy = 0;

	const auto ShuttleTurnWait = [&] {
		return (isPC(uqm::UQMOptions::read().landerStyle) ? 1 : 3);
	};
	if (uqm::currentActivity() & CHECK_ABORT)
	{
		return (false);
	}

	if (!pMS->Initialized)
	{
		uint16_t landerSpeedNumer {};
		uint16_t angle {};

		pMS->Initialized = true;

		turn_wait = 0;
		weapon_wait = 0;

		angle = FacingToAngle(GetFrameIndex(LanderFrame[0]));
		landerSpeedNumer = uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_SPEED) ?
							   WorldToVelocity(2 * ResScale(14)) :
							   WorldToVelocity(2 * ResScale(8));

#ifdef FAST_FAST
		landerSpeedNumer = WorldToVelocity(ResScale(48));
#endif

		SetVelocityComponents(&uqm::flagship().velocity,
							  Cosine(angle, landerSpeedNumer) / LanderSpeedDenom(),
							  Sine(angle, landerSpeedNumer) / LanderSpeedDenom());

		return true;
	}
	else if (crew_left /* alive and taking off */
			 && ((CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_ESCAPE] || CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_SPECIAL])
				 || planetSideDesc->InTransit))
	{
		return false;
	}

#ifdef DEBUG
	if (PulsedInputState.menu[KEY_DEBUG_2])
	{
		KillLanderCrewSeq(crew_left, c_gameSecond / 20);
	}
#endif

	else if (!crew_left && !damage_index)
	{ // Dead, damage dealt, and exploding
		if (explosion_index > ExplosionLife + EXPLOSION_WAIT_FRAMES)
		{
			return false;
		}

		if (explosion_index > ExplosionLife)
		{ // Keep going until the wait expires
			++explosion_index;
		}
		else if (explosion_index == 0)
		{ // Start the explosion animation
			if (LanderExplosion())
			{
				// Advance the state only once we've got the element
				++explosion_index;
			}
			else
			{ // We could not allocate because the queue was full, but
				// we will get another chance on the next iteration
				uqm::log::warn("DoPlanetSide(): could not allocate explosion element!");
			}
		}
	}
	else
	{
		if (crew_left)
		{
			int16_t index = GetFrameIndex(LanderFrame[0]);

			if (turn_wait)
			{
				--turn_wait;
			}
			else if (CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_LEFT] || CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_RIGHT])
			{
				uint16_t landerSpeedNumer {};
				uint16_t angle {};

				if (CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_LEFT])
				{
					--index;
				}
				else
				{
					++index;
				}

				index = NormalizeFacing(index);
				LanderFrame[0] = SetAbsFrameIndex(LanderFrame[0], index);

				angle = FacingToAngle(index);
				landerSpeedNumer = uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_SPEED) ?
									   WorldToVelocity(ResScale(2 * 14)) :
									   WorldToVelocity(ResScale(2 * 8));

#ifdef FAST_FAST
				landerSpeedNumer = WorldToVelocity(ResScale(48));
#endif

				SetVelocityComponents(&uqm::flagship().velocity,
									  Cosine(angle, landerSpeedNumer)
										  / LanderSpeedDenom(),
									  Sine(angle, landerSpeedNumer)
										  / LanderSpeedDenom());

				turn_wait = ShuttleTurnWait();
			}
			if (CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_THRUST]
				|| CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_UP])
			{
				GetNextVelocityComponents(
					&uqm::flagship().velocity, &dx, &dy, 1);
			}
			else
			{
				dx = dy = 0;
			}

			if (weapon_wait)
			{
				--weapon_wait;
			}
			else if (CurrentInputState.key[static_cast<int>(PlayerControlTemplates[0])][KEY_WEAPON])
			{
				LanderFire(index);

				weapon_wait = ShuttleFireWait();
				if (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_SHOT))
				{
					weapon_wait >>= 1;
				}
			}
		}
	}

	ScrollPlanetSide(dx, dy, OnTheGround);

	if (isDifficulty(uqm::Difficulty::Hard))
	{
		static TimeCount TimeOutClock;
		const TimeCount Now = GetTimeCounter();

		if (Now >= TimeOutClock)
		{
			GameClockTick();
			TimeOutClock = Now + ClockFrameRate;
		}
	}

	SleepThreadUntil(pMS->NextTime);
	// NOTE: The rate is not stabilized
	pMS->NextTime = GetTimeCounter() + PlanetSideRate;

	return true;
}

void MaskLanderGraphics(void)
{
	uint8_t currFlagState {};

	if (!uqm::UQMOptions::read().showUpgrades)
	{
		return;
	}

	currFlagState = (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_SHOT) << 0) + (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_SPEED) << 1) + (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_CARGO) << 2);

	if (currFlagState != LanderUpgradesFlag)
	{
		uint16_t i {};
		FRAME mods = CaptureDrawable(LoadGraphic(LanderModsPmapAnimRes));
		FRAME lander {};
		FRAME mask {};
		const DrawMode mode = MAKE_DRAW_MODE(DRAW_REPLACE, DrawFactor1);

		// Reload landersprites for remasking
		DestroyDrawable(ReleaseDrawable(LanderFrame[0]));
		LanderFrame[0] =
			CaptureDrawable(LoadGraphic(LanderMaskPmapAnimRes));

		LanderUpgradesFlag = currFlagState;

		for (uint16_t j {0}; j < 3; j++)
		{
			if (currFlagState & (1 << j))
			{
				for (i = 0; i < 16; i++)
				{
					mask = SetAbsFrameIndex(mods, i + (j * 16));
					lander = SetAbsFrameIndex(LanderFrame[0], i);
					ApplyMask(mask, lander, mode, nullptr);
				}
			}
		}

		DestroyDrawable(ReleaseDrawable(mods));
		mods = 0;
	}
}

void FreeLanderData(void)
{
	uint16_t i {};

	if (LanderFrame[0] == nullptr)
	{
		return;
	}

	for (i = 0; i < NumOrbitThemes; ++i)
	{
		DestroyMusic(OrbitMusic[i]);
		OrbitMusic[i] = 0;
	}

	delete LanderSounds;
	LanderSounds = 0;

	const uint16_t landerFrameCount {std::size(LanderFrame)};
	for (i = 0; i < landerFrameCount; ++i)
	{
		DestroyDrawable(ReleaseDrawable(LanderFrame[i]));
		LanderFrame[i] = 0;
	}

	LanderUpgradesFlag = 0;
}

void LoadLanderData(void)
{
	if (LanderFrame[0] != 0)
	{
		return;
	}

	LanderFrame[0] =
		CaptureDrawable(LoadGraphic(LanderMaskPmapAnimRes));
	LanderFrame[1] =
		CaptureDrawable(LoadGraphic(QuakeMaskPmapAnimRes));
	LanderFrame[2] =
		CaptureDrawable(LoadGraphic(LightningMaskAnimRes));
	LanderFrame[3] =
		CaptureDrawable(LoadGraphic(LavaMaskPmapAnimRes));
	LanderFrame[4] =
		CaptureDrawable(LoadGraphic(LanderShieldMaskAnimRes));
	LanderFrame[5] =
		CaptureDrawable(LoadGraphic(LanderLaunchMaskPmapAnimRes));
	LanderFrame[6] =
		CaptureDrawable(LoadGraphic(LanderReturnMaskPmapAnimRes));
	LanderFrame[7] =
		CaptureDrawable(LoadGraphic(LanderPcMaskPmapAnimRes));

	LanderUpgradesFlag = 0;

	LanderSounds = uqm::loadSoundBank(LanderSoundsRes).release();

	{
		for (uint16_t i {0}; i < num_orbit_themes(); ++i)
		{
			OrbitMusic[i] = load_orbit_theme(i);
		}
	}
}

uint8_t
SetPlanetMusic(uint8_t planet_type)
{
	const uint8_t OrbitNum = planet_type % num_orbit_themes();
	LanderMusic = OrbitMusic[OrbitNum];

	return !(uqm::UQMOptions::read().use3doMusic || uqm::UQMOptions::read().useRemixMusic || uqm::UQMOptions::read().volasMusic) ? 0 : OrbitNum;
}

static void
ReturnToOrbit(void)
{
	GFXCONTEXT OldContext {};
	GFXRECT r;

	if (is3DO(uqm::UQMOptions::read().landerStyle))
	{
		OldContext = SetContext(PlanetContext);
		GetContextClipRect(&r);

		SetTransitionSource(&r);
		BatchGraphics();

		DrawStarBackGround();
		DrawDefaultPlanetSphere();
		DrawPlanetSurfaceBorder();
		RedrawSurfaceScan(nullptr);
		ScreenTransition(uqm::UQMOptions::read().scrTrans, &r);
		UnbatchGraphics();
	}
	else
	{
		GFXRECT b;
		OldContext = SetContext(RadarContext);
		GetContextClipRect(&r);
		b = r;

		SetTransitionSource(&r);

		b.corner.x = b.corner.y = 0;

		BatchGraphics();
		ClearDrawable(); // TODO: color this frame smh

		DrawRadarArea();

		if (!isPC(uqm::UQMOptions::read().landerStyle))
		{
			ScreenTransition(uqm::UQMOptions::read().scrTrans, &r);
		}

		RedrawSurfaceScan(nullptr);
		UnbatchGraphics();
	}

	SetContext(OldContext);
}

static void
IdlePlanetSide(LanderInputState* inputState, TimePeriod howLong)
{
#define IDLE_OFFSET
	const TimeCount TimeOut = GetTimeCounter() + howLong;

	while (GetTimeCounter() < TimeOut)
	{
		// 10 to clear the lander off of the screen
		ScrollPlanetSide(0, 0, -(MapSurface.height / 2 + ResScale(10)));
		SleepThreadUntil(inputState->NextTime);
		inputState->NextTime += PlanetSideRate;
	}
}

static constexpr auto MaxOffsets {20};
static constexpr auto MaxOffsetsHd {400};

static void
LandingTakeoffSequence(LanderInputState* inputState, bool landing)
{
	// We cannot solve a quadratic equation in a macro, so use a sensible max
	// ResScale (10) to clear the lander off of the screen
	const auto DistanceCovered = [&] {
		return (MapSurface.height / 2 + ResScale(10));
	};
	int landingOfs[MaxOffsets];
	int start {};
	int end {};
	int index {};
	int landingOfsHD[MaxOffsetsHd];

	// Produce smooth acceleration deltas from a simple 1..x progression
	int delta {0};
	// JMS_GFX: In HdResolutionFactor graphics we run out of default offsets. -> Use larger
	// offset value.
	const int max_offsets {chooseIfHd(MaxOffsets, MaxOffsetsHd)};

	for (index = 0; index < max_offsets && delta < DistanceCovered();
		 ++index)
	{
		delta += index + 1;

		if (IsHd())
		{
			landingOfsHD[index] = -delta;
		}
		else
		{
			landingOfs[index] = -delta;
		}
	}
	assert(delta >= DistanceCovered() && "Increase MAX_OFFSETS!");

	if (landing)
	{
		start = index - 1;
		end = -1;
		delta = -1;
	}
	else
	{ // takeoff
		start = 0;
		end = index;
		delta = +1;
	}

	if (landing)
	{
		IdlePlanetSide(inputState, c_gameSecond);
	}

	// Draw the landing/takeoff lander positions
	for (index = start; index != end; index += delta)
	{

		if (IsHd())
		{
			ScrollPlanetSide(0, 0, landingOfsHD[index]);
		}
		else
		{
			ScrollPlanetSide(0, 0, landingOfs[index]);
		}

		SleepThreadUntil(inputState->NextTime);
		inputState->NextTime += PlanetSideRate;
	}

	if (!landing)
	{
		IdlePlanetSide(inputState, c_gameSecond / 2);
	}
}

void SetLanderTakeoff(void)
{
	assert(planetSideDesc != nullptr);
	if (planetSideDesc)
	{
		planetSideDesc->InTransit = true;
	}
}

// Returns whether the lander is still alive at the end of the sequence
bool KillLanderCrewSeq(uint16_t numKilled, TimePeriod period)
{
	TimeCount TimeOut;

	TimeOut = GetTimeCounter();
	for (uint16_t i {0}; i < numKilled && crew_left; ++i)
	{
		TimeOut += period;
		DeltaLanderCrew(-1, LANDER_INJURED);
		SleepThreadUntil(TimeOut);
	}

	return crew_left > 0;
}

// Maps a temperature to a (0-7) hazard rating.
// Thermal hazards aren't exposed to the user as a hazard number,
// but the code still works with them that way.
unsigned
GetThermalHazardRating(int temp)
{
	const static int tempBreakpoints[] =
		{50, 100, 150, 250, 350, 550, 800};
	const size_t numBreakpoints = std::size(tempBreakpoints);

	for (unsigned i {0}; i < numBreakpoints; ++i)
	{
		if (temp < tempBreakpoints[i])
		{
			return i;
		}
	}

	return numBreakpoints;
}

// Given a hazard type and rating, return the chance (out of 256) of the hazard
// being generated.
static uint8_t
GetHazardChance(int hazardType, unsigned HazardRating)
{
	uint8_t TectonicsChanceTab[] = {0, 0, 1 * 3, 2 * 3, 4 * 3, 8 * 3, 16 * 3, 32 * 3};
	uint8_t WeatherChanceTab[] = {0, 0, 1 * 3, 2 * 3, 3 * 3, 6 * 3, 12 * 3, 24 * 3};
	const uint8_t FireChanceTab[] = {0, 0, 1 * 3, 2 * 3, 4 * 3, 12 * 3, 24 * 3, 48 * 3};

	if (IsExtended())
	{
		TectonicsChanceTab[1] = 1;
		WeatherChanceTab[1] = 1;
	}

	switch (hazardType)
	{
		case EARTHQUAKE_DISASTER:
			return TectonicsChanceTab[HazardRating];
		case LIGHTNING_DISASTER:
			return WeatherChanceTab[HazardRating];
		case LAVASPOT_DISASTER:
			return FireChanceTab[HazardRating];
	}

	return 0;
}

void PlanetSide(GFXPOINT planetLoc)
{
	int16_t index {};
	LanderInputState landerInputState;
	PLANETSIDE_DESC PSD;

	memset(&PSD, 0, sizeof(PSD));
	PSD.InTransit = true;

	// Set our chances of hazards occurring.
	PSD.TectonicsChance = GetHazardChance(EARTHQUAKE_DISASTER,
										  pSolarSysState->SysInfo.PlanetInfo.Tectonics);
	PSD.WeatherChance = GetHazardChance(LIGHTNING_DISASTER,
										pSolarSysState->SysInfo.PlanetInfo.Weather);
	PSD.FireChance = GetHazardChance(LAVASPOT_DISASTER,
									 GetThermalHazardRating(
										 pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature));

	PSD.ElementLevel = GetStorageBayCapacity()
					 - uqm::gameState().sis().TotalElementMass;
	PSD.MaxElementLevel = MaxScrounged();
	if (uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_CARGO))
	{
		PSD.MaxElementLevel <<= 1;
	}
	if (PSD.ElementLevel < PSD.MaxElementLevel)
	{
		PSD.MaxElementLevel = PSD.ElementLevel;
	}
	PSD.ElementLevel = 0;

	PSD.MineralText[0].align = ALIGN_CENTER;
	PSD.MineralText[0].pStr = PSD.AmountBuf;
	PSD.MineralText[1] = PSD.MineralText[0];
	PSD.MineralText[2] = PSD.MineralText[1];

	PSD.ColorCycle[0] = BuildColor(MakeRgb15(0x1F, 0x03, 0x00), 0x7F);
	PSD.ColorCycle[1] = BuildColor(MakeRgb15(0x1F, 0x0A, 0x00), 0x7D);
	PSD.ColorCycle[2] = BuildColor(MakeRgb15(0x1F, 0x11, 0x00), 0x7B);
	PSD.ColorCycle[3] = BuildColor(MakeRgb15(0x1F, 0x1F, 0x00), 0x71);
	for (index = 4; index < (NumTextFrames >> 1) - 4; ++index)
	{
		PSD.ColorCycle[index] =
			BuildColor(MakeRgb15(0x1F, 0x1F, 0x1F), 0x0F);
	}
	PSD.ColorCycle[(NumTextFrames >> 1) - 4] =
		BuildColor(MakeRgb15(0x1F, 0x1F, 0x00), 0x71);
	PSD.ColorCycle[(NumTextFrames >> 1) - 3] =
		BuildColor(MakeRgb15(0x1F, 0x11, 0x00), 0x7B);
	PSD.ColorCycle[(NumTextFrames >> 1) - 2] =
		BuildColor(MakeRgb15(0x1F, 0x0A, 0x00), 0x7D);
	PSD.ColorCycle[(NumTextFrames >> 1) - 1] =
		BuildColor(MakeRgb15(0x1F, 0x03, 0x00), 0x7F);
	planetSideDesc = &PSD;

	index = NormalizeFacing(uqm::random().next());
	LanderFrame[0] = SetAbsFrameIndex(LanderFrame[0], index);
	crew_left = 0;
	damage_index = 0;
	explosion_index = 0;

	AnimateLanderWarmup();
	AnimateLaunch(LanderFrame[5], true);

	InitPlanetSide(planetLoc);

	landerInputState.NextTime = GetTimeCounter() + PlanetSideRate;
	LandingTakeoffSequence(&landerInputState, true);
	PSD.InTransit = false;

	landerInputState.Initialized = false;
	landerInputState.InputFunc = DoPlanetSide;
	SetMenuSounds(MenuSoundNone, MenuSoundNone);

	DoInput(&landerInputState, false);

	if (!(uqm::currentActivity() & CHECK_ABORT))
	{
		if (crew_left == 0)
		{
			--uqm::gameState().sis().NumLanders;
			if (!uqm::UQMOptions::read().submenu)
			{
				DrawLanders();
			}

			ReturnToOrbit();
		}
		else
		{
			PSD.InTransit = true;
			PlaySound(GetSound(LanderSounds, LANDER_RETURNS),
					  NotPositional(), nullptr, GameSoundPriority + 1);

			LandingTakeoffSequence(&landerInputState, false);

			if (is3DO(uqm::UQMOptions::read().landerStyle))
			{
				ReturnToOrbit();
			}

			AnimateLaunch(LanderFrame[6], false);
			if (!uqm::UQMOptions::read().submenu)
			{
				DeltaSISGauges(crew_left, 0, 0);
			}
			else
			{ // Get the crew from landers back but not draw anything
				const uint16_t CrewCapacity = GetCrewPodCapacity();
				uqm::gameState().sis().CrewEnlisted += crew_left;
				if (uqm::gameState().sis().CrewEnlisted > CrewCapacity)
				{
					uqm::gameState().sis().CrewEnlisted = CrewCapacity;
				}
			}

			if (PSD.ElementLevel)
			{
				for (index = 0; index < NUM_ELEMENT_CATEGORIES; ++index)
				{
					uqm::gameState().sis().ElementAmounts[index] +=
						PSD.ElementAmounts[index];
					uqm::gameState().sis().TotalElementMass +=
						PSD.ElementAmounts[index];
				}
				if (!uqm::UQMOptions::read().submenu)
				{
					DrawStorageBays(false);
				}
			}

			uqm::gameState().sis().TotalBioMass += PSD.BiologicalLevel;

			if (isPC(uqm::UQMOptions::read().landerStyle))
			{
				ReturnToOrbit();
				InitPCLander(false);
			}
		}
	}

	planetSideDesc = nullptr;

	{
		HELEMENT hElement {};
		HELEMENT hNextElement {};

		for (hElement = GetHeadElement();
			 hElement; hElement = hNextElement)
		{
			ELEMENT* ElementPtr {};

			LockElement(hElement, &ElementPtr);
			hNextElement = _GetSuccLink(ElementPtr);
			if (ElementPtr->state_flags & FiniteLife)
			{
				UnlockElement(hElement);

				RemoveElement(hElement);
				FreeElement(hElement);

				continue;
			}
			UnlockElement(hElement);
		}
	}

	ZeroVelocityComponents(&uqm::flagship().velocity);
	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
}

static void
DrawMinDatText(void)
{
	TEXT text;
	GFXRECT rect;
	Color OldColor;

	if (!strlen(GameString(STATUS_STRING_BASE + 20)) || !strlen(GameString(STATUS_STRING_BASE + 21)))
	{
		return;
	}

	OldColor = SetContextForeGroundColor(BlackColor);
	rect.corner.x = ResScale(5);
	rect.corner.y = RadarHeight() - ResScale(7);
	rect.extent.width = ResScale(46);
	rect.extent.height = ResScale(6);
	DrawFilledRectangle(&rect);

	FONT OldFont {SetContextFont(TinyFontBold)};
	SetContextForeGroundColor(LandStatMinColor);
	text.align = ALIGN_LEFT;
	text.baseline.x = rect.corner.x + ResScale(1);
	text.baseline.y = rect.corner.y + rect.extent.height;
	text.pStr = GameString(STATUS_STRING_BASE + 20); // MIN
	text.CharCount = (uint16_t)~0;

	font_DrawText(&text);

	SetContextForeGroundColor(LandStatDatColor);
	text.align = ALIGN_RIGHT;
	text.baseline.x = rect.corner.x + rect.extent.width - ResScale(1);
	text.pStr = GameString(STATUS_STRING_BASE + 21); // DAT
	text.CharCount = (uint16_t)~0;

	font_DrawText(&text);

	SetContextFont(OldFont);
	SetContextForeGroundColor(OldColor);
}

void InitLander(uint8_t LanderFlags)
{
	GFXRECT r;

	SetContext(RadarContext);
	BatchGraphics();

	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = RadarWidth();
	r.extent.height = RadarHeight();
	SetContextForeGroundColor(BlackColor);
	DrawFilledRectangle(&r);

	if (uqm::gameState().sis().NumLanders || LanderFlags)
	{
		uint8_t ShieldFlags {};
		uint8_t capacity_shift {};
		uint16_t free_space {};
		STAMP s;

		s.origin.x = 0; /* set up powered-down lander */
		s.origin.y = 0;

		s.frame = SetAbsFrameIndex(LanderFrame[0], 32);

		DrawStamp(&s);
		DrawMinDatText();
		if (LanderFlags == 0)
		{
			ShieldFlags = uqm::gameState().flag(uqm::GameFlag::LANDER_SHIELDS);
			capacity_shift = uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_CARGO);
			DrawMineralHelpers();
		}
		else
		{
			ShieldFlags = (unsigned char)(LanderFlags & ((1 << EARTHQUAKE_DISASTER) | (1 << BIOLOGICAL_DISASTER) | (1 << LIGHTNING_DISASTER) | (1 << LAVASPOT_DISASTER)));
			s.frame = IncFrameIndex(s.frame);
			DrawStamp(&s);
			if (LanderFlags & (1 << (4 + 0)))
			{
				s.frame = SetAbsFrameIndex(s.frame, 57);
			}
			else
			{
				s.frame = SetAbsFrameIndex(s.frame,
										   (AngleToFacing(FullCircle) << 1) + 3);
				DrawStamp(&s);
				s.frame = IncFrameIndex(s.frame);
			}
			DrawStamp(&s);
			if (!(LanderFlags & (1 << (4 + 1))))
			{
				capacity_shift = 0;
			}
			else
			{
				capacity_shift = 1;
				s.frame = SetAbsFrameIndex(s.frame, 59);
				DrawStamp(&s);
			}
			if (LanderFlags & (1 << (4 + 2)))
			{
				s.frame = SetAbsFrameIndex(s.frame, 58);
			}
			else
			{
				s.frame = SetAbsFrameIndex(s.frame,
										   (AngleToFacing(FullCircle) << 1) + 2);
			}
			DrawStamp(&s);
		}

		free_space = GetStorageBayCapacity()
				   - uqm::gameState().sis().TotalElementMass;
		if ((int)free_space < (int)(MaxScrounged() << capacity_shift))
		{
			r.corner.x = ResScale(1);
			r.extent.width = ResScale(4);
			r.extent.height =
				ResScale(MaxHoldBars()
						 - ((free_space >> capacity_shift)
							* MaxHoldBars() / MaxScrounged())
						 + 2);
			SetContextForeGroundColor(BlackColor);
			DrawFilledRectangle(&r);
		}

		s.frame = SetAbsFrameIndex(s.frame, 37);
		if (ShieldFlags & (1 << EARTHQUAKE_DISASTER))
		{
			DrawStamp(&s);
		}
		s.frame = IncFrameIndex(s.frame);
		if (ShieldFlags & (1 << BIOLOGICAL_DISASTER))
		{
			DrawStamp(&s);
		}
		s.frame = IncFrameIndex(s.frame);
		if (ShieldFlags & (1 << LIGHTNING_DISASTER))
		{
			DrawStamp(&s);
		}
		s.frame = IncFrameIndex(s.frame);
		if (ShieldFlags & (1 << LAVASPOT_DISASTER))
		{
			DrawStamp(&s);
		}
	}

	UnbatchGraphics();
}

void InitPCLander(bool Loading)
{
	GFXRECT r;

	SetContext(GetPCLanderContext(nullptr));

	GetContextClipRect(&r);
	r.corner = MAKE_POINT(0, 0);
	SetContextForeGroundColor(BlackColor);

	BatchGraphics();

	DrawFilledRectangle(&r);

	if (uqm::gameState().sis().NumLanders && !Loading
		&& !(pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED)
		&& !(pSolarSysState->pOrbitalDesc->data_index >= FIRST_GAS_GIANT
			 && pSolarSysState->pOrbitalDesc->data_index <= LAST_GAS_GIANT))
	{ // Do not draw lander graphics if it's impossible to land (shielded
		// and gas giant) This mimics PC behaviour.
		uint8_t ShieldFlags {};
		uint8_t capacity_shift {};
		uint16_t free_space {};
		STAMP s;

		s.frame = SetAbsFrameIndex(LanderFrame[7], 32);
		s.origin = MAKE_POINT(0, 0);
		DrawStamp(&s);

		ShieldFlags = uqm::gameState().flag(uqm::GameFlag::LANDER_SHIELDS);
		capacity_shift = uqm::gameState().flag(uqm::GameFlag::IMPROVED_LANDER_CARGO);

		free_space = GetStorageBayCapacity()
				   - uqm::gameState().sis().TotalElementMass;
		if ((int)free_space < (int)(MaxScrounged() << capacity_shift))
		{
			uint16_t inc {};

			inc = MaxHoldBars() - ((free_space >> capacity_shift) * MaxHoldBars() / MaxScrounged()) + 1;

			r.corner = MAKE_POINT(ResScale(1), 0);
			r.extent = MAKE_EXTENT(ResScale(2), ResScale(inc));
			SetContextForeGroundColor(BlackColor);
			DrawFilledRectangle(&r);

			r.extent = MAKE_EXTENT(ResScale(1), ResScale(1));

			for (uint16_t i {0}; i < inc; i++)
			{
				if (i % 2 == 0)
				{ // Draw grey boxes every 2nd iteration
					r.corner.y += ResScale(1);
					SetContextForeGroundColor(VdkgrayColor);
					DrawFilledRectangle(&r);
					r.corner.x += ResScale(1);
					SetContextForeGroundColor(BlackColor);
					DrawFilledRectangle(&r);
					r.corner.x -= ResScale(1);
					r.corner.y += ResScale(1);
				}
			}
		}

		s.frame = SetAbsFrameIndex(s.frame, 37);
		if (ShieldFlags & (1 << EARTHQUAKE_DISASTER))
		{
			DrawStamp(&s);
		}
		s.frame = IncFrameIndex(s.frame);
		if (ShieldFlags & (1 << BIOLOGICAL_DISASTER))
		{
			DrawStamp(&s);
		}
		s.frame = IncFrameIndex(s.frame);
		if (ShieldFlags & (1 << LIGHTNING_DISASTER))
		{
			DrawStamp(&s);
		}
		s.frame = IncFrameIndex(s.frame);
		if (ShieldFlags & (1 << LAVASPOT_DISASTER))
		{
			DrawStamp(&s);
		}
	}

	UnbatchGraphics();
}