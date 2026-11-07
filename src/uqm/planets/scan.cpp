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

#include "core/types/enum.h"
#include "lander.h"
#include "lifeform.h"
#include "scan.h"
#include "../build.h"
#include "../colors.h"
#include "../cons_res.h"
#include "../controls.h"
#include "../menustat.h"
#include "../encount.h"
// for EncounterGroup
#include "../gamestr.h"
#include "../nameref.h"
#include "../util.h" // for get_fuel_to_sol()
#include "../process.h"
#include "../setup.h"
#include "../sounds.h"
#include "../state.h"
#include "../sis.h"
#include "../save.h"
#include "core/string/StringUtils.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/inplib.h"
#include "uqm/random/UQMRandom.h"

namespace uqm
{
extern FrameRef SpaceJunkFrame;

// define SPIN_ON_SCAN to allow the planet to spin
// while scaning  is going on
#undef SPIN_ON_SCAN

static constexpr auto FlashIndex {105};

static GfxContextRef ScanContext;

static GfxPoint planetLoc;
static GfxRect cursorRect;
static FrameRef eraseFrame;

// ScanSystem() menu items
// The first three are from enum PlanetScanTypes in planets.h
enum class ScanMenuItem
{
	ExitScan = *ScanType::Count,
	AutoScan,
	DispatchShuttle,
};
consteval bool isArithmeticEnum(ScanMenuItem) { return true; }


void RepairBackRect(GfxRect* pRect)
{
	GfxRect new_r;
	GfxRect old_r;

	GetContextClipRect(&old_r);
	new_r.corner.x = pRect->corner.x + old_r.corner.x;
	new_r.corner.y = pRect->corner.y + old_r.corner.y;
	new_r.extent = pRect->extent;

	new_r.extent.height += new_r.corner.y & 1;
	new_r.corner.y &= ~1;

	DrawFromExtraScreen(&new_r);
}

static void EraseCoarseScan(void)
{
	SetContext(PlanetContext);

	BatchGraphics();
	DrawStarBackGround();
	DrawDefaultPlanetSphere();
	UnbatchGraphics();
}

static void PrintScanTitleText(GfxText* t)
{
	SetContextForeGroundColor(uqm::usesAltScanColors(uqm::UQMOptions::read().whichCoarseScan) ? ScanPcTitleColor6014 : ScanPcTitleColor);

	if (!uqm::UQMOptions::read().nebulae)
	{
		font_DrawText(t);
	}
	else
	{
		font_DrawTracedText(t, (uqm::usesAltScanColors(uqm::UQMOptions::read().whichCoarseScan) ? ScanPcTitleColor6014 : GetContextForeGroundColor()), OutlineColor);
	}

	SetContextForeGroundColor(ScanInfoColor);
}

static void PrintScanTitlePC(GfxText* t, char* buf, int16_t xpos)
{
	GfxRect r;

	t->baseline.x = xpos;

	t->pStr = buf;
	t->CharCount = (uint16_t)~0;

	PrintScanTitleText(t);

	TextRect(t, &r, nullptr);
	t->baseline.x += r.extent.width;
}

static void PrintScanText(GfxText* t)
{
	if (!uqm::usesPictograms(uqm::UQMOptions::read().whichCoarseScan))
	{
		t->CharCount = (uint16_t)~0;
	}

	if (!uqm::UQMOptions::read().nebulae)
	{
		font_DrawText(t);
	}
	else
	{
		font_DrawTracedText(t, GetContextForeGroundColor(),
							OutlineColor);
	}
}

static void MakeScanValue(uqstl::span<char> buf, long val, const char* extra)
{
	if (val >= 10 * 100)
	{ // 1 decimal place
		const auto fmtResult = fmt::format_to_n(buf.data(), buf.size() - 1, "{}.{}{}", val / 100, (val / 10) % 10, extra);
		*fmtResult.out = '\0';
	}
	else
	{ // 2 decimal places
		const auto fmtResult = fmt::format_to_n(buf.data(), buf.size() - 1, "{}.{:02}{}", val / 100, val % 100, extra);
		*fmtResult.out = '\0';
	}
}

static void MakeDayValue(uqstl::span<char> buf, long val, const char* extra)
{
	if (pSolarSysState->SysInfo.PlanetInfo.RotationPeriod < 240 * 10)
	{
		const auto fmtResult = fmt::format_to_n(buf.data(), buf.size() - 1, "{}.{:02}{}", val / 100, val % 100, extra);
		*fmtResult.out = '\0';
	}
	else
	{
		const auto fmtResult = fmt::format_to_n(buf.data(), buf.size() - 1, "{}.{:02}{}", val / 10, val % 10, extra);
		*fmtResult.out = '\0';
	}
}

static int16_t GetRotationalPeriod(void)
{
	if (pSolarSysState->SysInfo.PlanetInfo.RotationPeriod < 240 * 10)
	{
		return (int16_t)(pSolarSysState->SysInfo.PlanetInfo.RotationPeriod
						 * 10 / 24);
	}
	else
	{
		return (int16_t)((pSolarSysState->SysInfo.PlanetInfo.RotationPeriod
						  + (24 >> 1))
						 / 24);
	}
}

void GetPlanetTitle(uqstl::span<char> buf)
{
	const char* named = GetNamedPlanetaryBody();

	if (named)
	{
		uqm::utf8StringCopy(buf.data(), buf.size(), named);
		return;
	}

	// Unnamed body, use world type
	const int val {pSolarSysState->pOrbitalDesc->data_index & ~*PlanetType::PlanetShielded};
	if (val >= PlanetType::FirstGasGiant)
	{
		if (!IsExtended())
		{
			const auto fmtResult = fmt::format_to_n(buf.data(), buf.size() - 1, "{}", GameString(GameStringBase::ScanString + 4 + 51));
			*fmtResult.out = '\0';
			// Gas Giant
		}
		else if (val <= PlanetType::NumberOfPlanetTypes)
		{
			const auto fmtResult = fmt::format_to_n(buf.data(), buf.size() - 1, "{}", GameString(GameStringBase::ScanString + 4 + 2 + val));
			*fmtResult.out = '\0';
			// "Color" Gas Giant
		}
	}
	else
	{
		const auto fmtResult = fmt::format_to_n(buf.data(), buf.size() - 1, "{} {}", GameString(GameStringBase::ScanString + 4 + val), GameString(GameStringBase::ScanString + 4 + 50));
		*fmtResult.out = '\0';
		// World
	}
}

static void HazardCase(uint8_t hazard)
{
	const auto PickHazardColor = [](bool a, bool b) {
		return a ? DullYellowColor : (b ? BrightRedColor : ScanInfoColor);
	};
	Color HazardColor;

	const uint16_t Temperature = GetThermalHazardRating(
		pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature);
	const uint16_t Weather = pSolarSysState->SysInfo.PlanetInfo.Weather + 1;
	const uint16_t Tectonics = pSolarSysState->SysInfo.PlanetInfo.Tectonics + 1;

	switch (static_cast<LanderEvent>(hazard))
	{
		case LanderEvent::LavaspotDisaster:
			HazardColor = PickHazardColor(Temperature > 1 && Temperature < 5,
										  Temperature > 4);
			break;
		case LanderEvent::LightningDisaster:
			HazardColor =
				PickHazardColor(Weather > 2 && Weather < 5, Weather > 4);
			break;
		case LanderEvent::EarthquakeDisaster:
			HazardColor = PickHazardColor(Tectonics > 2 && Tectonics < 6,
										  Tectonics > 5);
			break;
		default:
			HazardColor = ScanInfoColor;
			break;
	}

	SetContextForeGroundColor(HazardColor);
}

static auto ScanTitleY() { return ResScale(13); }

static void PrintCoarseScanPC(void)
{
	int32_t val {};
	GfxText t;
	char buf[200] {};

	GetPlanetTitle({buf, sizeof(buf)});

	SetContext(PlanetContext);

	t.align = TextAlign::Center;
	t.baseline.x = ResScale(OrigSisScreenWidth() >> 1);
	t.baseline.y = ScanTitleY();
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;

	SetContextForeGroundColor(uqm::usesAltScanColors(uqm::UQMOptions::read().whichCoarseScan) ?
								  ScanInfoColor :
								  ScanPcTitleColor);
	SetContextFont(MicroFont);

	BatchGraphics();
	PrintScanText(&t);

	SetContextFont(SafeBool(TinyFont, TinyFontCond));

	const auto LeftSideBaselineXPc = [&] {
		return ResScale(2);
	};
	const auto RightSideBaselineXPc = [&] {
		return (SisScreenWidth() - ResScale(73) + SafeX());
	};
	const auto ScanBaselineYPc = [&] {
		return (PlanetOrgY() - ResScale(13));
	};
	const auto ScanLeadingPc = [&] {
		return ResScale(10);
	};

	const auto OrbitscanText {[&](auto a) {
		return (GameString(GameStringBase::OrbitscanString + (a)));
	}};

	t.baseline.y = ScanBaselineYPc();
	t.align = TextAlign::Left;

	uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(0));
	PrintScanTitlePC(&t, buf, LeftSideBaselineXPc()); // "Orbit: "

	val = ((pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist * 100L
			+ (EarthRadius >> 1))
		   / EarthRadius);
	MakeScanValue(buf, val, OrbitscanText(1)); // " a.u."
	PrintScanText(&t);
	t.baseline.y += ScanLeadingPc();

	uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(2));
	PrintScanTitlePC(&t, buf, LeftSideBaselineXPc()); // "Atmo: "

	if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity
		== GasGiantAtmosphere)
	{
		uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(3)); // "Super Thick"
	}
	else if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == 0)
	{
		uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(4)); // "Vacuum"
	}
	else
	{
		val = (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity * 100
			   + (EarthAtmosphere >> 1))
			/ EarthAtmosphere;
		MakeScanValue(buf, val, OrbitscanText(5)); // " atm"
	}
	PrintScanText(&t);
	t.baseline.y += ScanLeadingPc();

	uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(6));
	PrintScanTitlePC(&t, buf, LeftSideBaselineXPc()); // "Temp: "

	fmt::format_to_sz_n(buf, "{}{} c",
						pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature,
						StrDegreeSign);

	if (uqm::UQMOptions::read().hazardColors) // Planet Temperature
	{
		HazardCase(*LanderEvent::LavaspotDisaster);
	}

	PrintScanText(&t);
	t.baseline.y += ScanLeadingPc();

	uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(7));
	PrintScanTitlePC(&t, buf, LeftSideBaselineXPc()); // "Weather: "

	if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == 0)
	{
		uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(8)); // "None"
	}
	else
	{
		fmt::format_to_sz_n(buf, "{} {}", OrbitscanText(9), // "Class"
							pSolarSysState->SysInfo.PlanetInfo.Weather + 1);
	}

	if (uqm::UQMOptions::read().hazardColors) // Weather
	{
		HazardCase(*LanderEvent::LightningDisaster);
	}

	PrintScanText(&t);
	t.baseline.y += ScanLeadingPc();

	uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(10));
	PrintScanTitlePC(&t, buf, LeftSideBaselineXPc()); // "Tectonics: "

	if (Plansize(pSolarSysState->SysInfo.PlanetInfo.PlanDataPtr->Type) == PlanetSize::GasGiant)
	{
		uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(8)); // "None"
	}
	else
	{
		fmt::format_to_sz_n(buf, "{} {}", OrbitscanText(9), // "Class"
							pSolarSysState->SysInfo.PlanetInfo.Tectonics + 1);
	}

	if (uqm::UQMOptions::read().hazardColors) // Tectonics
	{
		HazardCase(*LanderEvent::EarthquakeDisaster);
	}

	PrintScanText(&t);

	t.baseline.y = ScanBaselineYPc();

	uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(11));
	PrintScanTitlePC(&t, buf, RightSideBaselineXPc()); // "Mass: "

	val = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius;
	val = ((uint32_t)val * (uint32_t)val * (uint32_t)val / 100L
			   * pSolarSysState->SysInfo.PlanetInfo.PlanetDensity
		   + ((100L * 100L) >> 1))
		/ (100L * 100L);
	if (val == 0)
	{
		val = 1;
	}
	MakeScanValue(buf, val, OrbitscanText(12)); // " e.s."
	PrintScanText(&t);
	t.baseline.y += ScanLeadingPc();

	uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(13));
	PrintScanTitlePC(&t, buf, RightSideBaselineXPc()); // "Radius: "

	val = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius;
	MakeScanValue(buf, val, OrbitscanText(12)); // " e.s."
	PrintScanText(&t);
	t.baseline.y += ScanLeadingPc();

	uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(14));
	PrintScanTitlePC(&t, buf, RightSideBaselineXPc()); // "Gravity: "

	val = pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity;
	if (val == 0)
	{
		val = 1;
	}
	MakeScanValue(buf, val, OrbitscanText(15)); // " g."
	PrintScanText(&t);
	t.baseline.y += ScanLeadingPc();

	uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(16));
	PrintScanTitlePC(&t, buf, RightSideBaselineXPc()); // "Day: "

	val = GetRotationalPeriod();
	MakeDayValue(buf, val, OrbitscanText(17)); // " days"
	PrintScanText(&t);
	t.baseline.y += ScanLeadingPc();

	uqm::utf8StringCopy(buf, sizeof(buf), OrbitscanText(18));
	PrintScanTitlePC(&t, buf, RightSideBaselineXPc()); // "Tilt: "

	val = pSolarSysState->SysInfo.PlanetInfo.AxialTilt;
	if (val < 0)
	{
		val = -val;
	}
	fmt::format_to_sz_n(buf, "{}{}", val, StrDegreeSign);
	PrintScanText(&t);

	UnbatchGraphics();
}

static void PrintCoarseScan3DO(void)
{
	const auto ScanLeading = [&] {
		return ResScale(19);
	};
	int32_t val {};
	GfxText t;
	GfxStamp s;
	char buf[200] {};
	uint16_t frameIndex = 20;

	if (uqm::UQMOptions::read().whichCoarseScan == uqm::CoarseScanStyle::P6014Pictograms)
	{
		frameIndex = 24;
	}

	SetContext(PlanetContext);

	GetPlanetTitle({buf, sizeof(buf)});

	BatchGraphics();

	t.align = TextAlign::Center;
	t.baseline.x = ResScale(OrigSisScreenWidth() >> 1);
	t.baseline.y = ScanTitleY();
	t.pStr = buf;
	t.CharCount = (uint16_t)~0;
	SetContextForeGroundColor(ScanInfoColor);
	SetContextFont(MicroFont);
	PrintScanText(&t);

	s.frame = SetAbsFrameIndex(SpaceJunkFrame, frameIndex);

	s.origin.x = PlanetOrgX() - ResScale(107);
	s.origin.y = PlanetOrgY() - ResScale(46) + UseDosSpheres();

	DrawStamp(&s);

	const auto ScanBaselineY = [&] {
		return (PlanetOrgY() - ResScale(36) + UseDosSpheres());
	};
	const auto LeftSideBaselineX = [&] {
		return (PlanetOrgX() - ResScale(78));
	};
	const auto RightSideBaselineX = [&] {
		return (PlanetOrgX() + ResScale(78));
	};

	t.baseline.x = LeftSideBaselineX();
	t.baseline.y = ScanBaselineY();
	t.align = TextAlign::Left;

	val = ((pSolarSysState->SysInfo.PlanetInfo.PlanetToSunDist * 100L
			+ (EarthRadius >> 1))
		   / EarthRadius);
	MakeScanValue(buf, val, StrEarthSign);
	PrintScanText(&t);
	t.baseline.y += ScanLeading();

	if (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity
		== GasGiantAtmosphere)
	{
		uqm::strncpy_safe(buf, StrInfinitySign);
	}
	else
	{
		val = (pSolarSysState->SysInfo.PlanetInfo.AtmoDensity * 100
			   + (EarthAtmosphere >> 1))
			/ EarthAtmosphere;
		MakeScanValue(buf, val, StrEarthSign);
	}
	PrintScanText(&t);
	t.baseline.y += ScanLeading();

	fmt::format_to_sz_n(buf, "{}{}",
						pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature,
						StrDegreeSign);

	if (uqm::UQMOptions::read().hazardColors) // Planet Temperature
	{
		HazardCase(*LanderEvent::LavaspotDisaster);
	}

	PrintScanText(&t);
	t.baseline.y += ScanLeading();

	fmt::format_to_sz_n(buf, "<{}>",
						pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == 0 ? 0 : (pSolarSysState->SysInfo.PlanetInfo.Weather + 1));

	if (uqm::UQMOptions::read().hazardColors) // Weather
	{
		HazardCase(*LanderEvent::LightningDisaster);
	}

	PrintScanText(&t);
	t.baseline.y += ScanLeading();

	fmt::format_to_sz_n(buf, "<{}>",
						Plansize(
							pSolarSysState->SysInfo.PlanetInfo.PlanDataPtr->Type)
								== PlanetSize::GasGiant ?
							0 :
							(pSolarSysState->SysInfo.PlanetInfo.Tectonics + 1));

	if (uqm::UQMOptions::read().hazardColors) // Tectonics
	{
		HazardCase(*LanderEvent::EarthquakeDisaster);
	}

	PrintScanText(&t);

	t.baseline.x = RightSideBaselineX();
	t.baseline.y = ScanBaselineY();
	t.align = TextAlign::Right;

	val = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius;
	val = ((uint32_t)val * (uint32_t)val * (uint32_t)val / 100L
			   * pSolarSysState->SysInfo.PlanetInfo.PlanetDensity
		   + ((100L * 100L) >> 1))
		/ (100L * 100L);
	if (val == 0)
	{
		val = 1;
	}
	MakeScanValue(buf, val, StrEarthSign);

	if (uqm::UQMOptions::read().hazardColors)
	{
		SetContextForeGroundColor(ScanInfoColor);
	}

	PrintScanText(&t);
	t.baseline.y += ScanLeading();

	val = pSolarSysState->SysInfo.PlanetInfo.PlanetRadius;
	MakeScanValue(buf, val, StrEarthSign);
	PrintScanText(&t);
	t.baseline.y += ScanLeading();

	val = pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity;
	if (val == 0)
	{
		val = 1;
	}
	MakeScanValue(buf, val, StrEarthSign);
	PrintScanText(&t);
	t.baseline.y += ScanLeading();

	val = pSolarSysState->SysInfo.PlanetInfo.AxialTilt;
	if (val < 0)
	{
		val = -val;
	}
	fmt::format_to_sz_n(buf, "{}{}", val, StrDegreeSign);
	PrintScanText(&t);
	t.baseline.y += ScanLeading();

	val = GetRotationalPeriod();
	MakeDayValue(buf, val, StrEarthSign);
	PrintScanText(&t);

	UnbatchGraphics();
}

static void initPlanetLocationImage(void)
{
	// Get the cursor image
	FrameRef cursorFrame {SetAbsFrameIndex(MiscDataFrame, FlashIndex)};
	cursorRect.extent = GetFrameBounds(cursorFrame);
}

static void savePlanetLocationImage(void)
{
	GfxRect r;
	FrameRef cursorFrame = SetAbsFrameIndex(MiscDataFrame, FlashIndex);
	const HotSpot hs = GetFrameHot(cursorFrame);

	DestroyDrawable(ReleaseDrawable(eraseFrame));

	r = cursorRect;
	r.corner.x -= hs.x;
	r.corner.y -= hs.y;
	eraseFrame = CaptureDrawable(CopyContextRect(&r));
	SetFrameHot(eraseFrame, hs);
}

static void restorePlanetLocationImage(void)
{
	GfxStamp s;

	s.origin = cursorRect.corner;
	s.frame = eraseFrame; // saved image
	DrawStamp(&s);
}

static void drawPlanetCursor(bool filled)
{
	GfxStamp s;

	s.origin = cursorRect.corner;
	s.frame = SetAbsFrameIndex(MiscDataFrame, FlashIndex);
	if (filled)
	{
		DrawFilledStamp(&s);
	}
	else
	{
		DrawStamp(&s);
	}
}

static void setPlanetCursorLoc(GfxPoint new_pt)
{
	new_pt.x >>= MagShift;
	new_pt.y >>= MagShift;
	cursorRect.corner = new_pt;
}

static void setPlanetLoc(GfxPoint new_pt, bool restoreOld)
{
	planetLoc = new_pt;

	SetContext(ScanContext);
	if (restoreOld)
	{
		restorePlanetLocationImage();
	}
	setPlanetCursorLoc(new_pt);
	savePlanetLocationImage();
}

static constexpr auto FlashFrameDelay {c_gameSecond / 16};

static void flashPlanetLocation(void)
{
	static uint8_t c = 0x00;
	static int val = -2;
	static GfxPoint prevPt;
	static TimeCount NextTime {};
	bool locChanged {};
	const TimeCount Now = GetTimeCounter();

	locChanged = prevPt.x != cursorRect.corner.x
			  || prevPt.y != cursorRect.corner.y;

	if (!locChanged && Now < NextTime)
	{
		return; // nothing to do
	}

	if (locChanged)
	{ // Reset the flashing cycle
		c = 0x00;
		val = -2;
		prevPt = cursorRect.corner;

		NextTime = Now + FlashFrameDelay;
	}
	else
	{ // Continue the flashing cycle
		if (c == 0x00 || c == 0x1A)
		{
			val = -val;
		}
		c += val;

		if (Now - NextTime > FlashFrameDelay)
		{
			NextTime = Now + FlashFrameDelay;
		}
		// missed timing by too much
		else
		{
			NextTime += FlashFrameDelay;
		}
		// stable frame rate
	}

	SetContext(ScanContext);
	SetContextForeGroundColor(BuildColor(MakeRgb15(c, c, c), c));
	drawPlanetCursor(true);
}

void RedrawSurfaceScan(const GfxPoint* newLoc)
{
	GfxContextRef OldContext {SetContext(ScanContext)};

	BatchGraphics();
	DrawPlanet(0, BlackColor);
	DrawScannedObjects(true);
	if (newLoc)
	{
		setPlanetLoc(*newLoc, false);
		drawPlanetCursor(false);
	}
	UnbatchGraphics();

	SetContext(OldContext);
}

static uint16_t getLandingFuelNeeded(void)
{
	uint16_t fuel {};

	fuel = pSolarSysState->SysInfo.PlanetInfo.SurfaceGravity << 1;
	if (fuel > 3 * FuelTankScale)
	{
		fuel = 3 * FuelTankScale;
	}

	return fuel;
}

static void spawnFwiffo(void)
{
	ShipFragmentHandle hStarShip {};

	EncounterGroup = 0;
	PutGroupInfo(GroupsRandom, GroupSaveIp);
	ReinitQueue(&uqm::gameState().game().ip_group_q);
	assert(CountLinks(&uqm::gameState().game().npc_built_ship_q) == 0);

	hStarShip = CloneShipFragment(RaceId::Spathi,
								  &uqm::gameState().game().npc_built_ship_q, 1);
	if (hStarShip)
	{
		ShipFragment* StarShipPtr {};

		StarShipPtr = LockShipFrag(&uqm::gameState().game().npc_built_ship_q,
								   hStarShip);
		// Name Fwiffo
		StarShipPtr->captains_name_index = NameOffset + NumCaptainsNames;
		UnlockShipFrag(&uqm::gameState().game().npc_built_ship_q, hStarShip);
	}
}

// Returns true if the parent menu should remain
static bool DispatchLander(void)
{
	const int16_t landingFuel = getLandingFuelNeeded();

	EraseCoarseScan();

	// Deactivate planet rotation callback
	InputFrameCallback* oldCallback {SetInputCallback(nullptr)};

	if (!uqm::UQMOptions::read().infiniteFuel)
	{
		uqm::gameState().sis().FuelOnBoard -= landingFuel;

		DrawMineralHelpers();
		if (!uqm::UQMOptions::read().submenu)
		{
			DeltaSISGauges(0, UndefinedDelta, 0);
		}
	}

	SetContext(ScanContext);
	drawPlanetCursor(false);

	PlanetSide(planetLoc);
	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		return false;
	}

	if (uqm::gameState().flag(uqm::GameFlag::FOUND_PLUTO_SPATHI) == 1)
	{
		/* Create Fwiffo group and go into comm with it */
		spawnFwiffo();

		NextActivity |= ActivityFlag::CheckLoad; /* fake a load game */
		uqm::currentActivity() |= ActivityFlag::StartEncounter;
		SaveSolarSysLocation();

		return false;
	}

	if (uqm::usesPictograms(uqm::UQMOptions::read().whichCoarseScan))
	{
		PrintCoarseScan3DO();
	}
	else
	{
		PrintCoarseScanPC();
	}

	// Reactivate planet rotation callback
	SetInputCallback(oldCallback);

	return true;
}

struct PickPlanetState
{
	bool success;
	// true when player selected a location
};

static bool DoPickPlanetSide(MenuState* pMS)
{
	PickPlanetState* pickState = (PickPlanetState*)pMS->privData;
	const TimeCount TimeIn {GetTimeCounter()};
	bool select {};
	bool cancel {};

	select = PulsedInputState.menu[*MenuKey::MenuSelect];
	cancel = PulsedInputState.menu[*MenuKey::MenuCancel];

	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		pickState->success = false;
		return false;
	}

	if (cancel)
	{
		pickState->success = false;
		return false;
	}
	else if (select)
	{
		pickState->success = true;
		return false;
	}
	else
	{
		int16_t dx = 0;
		int16_t dy = 0;
		GfxPoint new_pt;
		static TimeCount tNext {};

		new_pt = planetLoc;

		if (tNext != TimeCount {} && TimeIn >= tNext)
		{
			tNext = TimeCount {};
		}

		if (tNext == TimeCount {} || TimeIn >= tNext)
		{
			if (CurrentInputState.menu[*MenuKey::MenuLeft])
			{
				dx = -ResScale(1);
			}
			if (CurrentInputState.menu[*MenuKey::MenuRight])
			{
				dx = ResScale(1);
			}
			if (CurrentInputState.menu[*MenuKey::MenuUp])
			{
				dy = -ResScale(1);
			}
			if (CurrentInputState.menu[*MenuKey::MenuDown])
			{
				dy = ResScale(1);
			}

			// Triple the cursor speed when the "Next" key is held down
			if (DirKeysPress() && CurrentInputState.menu[*MenuKey::MenuNext])
			{
				dx *= 3;
				dy *= 3;
			}
		}

		BatchGraphics();

		dx = dx << MagShift;
		if (dx)
		{
			new_pt.x += dx;
			if (new_pt.x < 0)
			{
				new_pt.x += (ScaledMapWidth() << MagShift);
			}
			else if (new_pt.x >= (ScaledMapWidth() << MagShift))
			{
				new_pt.x -= (ScaledMapWidth() << MagShift);
			}
		}
		dy = dy << MagShift;
		if (dy)
		{
			new_pt.y += dy;
			if (new_pt.y < 0 || new_pt.y >= (MapHeight() << MagShift))
			{
				new_pt.y = planetLoc.y;
			}
		}

		if (!pointsEqual(new_pt, planetLoc))
		{
			setPlanetLoc(new_pt, true);
		}

		flashPlanetLocation();

		UnbatchGraphics();

		if (dx || dy)
		{
			tNext = TimeIn + c_gameSecond / 40;
		}
	}

	return true;
}

static void drawLandingFuelUsage(uint16_t fuel)
{
	/* We need this so we can save the StatusMessageMode
	 * and fix it when we're done.
	 */
	StatMsgMode old_status_message_mode = StatMsgMode::Undefined;
	char buf[100] {};

	if (((int32_t)(uqm::gameState().sis().FuelOnBoard) - fuel)
		<= (int32_t)(get_fuel_to_sol()))
	{ // We will not have enough fuel to get to Sol if we dispatch the
		// lander
		old_status_message_mode = SetStatusMessageMode(StatMsgMode::Alert);
	}
	else if (((int32_t)(uqm::gameState().sis().FuelOnBoard) - fuel)
			 <= (int32_t)(get_fuel_to_sol() + (5 * FuelTankScale)))
	{ // We will have enough fuel to get to Sol if we dispatch the lander
		// but will have less than 5 to spare
		old_status_message_mode = SetStatusMessageMode(StatMsgMode::Warning);
	}

	fmt::format_to_n(buf, sizeof(buf) - 1, "{}{:1.1}", GameString(GameStringBase::NavigationString + 5), (float)fuel / FuelTankScale);
	DrawStatusMessage(buf);

	if (old_status_message_mode != StatMsgMode::Undefined)
	{
		SetStatusMessageMode(old_status_message_mode);
	}
}

static void eraseLandingFuelUsage(void)
{
	DrawStatusMessage(nullptr);
}

static bool PickPlanetSide(void)
{
	MenuState menuState;
	PickPlanetState PickState;
	const uint16_t fuel = getLandingFuelNeeded();
	bool retval = true;

	memset(&menuState, 0, sizeof menuState);
	menuState.privData = &PickState;

	if (is3DO(uqm::UQMOptions::read().landerStyle))
	{
		ClearSISRect(ClearSisRadar);
	}

	SetContext(ScanContext);
	BatchGraphics();
	DrawPlanet(0, BlackColor);
	DrawScannedObjects(false);
	UnbatchGraphics();

	drawLandingFuelUsage(fuel);
	// Set the current flash location
	setPlanetCursorLoc(planetLoc);
	savePlanetLocationImage();

	if (is3DO(uqm::UQMOptions::read().landerStyle))
	{
		InitLander(0);
		DrawRadarBorder();
	}

	SetMenuSounds(MenuSoundNone, MenuSoundSelect);

	PickState.success = false;
	menuState.InputFunc = DoPickPlanetSide;
	DoInput(&menuState, true);

	eraseLandingFuelUsage();
	if (PickState.success)
	{ // player chose a location
		retval = DispatchLander();
	}
	else
	{ // player bailed out
		restorePlanetLocationImage();
	}

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	return retval;
}

static constexpr auto NumFlashColors {8};

static void DrawScannedStuff(uint16_t y, uint16_t scan)
{
	ElementHandle hNextElement {};
	Color OldColor;

	OldColor = SetContextForeGroundColor(BlackColor);

	for (ElementHandle hElement {GetHeadElement()}; hElement; hElement = hNextElement)
	{
		Element* ElementPtr {};
		int16_t dy {};
		GfxStamp s;

		LockElement(hElement, &ElementPtr);
		hNextElement = GetSuccElement(ElementPtr);

		dy = y - ElementPtr->current.location.y;
		if (uqm::lowByte(ElementPtr->scan_node) != scan || dy < 0)
		{ // node of wrong type, or not time for it yet
			UnlockElement(hElement);
			continue;
		}

		// XXX: flag this as 'found' scanned object
		ElementPtr->state_flags |= Appearing;

		s.origin = ElementPtr->current.location;

		if (dy >= NumFlashColors)
		{ // flashing done for this node, draw normal
			s.frame = ElementPtr->next.image.frame;
			DrawStamp(&s);
		}
		else
		{
			uint8_t grad {};
			Color c = WhiteColor;
			uint16_t nodeSize {};

			// mineral -- white --> turquoise?? (contrasts with red)
			// energy -- white --> red (contrasts with white)
			// bio -- white --> violet (contrasts with green)
			grad = 0xff - 0xff * dy / (NumFlashColors - 1);
			switch (static_cast<ScanType>(scan))
			{
				case ScanType::Mineral:
					c.r = grad;
					break;
				case ScanType::Energy:
					c.g = grad;
					c.b = grad;
					break;
				case ScanType::Biological:
					c.g = grad;
					break;
				default:
					break;
			}

			SetContextForeGroundColor(c);

			// flash the node from the smallest size to node size
			// Get the node size for mineral, or number of transitions
			// for other scan types (was set by GeneratePlanetSide())
			nodeSize = GetFrameIndex(ElementPtr->next.image.frame)
					 - GetFrameIndex(ElementPtr->current.image.frame);
			if (dy > nodeSize)
			{
				dy = nodeSize;
			}

			s.frame =
				SetRelFrameIndex(ElementPtr->current.image.frame, dy);
			DrawFilledStamp(&s);
		}

		UnlockElement(hElement);
	}

	SetContextForeGroundColor(OldColor);
}

static void DrawPCScannedStuff(uint16_t scan)
{
	ElementHandle hNextElement {};
	TimePeriod interval;
	TimeCount now;
	Element* ElementPtr {};
	GfxStamp s;

	interval = c_gameSecond / 10;

	ElementHandle hElement {GetHeadElement()};
	now = GetTimeCounter() + interval;

	while (hElement && !AnyButtonPress(true))
	{
		if ((uqm::currentActivity() & ActivityFlag::CheckAbort))
		{
			return;
		}

		if (GetTimeCounter() >= now)
		{
			LockElement(hElement, &ElementPtr);
			hNextElement = GetSuccElement(ElementPtr);
			if (uqm::lowByte(ElementPtr->scan_node) != scan)
			{ // node of wrong type, or not time for it yet
				UnlockElement(hElement);
				hElement = hNextElement;
			}
			else
			{
				uint16_t nodeSize {};
				uint16_t diff {};

				uint16_t growth {0};
				ElementPtr->state_flags |= Appearing;
				s.origin = ElementPtr->current.location;
				now = GetTimeCounter() + TimePeriod {17};

				while (growth < NumFlashColors && !AnyButtonPress(true))
				{
					if (GetTimeCounter() >= now)
					{
						now = GetTimeCounter() + TimePeriod {17};
						diff = growth;
						nodeSize = GetFrameIndex(
									   ElementPtr->next.image.frame)
								 - GetFrameIndex(
									   ElementPtr->current.image.frame);
						if (diff > nodeSize)
						{
							diff = nodeSize;
						}

						s.frame = SetRelFrameIndex(
							ElementPtr->current.image.frame, diff);
						DrawStamp(&s);
						growth++;
					}
					RotatePlanetSphere(true, nullptr);
				}
				if (growth < NumFlashColors)
				{ // didn't finish - draw
					s.frame = ElementPtr->next.image.frame;
					DrawStamp(&s);
				}

				UnlockElement(hElement);
				now = GetTimeCounter() + interval;
				hElement = hNextElement;
			}
		}
		RotatePlanetSphere(true, nullptr);
	}
	if (hElement)
	{ // scan aborted - make everything scanned, workaround for singular
		// scan
		while (hElement)
		{
			LockElement(hElement, &ElementPtr);
			hNextElement = GetSuccElement(ElementPtr);
			if (uqm::lowByte(ElementPtr->scan_node) != scan)
			{ // node of wrong type, or not time for it yet
				UnlockElement(hElement);
				hElement = hNextElement;
			}
			else
			{
				ElementPtr->state_flags |= Appearing;
				UnlockElement(hElement);
				hElement = hNextElement;
			}
		}
	}
}

uint16_t callGenerateForScanType(const SolarSystemState* solarSys,
								 const SystemPlanet* world, uint16_t node, ScanType scanType,
								 SurfaceNode* info)
{
	switch (scanType)
	{
		case ScanType::Mineral:
			return (*solarSys->genFuncs->generateMinerals)(
				solarSys, world, node, info);
		case ScanType::Energy:
			return (*solarSys->genFuncs->generateEnergy)(
				solarSys, world, node, info);
		case ScanType::Biological:
			return (*solarSys->genFuncs->generateLife)(
				solarSys, world, node, info);
		default:
			break;
	}

	assert(false);
	return 0;
}

bool callPickupForScanType(SolarSystemState* solarSys, SystemPlanet* world,
						   uint16_t node, ScanType scanType)
{
	switch (scanType)
	{
		case ScanType::Mineral:
			return (*solarSys->genFuncs->pickupMinerals)(
				solarSys, world, node);
		case ScanType::Energy:
			return (*solarSys->genFuncs->pickupEnergy)(
				solarSys, world, node);
		case ScanType::Biological:
			return (*solarSys->genFuncs->pickupLife)(
				solarSys, world, node);
		default:
			break;
	}

	assert(false);
	return false;
}

static void ScanPlanet(uint16_t scanType)
{
	const auto ScanDuration = [&] {
		return ((GameTicksPerSecond * 7 / 4 + chooseIfHd(UINT8_MAX, 0ui8)));
	};
	// NumFlashColors for flashing blips; 1 for the final frame
	const auto ScanLinesOg = [&] {
		return (OriginalMapHeight() + NumFlashColors + 1);
	};
	const auto ScanLines = [&] {
		return ResScale(ScanLinesOg());
	};
	const auto ScanLineWait = [&] {
		return TimePeriod {ScanDuration()} / ScanLinesOg();
	};
	// For taming the scan FPS on underpowered devices
	const auto ScanLineFps = [&] {
		return (c_gameSecond / chooseIfHd(42, 60));
	};

	uint16_t startScan {};
	uint16_t endScan {};
	GfxRect r;
	const static Color textColors[] =
		{
			ScanMineralTextColor,
			ScanEnergyTextColor,
			ScanBiologicalTextColor,
		};
	const static Color tintColors[] =
		{
			ScanMineralTintColor,
			ScanEnergyTintColor,
			ScanBiologicalTintColor,
		};

	if (scanType == *ScanMenuItem::AutoScan)
	{
		startScan = *ScanType::Mineral;
		endScan = *ScanType::Biological;
	}
	else
	{
		startScan = scanType;
		endScan = scanType;
	}

	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		return;
	}

	DrawMenuStateStrings(MenuString::MinScan, scanType);

	for (uint16_t scan {startScan}; scan <= endScan; ++scan)
	{
		GfxText t;
		int16_t i = 0;
		Color tintColor;
		// Alpha value will be ignored.
		static TimeCount TimeOut;
		static TimeCount TimeOutDraw;
		TimeCount Now;

		t.baseline.x = ResScale(OrigSisScreenWidth() >> 1);
		t.baseline.y = SisScreenHeight() - MapHeight() - ResScale(7);
		t.align = TextAlign::Center;
		t.CharCount = (uint16_t)~0;

		t.pStr = GameString(GameStringBase::ScanString + scan);

		SetContext(PlanetContext);
		r.corner.x = 0;
		r.corner.y = t.baseline.y - ResScale(10);
		r.extent.width = SisScreenWidth();
		r.extent.height = t.baseline.y - r.corner.y + ResScale(1);
		RepairBackRect(&r);

		SetContextFont(MicroFont);
		SetContextForeGroundColor(textColors[scan]);
		font_DrawText(&t);

		SetContext(ScanContext);

		// Draw a virgin surface
		if (uqm::UQMOptions::read().scanStyle != uqm::EmulationMode::PC)
		{
			DrawPlanet(0, BlackColor);
		}

		tintColor = tintColors[scan];

		FlushInput();

		pSolarSysState->Orbit.scanType = static_cast<ScanType>(scan);
		RerenderPlanetSphere();

		if (uqm::UQMOptions::read().scanStyle != uqm::EmulationMode::PC)
		{
			while (i < ScanLines())
			{
				if ((uqm::currentActivity() & ActivityFlag::CheckAbort))
				{
					return;
				}

				Now = GetTimeCounter();
				if (Now >= TimeOut)
				{
					if (AnyButtonPress(true))
					{
						break;
					}

					TimeOut = Now + ScanLineWait();

					i += ResScale(1);

					if (Now >= TimeOutDraw)
					{
						TimeOutDraw = Now + ScanLineFps();

						BatchGraphics();
						DrawPlanet(i, tintColor);
						DrawScannedStuff(i, scan);
						UnbatchGraphics();
					}
				}
				RotatePlanetSphere(true, nullptr);
			}
		}
		else
		{
			DrawPCScanTint(scan);	  // palette-swap topo and Sphere map
			DrawPCScannedStuff(scan); // PC-style node pop-in

			if ((uqm::currentActivity() & ActivityFlag::CheckAbort))
			{
				return;
			}

			if (scanType == *ScanMenuItem::AutoScan)
			{ // delay between scans
				TimeOut = GetTimeCounter() + c_gameSecond;
				while (GetTimeCounter() < TimeOut
					   && !AnyButtonPress(true))
				{
					RotatePlanetSphere(true, nullptr);
				}
			}
			else
			{ // endless state - mimics PC "Exit Scan"
				while (!AnyButtonPress(true))
				{
					RotatePlanetSphere(true, nullptr);
				}
			}
		}

		if (i < ScanLines() && uqm::UQMOptions::read().scanStyle != uqm::EmulationMode::PC)
		{ // not for PC-scan, frame flashes otherwise
			// Aborted by a keypress; draw in finished state
			BatchGraphics();
			DrawPlanet(ScanLines() - 1, tintColor);
			DrawScannedStuff(ScanLines() - 1, scan);
			UnbatchGraphics();
		}
	}

	SetContext(PlanetContext);
	RepairBackRect(&r);

	SetContext(ScanContext);
	pSolarSysState->Orbit.scanType = ScanType::Count;

	if (uqm::UQMOptions::read().scanStyle == uqm::EmulationMode::PC || useDosSpheres)
	{
		RerenderPlanetSphere();
	}

	if (scanType == *ScanMenuItem::AutoScan || uqm::UQMOptions::read().scanStyle == uqm::EmulationMode::PC)
	{ // clear the last scan
		DrawPlanet(0, BlackColor);
		DrawDefaultPlanetSphere();
		DrawScannedObjects(false);
	}

	FlushInput();
}

static bool DoScan(MenuState* pMS)
{
	bool select {};
	bool cancel {};

	select = PulsedInputState.menu[*MenuKey::MenuSelect];
	cancel = PulsedInputState.menu[*MenuKey::MenuCancel];

	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		return false;
	}

	if (cancel || (select && pMS->CurState == ScanMenuItem::ExitScan))
	{
		return false;
	}
	else if (select)
	{
		if (pMS->CurState == ScanMenuItem::DispatchShuttle)
		{
			if (pSolarSysState->pOrbitalDesc->data_index & *PlanetType::PlanetShielded
				|| pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == GasGiantAtmosphere)
			{ // cannot dispatch to shielded planets or gas giants
				PlayMenuSound(MenuSoundEffect::Failure);
				return true;
			}

			const uint16_t fuel_required {getLandingFuelNeeded()};
			if (uqm::gameState().sis().FuelOnBoard < fuel_required
				|| uqm::gameState().sis().NumLanders == 0
				|| uqm::gameState().sis().CrewEnlisted == 0)
			{
				PlayMenuSound(MenuSoundEffect::Failure);
				return true;
			}

			SetFlashRect(nullptr, false);
			DrawMenuStateStrings(MenuString::MinScan, pMS->CurState);

			if (!PickPlanetSide())
			{
				return false;
			}

			DrawMenuStateStrings(MenuString::MinScan, pMS->CurState);
			SetFlashRect(SfrMenu3do(), false);

			return true;
		}

		// Various scans
		if (pSolarSysState->pOrbitalDesc->data_index & *PlanetType::PlanetShielded)
		{ // cannot scan shielded planets
			PlayMenuSound(MenuSoundEffect::Failure);
			return true;
		}

		ScanPlanet(pMS->CurState);
		if (pMS->CurState == ScanMenuItem::AutoScan)
		{
			pMS->CurState = *ScanMenuItem::DispatchShuttle;
		}
		DrawMenuStateStrings(MenuString::MinScan, pMS->CurState);
	}
	else if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::PC || (!(pSolarSysState->pOrbitalDesc->data_index & *PlanetType::PlanetShielded) && pSolarSysState->SysInfo.PlanetInfo.AtmoDensity != GasGiantAtmosphere))
	{
		DoMenuChooser(pMS, MenuString::MinScan);
	}

	return true;
}

static GfxContextRef CreateScanContext(bool inSpace)
{
	GfxRect r;
	int16_t x_offset {};

	// ScanContext rect is relative to SpaceContext
	GfxContextRef oldContext {SetContext(SpaceContext)};
	GetContextClipRect(&r);

	GfxContextRef context {CreateContext("ScanContext")};
	SetContext(context);
	SetContextFGFrame(Screen);
	x_offset = ResDescale(r.extent.width - ScaledMapWidth());
	r.extent.width = ScaledMapWidth();
	if (inSpace && x_offset > 0)
	{
		x_offset >>= 1;
		x_offset += 2;
		r.extent.width -= ResScale(5);
	}
	r.corner.x += ResScale(x_offset);
	r.corner.y += r.extent.height - MapHeight();
	r.extent.height = MapHeight();

	SetContextClipRect(&r);

	SetContext(oldContext);

	return context;
}

GfxContextRef GetScanContext(bool* owner)
{
	// TODO: Make GfxContextRef ref-counted
	if (ScanContext)
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
			ScanContext = CreateScanContext(true);
		}
		else
		{
			ScanContext = CreateScanContext(false);
		}
	}
	return ScanContext;
}

void DestroyScanContext(void)
{
	if (ScanContext)
	{
		DestroyContext(ScanContext);
		ScanContext = nullptr;
	}
}

void ScanSystem(void)
{
	MenuState menuState;

	memset(&menuState, 0, sizeof menuState);

	// fmt::printWorld (pSolarSysState->pOrbitalDesc);

	GetScanContext(nullptr);

	if (uqm::UQMOptions::read().whichMenu == uqm::EmulationMode::Console3DO && ((pSolarSysState->pOrbitalDesc->data_index & *PlanetType::PlanetShielded) || pSolarSysState->SysInfo.PlanetInfo.AtmoDensity == GasGiantAtmosphere))
	{
		menuState.CurState = *ScanMenuItem::ExitScan;
	}
	else
	{
		menuState.CurState = *ScanMenuItem::AutoScan;
		planetLoc.x = (ScaledMapWidth() >> 1) << MagShift;
		planetLoc.y = (MapHeight() >> 1) << MagShift;

		initPlanetLocationImage();
		SetContext(ScanContext);
		DrawScannedObjects(false);
	}

	DrawMenuStateStrings(MenuString::MinScan, menuState.CurState);

	SetFlashRect(SfrMenu3do(), false);

	if (uqm::usesPictograms(uqm::UQMOptions::read().whichCoarseScan))
	{
		PrintCoarseScan3DO();
	}
	else
	{
		PrintCoarseScanPC();
	}

	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);

	menuState.InputFunc = DoScan;

	DoInput(&menuState, false);

	SetFlashRect(nullptr, false);

	// cleanup scan graphics
	if (!(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		BatchGraphics();
		SetContext(ScanContext);
		DrawPlanet(0, BlackColor);

		EraseCoarseScan();
		UnbatchGraphics();
	}

	DestroyDrawable(ReleaseDrawable(eraseFrame));
	eraseFrame = nullptr;
}

static void generateBioNode(SolarSystemState* system, Element* NodeElementPtr,
							uint8_t* life_init_tab, uint16_t creatureType)
{
	uint16_t i {};
	uint32_t j {};

	if (isDifficulty(uqm::Difficulty::Hard))
	{
		CreatureData[EvilOne].Attributes =
			BehaviorHunt | AwarenessHigh | SpeedSlow | DangerMonstrous;
		CreatureData[EvilOne].ValueAndHitPoints = uqm::makeByte(5, 5);
		// CreatureData[ZexBeauty].Attributes =
		//		BehaviorHunt | AwarenessHigh | SpeedFast |
		//			DangerMonstrous;
		// CreatureData[ZexBeauty].ValueAndHitPoints = MAKE_BYTE(15, 30);
	}
	else if (isDifficulty(uqm::Difficulty::Easy))
	{
		CreatureData[ZexBeauty].Attributes =
			BehaviorHunt | AwarenessLow | SpeedSlow | DangerMonstrous;
		CreatureData[ZexBeauty].ValueAndHitPoints = uqm::makeByte(15, 8);
	}

	// NOTE: uqm::random().next() calls here are NOT part of the deterministic
	// planet generation PRNG flow.
	if (CreatureData[creatureType].Attributes & SpeedMask)
	{
		// Place moving creatures at a random location.
		i = uqm::random().next();
		j = (uint32_t)uqm::random().next();

		NodeElementPtr->current.location.x =
			((chooseIfHd(static_cast<uint16_t>(uqm::lowByte(i)), uqm::lowWord(j)) % (ScaledMapWidth() - (8 << 1))) + 8);
		NodeElementPtr->current.location.y =
			(chooseIfHd(static_cast<uint16_t>(uqm::highByte(i)), uqm::highWord(j)) % (MapHeight() - (8 << 1))) + 8;
	}

	if (system->PlanetSideFrame[0] == 0)
	{
		system->PlanetSideFrame[0] =
			CaptureDrawable(LoadGraphic(CannisterMaskPmapAnimRes));
	}

	for (i = 0; i < MaxLifeVariation
				&& life_init_tab[i] != (uint8_t)(creatureType + 1);
		 ++i)
	{
		if (life_init_tab[i] != 0)
		{
			continue;
		}

		life_init_tab[i] = (uint8_t)creatureType + 1;

		system->PlanetSideFrame[i + 3] = load_life_form(creatureType);
		break;
	}

	NodeElementPtr->mass_points = (uint8_t)creatureType;
	NodeElementPtr->hit_points = uqm::highNibble(
		CreatureData[creatureType].ValueAndHitPoints);
	DisplayArray[NodeElementPtr->PrimIndex].Object.Stamp.frame = SetAbsFrameIndex(
		system->PlanetSideFrame[i + 3], (uint16_t)uqm::random().next());
}

void GeneratePlanetSide(void)
{
	uint8_t life_init_tab[MaxLifeVariation];
	// life_init_tab is filled with the creature types of already
	// selected creatures. If an entry is 0, none has been selected
	// yet, otherwise, it is 1 more than the creature type.

	InitDisplayList();
	if (pSolarSysState->pOrbitalDesc->data_index & *PlanetType::PlanetShielded)
	{
		return;
	}

	memset(life_init_tab, 0, sizeof life_init_tab);

	for (ScanType scan {ScanType::Biological}; scan >= ScanType::Mineral; --scan)
	{
		uint16_t num_nodes {};
		FrameRef f {};

		f = SetAbsFrameIndex(MiscDataFrame,
							 NumScandotTransitions * (scan - ScanType::Energy));

		num_nodes = callGenerateForScanType(pSolarSysState,
											pSolarSysState->pOrbitalDesc, GenerateAll, scan, nullptr);

		while (num_nodes--)
		{
			Element* NodeElementPtr {};
			SurfaceNode info;

			if (isNodeRetrieved(&pSolarSysState->SysInfo.PlanetInfo,
								scan, num_nodes))
			{
				continue;
			}

			ElementHandle hNodeElement {AllocElement()};
			if (!hNodeElement)
			{
				continue;
			}

			LockElement(hNodeElement, &NodeElementPtr);

			callGenerateForScanType(pSolarSysState,
									pSolarSysState->pOrbitalDesc, num_nodes,
									scan, &info);

			NodeElementPtr->scan_node = uqm::makeWord(*scan, num_nodes + 1);
			NodeElementPtr->playerNr = *PlanetsidePlayer::NonPlayer;
			NodeElementPtr->current.location.x = info.loc_pt.x;
			NodeElementPtr->current.location.y = info.loc_pt.y;

			SetPrimType(&DisplayArray[NodeElementPtr->PrimIndex],
						PrimitiveType::Stamp);
			if (scan == ScanType::Mineral)
			{
				NodeElementPtr->turn_wait = info.type;

				// JMS: Partially scavenged energy blips won't return
				// anymore to original size after leaving planet.
				NodeElementPtr->mass_points = uqm::highByte(info.density)
											- pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[*scan][num_nodes];

				NodeElementPtr->current.image.frame = SetAbsFrameIndex(
					MiscDataFrame, (NumScandotTransitions * 2)
									   + ElementCategory(info.type) * 5);
				NodeElementPtr->next.image.frame = SetRelFrameIndex(
					NodeElementPtr->current.image.frame,
					uqm::lowByte(info.density) + 1);
				DisplayArray[NodeElementPtr->PrimIndex].Object.Stamp.frame =
					IncFrameIndex(NodeElementPtr->next.image.frame);
			}
			else /* (scan == BIOLOGICAL_SCAN || scan == ENERGY_SCAN) */
			{
				NodeElementPtr->current.image.frame = f;
				NodeElementPtr->next.image.frame = SetRelFrameIndex(
					f, NumScandotTransitions - 1);
				NodeElementPtr->turn_wait = uqm::makeByte(4, 4);
				NodeElementPtr->preprocess_func = object_animation;
				if (scan == ScanType::Energy)
				{
					NodeElementPtr->mass_points = MaxScrounged();
					DisplayArray[NodeElementPtr->PrimIndex].Object.Stamp.frame = pSolarSysState->PlanetSideFrame[1];
				}
				else /* (scan == BIOLOGICAL_SCAN) */
				{
					generateBioNode(pSolarSysState, NodeElementPtr,
									life_init_tab, info.type);
				}
			}

			NodeElementPtr->next.location.x =
				NodeElementPtr->current.location.x << MagShift;
			NodeElementPtr->next.location.y =
				NodeElementPtr->current.location.y << MagShift;
			UnlockElement(hNodeElement);

			PutElement(hNodeElement);
		}
	}
}

bool isNodeRetrieved(PlanetData* planetInfo, ScanType scanType, uint8_t nodeNr)
{
	return (planetInfo->ScanRetrieveMask[*scanType] & ((uint32_t)1 << nodeNr))
		!= 0;
}

uint16_t countNodesRetrieved(PlanetData* planetInfo, ScanType scanType)
{
	uint16_t count {};
	uint32_t mask = planetInfo->ScanRetrieveMask[*scanType];

	// count the number of bits set
	// Caution: 'mask' must be unsigned
	for (count = 0; mask != 0; mask >>= 1)
	{
		if (mask & 1)
		{
			++count;
		}
	}
	return count;
}

void setNodeRetrieved(PlanetData* planetInfo, ScanType scanType, uint8_t nodeNr)
{
	planetInfo->ScanRetrieveMask[*scanType] |= ((uint32_t)1 << nodeNr);
}

void setNodeNotRetrieved(PlanetData* planetInfo, ScanType scanType, uint8_t nodeNr)
{
	planetInfo->ScanRetrieveMask[*scanType] &= ~((uint32_t)1 << nodeNr);
}

} // namespace uqm
