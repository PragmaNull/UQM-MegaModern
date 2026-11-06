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

#include "planets.h"

#include "core/string/StringUtils.h"

#include "scan.h"
#include "lander.h"
#include "../colors.h"
#include "../element.h"
#include "../settings.h"
#include "../controls.h"
#include "../sounds.h"
#include "../gameopt.h"
#include "../shipcont.h"
#include "../setup.h"
#include "../uqmdebug.h"
#include "../resinst.h"
#include "../nameref.h"
#include "../starmap.h"
#include "../util.h"
#include "options.h"
#include "libs/graphics/gfx_common.h"
#include "../gendef.h"
#include "uqm/gamestr.h"


// PlanetOrbitMenu() items
enum PlanetMenuItems
{
	// XXX: Must match the enum in menustat.h
	SCAN = 0,
	STARMAP,
	EQUIP_DEVICE,
	CARGO,
	ROSTER,
	GAME_MENU,
	NAVIGATION,
};

GFXCONTEXT PlanetContext;
// Context for rotating planet view and lander surface view

bool useDosSpheres = false;
bool use3DOSpheres = false;
uint8_t OrbitNum = 0;

void DestroyOrbitStruct(PlanetOrbit* Orbit, int16_t height)
{
	DestroyDrawable(ReleaseDrawable(Orbit->TopoZoomFrame));
	Orbit->TopoZoomFrame = 0;

	if (Orbit->lpTopoData)
	{
		delete[] Orbit->lpTopoData;
		Orbit->lpTopoData = nullptr;
	}

	DestroyDrawable(ReleaseDrawable(Orbit->SphereFrame));
	Orbit->SphereFrame = 0;

	DestroyDrawable(ReleaseDrawable(Orbit->ObjectFrame));
	Orbit->ObjectFrame = 0;

	DestroyDrawable(ReleaseDrawable(Orbit->TintFrame));
	Orbit->TintFrame = 0;

	Orbit->TintColor = BlackColor;

	if (Orbit->TopoColors)
	{
		delete[] Orbit->TopoColors;
		Orbit->TopoColors = nullptr;
	}

	if (Orbit->ScanColors)
	{
		for (uint16_t i {0}; i < NUM_SCAN_TYPES; i++)
		{
			delete[] Orbit->ScanColors[i];
			Orbit->ScanColors[i] = nullptr;
		}
		delete[] Orbit->ScanColors;
		Orbit->ScanColors = nullptr;
	}

	if (Orbit->ScratchArray)
	{
		delete[] Orbit->ScratchArray;
		Orbit->ScratchArray = nullptr;
	}

	DestroyDrawable(ReleaseDrawable(Orbit->WorkFrame));
	Orbit->WorkFrame = 0;

	DestroyDrawable(ReleaseDrawable(Orbit->BackFrame));
	Orbit->BackFrame = 0;

	if (Orbit->light_diff)
	{
		for (uint16_t j {0}; j < height; j++)
		{
			delete[] Orbit->light_diff[j];
			Orbit->light_diff[j] = nullptr;
		}
		delete[] Orbit->light_diff;
		Orbit->light_diff = nullptr;
	}

	if (Orbit->map_rotate)
	{
		for (uint16_t k {0}; k < height; k++)
		{
			delete[] Orbit->map_rotate[k];
			Orbit->map_rotate[k] = nullptr;
		}
		delete[] Orbit->map_rotate;
		Orbit->map_rotate = nullptr;
	}

	DestroyDrawable(ReleaseDrawable(Orbit->TopoMask));
	Orbit->TopoMask = 0;

	if (Orbit->sphereBytes)
	{
		delete[] Orbit->sphereBytes;
		Orbit->sphereBytes = nullptr;
	}

	delete Orbit->sphereMap;
	Orbit->sphereMap = 0;

	Orbit->scanType = 0;

	DestroyDrawable(ReleaseDrawable(Orbit->Shade));
	Orbit->Shade = 0;

	if (Orbit->ShadeColors)
	{
		delete[] Orbit->ShadeColors;
		Orbit->ShadeColors = nullptr;
	}
}


static void CreatePlanetContext(void)
{
	GfxRect r;

	assert(PlanetContext == nullptr);

	// PlanetContext rect is relative to SpaceContext
	GFXCONTEXT oldContext {SetContext(SpaceContext)};
	GetContextClipRect(&r);

	PlanetContext = CreateContext("PlanetContext");
	SetContext(PlanetContext);
	SetContextFGFrame(Screen);
	r.extent.height -= MapHeight() + MapBorderHeight();
	SetContextClipRect(&r);

	SetContext(oldContext);
}

static void DestroyPlanetContext(void)
{
	if (PlanetContext)
	{
		DestroyContext(PlanetContext);
		PlanetContext = nullptr;
	}
}

void DrawScannedObjects(bool Reversed)
{
	HELEMENT hElement {};
	HELEMENT hNextElement {};

	for (hElement = Reversed ? GetTailElement() : GetHeadElement();
		 hElement; hElement = hNextElement)
	{
		Element* ElementPtr {};

		LockElement(hElement, &ElementPtr);
		hNextElement = Reversed ?
						   GetPredElement(ElementPtr) :
						   GetSuccElement(ElementPtr);

		if (ElementPtr->state_flags & Appearing)
		{
			GfxStamp s;

			s.origin = ElementPtr->current.location;
			s.frame = ElementPtr->next.image.frame;
			DrawStamp(&s);
		}

		UnlockElement(hElement);
	}
}

void DrawPlanetSurfaceBorder(void)
{
	GfxRect oldClipRect;
	GfxRect clipRect;
	GfxRect r;

	GFXCONTEXT oldContext {SetContext(SpaceContext)};
	GetContextClipRect(&oldClipRect);

	// Expand the context clip-rect so that we can tweak the existing border
	clipRect = oldClipRect;
	clipRect.corner.x -= ResScale(1);
	clipRect.extent.width += ResScale(2);
	clipRect.extent.height += ResScale(1);
	SetContextClipRect(&clipRect);

	BatchGraphics();

	// Border bulk
	SetContextForeGroundColor(
		BuildColor(MakeRgb15(0x0A, 0x0A, 0x0A), 0x08));
	r.corner.x = 0;
	r.corner.y = clipRect.extent.height - MapHeight() - MapBorderHeight();
	r.extent.width = clipRect.extent.width;
	r.extent.height = MapBorderHeight() - ResScale(2);
	DrawFilledRectangle(&r);

	SetContextForeGroundColor(SisBottomRightBorderColor);

	// Border top shadow line
	r.extent.width -= ResScale(1);
	r.extent.height = ResScale(1);
	r.corner.x = ResScale(1);
	r.corner.y -= ResScale(1);
	DrawFilledRectangle(&r);

	// XXX: We will need bulk left and right rects here if MapWidth() changes

	// Right shadow line
	r.extent.width = ResScale(1);
	r.extent.height = MapHeight() + ResScale(2);
	r.corner.y += MapBorderHeight() - ResScale(1);
	r.corner.x = clipRect.extent.width - ResScale(1);
	DrawFilledRectangle(&r);

	SetContextForeGroundColor(SisLeftBorderColor);

	// Left shadow line
	r.corner.x -= MapWidth() + ResScale(1);
	DrawFilledRectangle(&r);

	// Border bottom shadow line
	r.extent.width = MapWidth() + ResScale(2);
	r.extent.height = ResScale(1);
	DrawFilledRectangle(&r);

	if (isPC(uqm::UQMOptions::read().landerStyle))
	{
		r.corner.x = ResScale(UqmMapWidth - Sc2MapWidth)
				   - SisOrgX() + ResScale(1) + SafePos(1);
		r.corner.y = clipRect.extent.height - MapHeight() - ResScale(1);
		r.extent.width = ResScale(1);
		r.extent.height = MapHeight();
		SetContextForeGroundColor(SisBottomRightBorderColor);
		DrawFilledRectangle(&r);
		r.corner.x += ResScale(1);
		r.extent.width = ResScale(4);
		r.corner.y -= ResScale(1);
		r.extent.height += ResScale(2);
		SetContextForeGroundColor(
			BuildColorRgba(0x52, 0x52, 0x52, 0xFF));
		DrawFilledRectangle(&r);
		r.corner.x += ResScale(4);
		r.extent.width = ResScale(1);
		r.corner.y += ResScale(1);
		r.extent.height -= ResScale(2);
		SetContextForeGroundColor(SisLeftBorderColor);
		DrawFilledRectangle(&r);

		DrawBorder(LANDER_DOS_FRAME);
	}
	else
	{
		DrawBorder(LANDER_3DO_FRAME);
	}

	UnbatchGraphics();

	SetContextClipRect(&oldClipRect);
	SetContext(oldContext);
}

typedef enum
{
	DRAW_ORBITAL_FULL,
	DRAW_ORBITAL_WAIT,
	DRAW_ORBITAL_UPDATE,
	DRAW_ORBITAL_FROM_STARMAP,

} DRAW_ORBITAL_MODE;

static void DrawEnterOrbitText(GfxRect rect)
{
	GfxText text;
	FRAME OldFontEffect {};
	int16_t leading {};
	char buf[256];

	FONT OldFont {SetContextFont(MicroFont)};
	OldFontEffect = SetContextFontEffect(
		SetAbsFrameIndex(FontGradFrame, 12));

	GetContextFontLeading(&leading);

	text.baseline = rect.corner;
	text.baseline.x += rect.extent.width >> 1;
	text.baseline.y += ResScale(4) + leading - ResScale(4);
	text.align = ALIGN_CENTER;

	const int16_t og_baseline_x {text.baseline.x};

	uqm::utf8StringCopy((char*)buf, sizeof(buf),
						GameString(NAVIGATION_STRING_BASE + 8));

	text.align = ALIGN_CENTER;
	{
		uqstl::vector<uqstl::string> tokens;
		uqm::tokenize(uqstl::string_view {buf}, tokens, '\n', false);
		for (const auto& tok : tokens)
		{
			text.pStr = AlignText((const char*)tok.c_str(),
								  &text.baseline.x);
			text.CharCount = (uint16_t)~0;

			font_DrawText(&text);

			text.baseline.y += leading;
			text.baseline.x = og_baseline_x;
		}
	}

	SetContextFont(OldFont);
	SetContextFontEffect(OldFontEffect);
}

void DrawOrbitMapGraphic(void)
{
	GfxStamp s;

	SetContext(GetScanContext(nullptr));

	if (uqm::UQMOptions::read().sphereType != 1)
	{
		const bool HaveString =
			strlen(GameString(NAVIGATION_STRING_BASE + 8)) > 0;

		s.frame = SetAbsFrameIndex(CaptureDrawable(
									   LoadGraphic(OrbenterPmapAnimRes)),
								   HaveString);

		s.origin.x = -SafeX();
		s.origin.y = 0;

		if (isPC(uqm::UQMOptions::read().landerStyle))
		{
			s.origin.x -=
				ResScale(((UqmMapWidth - Sc2MapWidth) / 2)
						 + SafeNum(5));
		}

		DrawStamp(&s);

		if (HaveString)
		{
			GfxRect rect;

			GetFrameRect(s.frame, &rect);
			rect.corner.x += s.origin.x;
			DrawEnterOrbitText(rect);
		}

		DestroyDrawable(ReleaseDrawable(s.frame));
	}
	else
	{
		DrawPlanet(0, BlackColor);
		DestroyDrawable(ReleaseDrawable(pSolarSysState->TopoFrame));
		pSolarSysState->TopoFrame = 0;
	}
#if 0
	if (never)
	{
		GfxStamp ss;
		SystemPlanet *pPlanetDesc;
		PlanetOrbit *Orbit = &pSolarSysState->Orbit;
		int PlanetScale = chooseIfHd (319, 512);
		int PlanetRescale = 1275;

		pPlanetDesc = pSolarSysState->pOrbitalDesc;
		GeneratePlanetSurface (
			pPlanetDesc, nullptr, PlanetScale, PlanetScale);
		ss.origin.x = ResScale (OrigSisScreenWidth() >> 1);
		ss.origin.y = ResScale (191);

		ss.frame = chooseIfHd (Orbit->SphereFrame, CaptureDrawable (
			RescaleFrame (
				Orbit->SphereFrame, PlanetRescale, PlanetRescale
			)));

		DrawStamp (&ss);
		DestroyDrawable (ReleaseDrawable (ss.frame));
	}
#endif
}

static void DrawOrbitalDisplay(DRAW_ORBITAL_MODE Mode)
{
	GfxRect r;

	SetContext(SpaceContext);
	GetContextClipRect(&r);

	BatchGraphics();

	if (Mode != DRAW_ORBITAL_UPDATE)
	{
		SetTransitionSource(nullptr);
		DrawSISFrame();
		DrawSISMessage(nullptr);
		DrawSISTitle(uqm::gameState().sis().PlanetName);
		DrawStarBackGround();
		DrawPlanetSurfaceBorder();
	}

	if (Mode == DRAW_ORBITAL_WAIT)
	{
		DrawOrbitMapGraphic();

		if (isPC(uqm::UQMOptions::read().landerStyle))
		{
			InitPCLander(true);
		}
	}
	else if (Mode == DRAW_ORBITAL_FULL)
	{
		DrawDefaultPlanetSphere();
		DrawMenuStateStrings(PM_SCAN, SCAN);
	}
	else if (Mode == DRAW_ORBITAL_FROM_STARMAP)
	{
		DrawDefaultPlanetSphere();
		DrawMenuStateStrings(PM_SCAN, STARMAP);
	}
	else
	{
		DrawMenuStateStrings(PM_SCAN, SCAN);
	}

	if (Mode != DRAW_ORBITAL_WAIT)
	{
		SetContext(GetScanContext(nullptr));
		DrawPlanet(0, BlackColor);
		if (isPC(uqm::UQMOptions::read().landerStyle))
		{
			InitPCLander(false);
		}
	}

	if (Mode != DRAW_ORBITAL_UPDATE)
	{
		ScreenTransition(uqm::UQMOptions::read().scrTrans, &r);
	}

	UnbatchGraphics();

	// for later RepairBackRect()

	LoadIntoExtraScreen(&r);
}

// Initialise the surface graphics, and start the planet music.
// Called from the GenerateFunctions.generateOribital() function
// (when orbit is entered; either from IP, or from loading a saved game)
// and when "starmap" is selected from orbit and then cancelled;
// also after in-orbit comm and after defeating planet guards in combat.
// SurfDefFrame contains surface definition images when a planet comes
// with its own bitmap (currently only for Earth)
void LoadPlanet(FRAME SurfDefFrame)
{
	const bool WaitMode = !(LastActivity & CHECK_LOAD);
	TimeCount sleep;

#ifdef DEBUG
	if (disableInteractivity)
	{
		return;
	}
#endif

	assert(pSolarSysState->InOrbit && !pSolarSysState->TopoFrame);

	CreatePlanetContext();

	StopMusic();

	sleep = GetTimeCounter() + (c_gameSecond * 6 / 5);
	SystemPlanet* pPlanetDesc {pSolarSysState->pOrbitalDesc};

	if (WaitMode)
	{
		if (uqm::UQMOptions::read().sphereType == 1)
		{
			GetPlanetTopography(pPlanetDesc, SurfDefFrame);
		}
		DrawOrbitalDisplay(DRAW_ORBITAL_WAIT);
	}

	GeneratePlanetSurface(pPlanetDesc, SurfDefFrame, 0, 0);
	OrbitNum = SetPlanetMusic(pPlanetDesc->data_index & ~PLANET_SHIELDED);
	GeneratePlanetSide();
	MaskLanderGraphics();

	if (isPC(uqm::UQMOptions::read().scrTrans))
	{
		SleepThreadUntil(sleep);
	}

	if (!PLRPlaying((MUSIC_REF)~0))
	{
		PlayMusicResume(LanderMusic, NormalVolume);
	}

	if (WaitMode)
	{
		if (is3DO(uqm::UQMOptions::read().scrTrans) || uqm::UQMOptions::read().sphereType == 1)
		{
			ZoomInPlanetSphere();
		}
		DrawOrbitalDisplay(DRAW_ORBITAL_UPDATE);
	}
	else
	{
		DrawOrbitalDisplay(DRAW_ORBITAL_FULL);
	}
}

void FreePlanet(void)
{
	PlanetOrbit* Orbit = &pSolarSysState->Orbit;

	UninitSphereRotation();

	SetMusicPosition();

	StopMusic();

	for (uint16_t i {0}; i < std::size(pSolarSysState->PlanetSideFrame); ++i)
	{
		DestroyDrawable(
			ReleaseDrawable(pSolarSysState->PlanetSideFrame[i]));
		pSolarSysState->PlanetSideFrame[i] = 0;
	}

	//    FreeLanderData ();

	pSolarSysState->XlatTable.reset();
	DestroyDrawable(ReleaseDrawable(pSolarSysState->TopoFrame));
	pSolarSysState->TopoFrame = 0;

	if (uqm::UQMOptions::read().scanStyle == uqm::EmulationMode::PC)
	{
		for (uint16_t k {0}; k < NUM_SCAN_TYPES; k++)
		{
			DestroyDrawable(
				ReleaseDrawable(pSolarSysState->ScanFrame[k]));
			pSolarSysState->ScanFrame[k] = 0;
		}
	}

	pSolarSysState->OrbitalCMap.reset();

	DestroyOrbitStruct(Orbit, MapHeight());

	pSolarSysState->SysInfo.PlanetInfo.DiscoveryStrings.reset();
	FreeLanderFont(&pSolarSysState->SysInfo.PlanetInfo);

	// Need to make sure our own CONTEXTs are not active because
	// we will destroy them now
	SetContext(SpaceContext);
	DestroyPlanetContext();
	DestroyScanContext();
	DestroyPCLanderContext();
	useDosSpheres = false;
	use3DOSpheres = false;
}

void LoadStdLanderFont(PlanetData* info)
{
	info->LanderFont = LoadFont(LanderFontRes);
	info->LanderFontEff = CaptureDrawable(
		LoadGraphic(LanderFonteffPmapAnimRes));
}

void FreeLanderFont(PlanetData* info)
{
	DestroyFont(info->LanderFont);
	info->LanderFont = nullptr;
	DestroyDrawable(ReleaseDrawable(info->LanderFontEff));
	info->LanderFontEff = nullptr;
}

static bool DoPlanetOrbit(MenuState* pMS)
{
	bool select = (bool)PulsedInputState.menu[KEY_MENU_SELECT];

	if ((uqm::currentActivity() & (CHECK_ABORT | CHECK_LOAD))
		|| uqm::gameState().sis().CrewEnlisted == (uint16_t)~0)
	{
		return false;
	}

	// XXX: pMS actually refers to pSolarSysState->menuState
	const bool handled {DoMenuChooser(pMS, PM_SCAN)};
	if (handled)
	{
		return true;
	}

	if (!select)
	{
		return true;
	}

	SetFlashRect(nullptr, false);

	switch (pMS->CurState)
	{
		case SCAN:
			ScanSystem();
			if (uqm::currentActivity() & START_ENCOUNTER)
			{ // Found Fwiffo on Pluto
				return false;
			}
			break;
		case EQUIP_DEVICE:
			select = DevicesMenu();
			if (uqm::currentActivity() & START_ENCOUNTER)
			{ // Invoked Talking Pet, a Caster or Sun Device over Chmmr,
				// or a Caster for Ilwrath
				// Going into conversation
				return false;
			}
			break;
		case CARGO:
			CargoMenu();
			break;
		case ROSTER:
			select = RosterMenu();
			break;
		case GAME_MENU:
			if (!GameOptions())
			{
				return false; // abort or load
			}
			break;
		case STARMAP:
			{
				// Deactivate planet rotation
				InputFrameCallback* oldCallback {SetInputCallback(nullptr)};

				RepairSISBorder();

				const bool AutoPilotSet {StarMap()};
				if (uqm::currentActivity() & CHECK_ABORT)
				{
					return false;
				}

				// Reactivate planet rotation
				SetInputCallback(oldCallback);

				if (!AutoPilotSet)
				{												   // Redraw the orbital display
					DrawOrbitalDisplay(DRAW_ORBITAL_FROM_STARMAP); //WAS FULL
					break;
				}
				FALLTHROUGH; // Fall through !!!
			}
		case NAVIGATION:
			return false;
	}

	if (!(uqm::currentActivity() & CHECK_ABORT))
	{
		if (select)
		{ // 3DO menu jumps to NAVIGATE after a successful submenu run
			if (uqm::UQMOptions::read().whichMenu != uqm::EmulationMode::PC)
			{
				pMS->CurState = NAVIGATION;
			}
			if (pMS->CurState != STARMAP)
			{
				DrawMenuStateStrings(PM_SCAN, pMS->CurState);
			}
		}
		SetFlashRect(SfrMenu3do(), false);
	}

	return true;
}

static void on_input_frame(void)
{
	if (!(uqm::currentActivity() & CHECK_ABORT))
	{
		RotatePlanetSphere(true, nullptr);
	}
}

void PlanetOrbitMenu(void)
{
	MenuState menuState;

	memset(&menuState, 0, sizeof menuState);

	SetFlashRect(SfrMenu3do(), false);

	menuState.CurState = SCAN;
	SetMenuSounds(MenuSoundArrows, MenuSoundSelect);
	InputFrameCallback* oldCallback {SetInputCallback(on_input_frame)};

	menuState.InputFunc = DoPlanetOrbit;
	DoInput(&menuState, true);

	SetInputCallback(oldCallback);

	SetFlashRect(nullptr, false);
	if (!(uqm::currentActivity() & CHECK_LOAD))
	{
		DrawMenuStateStrings(PM_STARMAP, -NAVIGATION);
	}
}