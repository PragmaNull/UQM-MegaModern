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

#include "../colors.h"
#include "../controls.h"
#include "../element.h"
#include "../gameopt.h"
#include "../gendef.h"
#include "../nameref.h"
#include "../resinst.h"
#include "../settings.h"
#include "../setup.h"
#include "../shipcont.h"
#include "../sounds.h"
#include "../starmap.h"
#include "../uqmdebug.h"
#include "../util.h"
#include "core/string/StringUtils.h"
#include "core/types/enum.h"
#include "lander.h"
#include "libs/graphics/gfx_common.h"
#include "options.h"
#include "scan.h"
#include "uqm/gamestr.h"


namespace uqm
{
// PlanetOrbitMenu() items
enum class PlanetMenuItem
{
	// XXX: Must match the enum in menustat.h
	Scan = 0,
	Starmap,
	EquipDevice,
	Cargo,
	Roster,
	GameMenu,
	Navigation,
};
consteval bool isArithmeticEnum(PlanetMenuItem) { return true; }

GfxContextRef PlanetContext;
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
		for (uint16_t i {0}; i < ScanType::Count; i++)
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

	Orbit->scanType = ScanType::Mineral;

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
	const ContextScope contextScope {SpaceContext};
	GetContextClipRect(&r);

	PlanetContext = CreateContext("PlanetContext");
	SetContext(PlanetContext);
	SetContextFGFrame(Screen);
	r.extent.height -= MapHeight() + MapBorderHeight();
	SetContextClipRect(&r);
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
	ElementHandle hElement {};
	ElementHandle hNextElement {};

	for (hElement = Reversed ? GetTailElement() : GetHeadElement();
		 hElement; hElement = hNextElement)
	{
		Element* ElementPtr {};

		LockElement(hElement, &ElementPtr);
		hNextElement = Reversed ?
						   GetPredElement(ElementPtr) :
						   GetSuccElement(ElementPtr);

		if (testFlag(ElementPtr->state_flags, ElementFlags::Appearing))
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

	const ContextScope contextScope {SpaceContext};
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

		DrawBorder(BorderFrameIndex::LanderDos);
	}
	else
	{
		DrawBorder(BorderFrameIndex::Lander3do);
	}

	UnbatchGraphics();

	SetContextClipRect(&oldClipRect);
}

enum class DrawOrbitalMode
{
	Full,
	Wait,
	Update,
	FromStarmap,

};

static void DrawEnterOrbitText(GfxRect rect)
{
	GfxText text;
	int16_t leading {};
	uqstl::array<char, 256> buf;

	const FontScope fontScope {MicroFont};
	const FontEffectScope fontEffectScope {SetAbsFrameIndex(FontGradFrame, 12)};

	GetContextFontLeading(&leading);

	text.baseline = rect.corner;
	text.baseline.x += rect.extent.width >> 1;
	text.baseline.y += ResScale(4) + leading - ResScale(4);
	text.align = TextAlign::Center;

	const int16_t og_baseline_x {text.baseline.x};

	uqm::utf8StringCopy(buf,
						GameString(GameStringBase::NavigationString + 8));

	text.align = TextAlign::Center;
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
}

void DrawOrbitMapGraphic(void)
{
	GfxStamp s;

	SetContext(GetScanContext(nullptr));

	if (uqm::UQMOptions::read().sphereType != 1)
	{
		const bool HaveString =
			strlen(GameString(GameStringBase::NavigationString + 8)) > 0;

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

static void DrawOrbitalDisplay(DrawOrbitalMode Mode)
{
	GfxRect r;

	SetContext(SpaceContext);
	GetContextClipRect(&r);

	BatchGraphics();

	if (Mode != DrawOrbitalMode::Update)
	{
		SetTransitionSource(nullptr);
		DrawSISFrame();
		DrawSISMessage(nullptr);
		DrawSISTitle(uqm::gameState().sis().PlanetName.data());
		DrawStarBackGround();
		DrawPlanetSurfaceBorder();
	}

	if (Mode == DrawOrbitalMode::Wait)
	{
		DrawOrbitMapGraphic();

		if (isPC(uqm::UQMOptions::read().landerStyle))
		{
			InitPCLander(true);
		}
	}
	else if (Mode == DrawOrbitalMode::Full)
	{
		DrawDefaultPlanetSphere();
		DrawMenuStateStrings(MenuString::Scan, *PlanetMenuItem::Scan);
	}
	else if (Mode == DrawOrbitalMode::FromStarmap)
	{
		DrawDefaultPlanetSphere();
		DrawMenuStateStrings(MenuString::Scan, *PlanetMenuItem::Starmap);
	}
	else
	{
		DrawMenuStateStrings(MenuString::Scan, *PlanetMenuItem::Scan);
	}

	if (Mode != DrawOrbitalMode::Wait)
	{
		SetContext(GetScanContext(nullptr));
		DrawPlanet(0, BlackColor);
		if (isPC(uqm::UQMOptions::read().landerStyle))
		{
			InitPCLander(false);
		}
	}

	if (Mode != DrawOrbitalMode::Update)
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
void LoadPlanet(FrameRef SurfDefFrame)
{
	const bool WaitMode = !(LastActivity & ActivityFlag::CheckLoad);
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
		DrawOrbitalDisplay(DrawOrbitalMode::Wait);
	}

	GeneratePlanetSurface(pPlanetDesc, SurfDefFrame, 0, 0);
	OrbitNum = SetPlanetMusic(pPlanetDesc->data_index & ~*PlanetType::PlanetShielded);
	GeneratePlanetSide();
	MaskLanderGraphics();

	if (isPC(uqm::UQMOptions::read().scrTrans))
	{
		SleepThreadUntil(sleep);
	}

	if (!PLRPlaying((MusicRef)~0))
	{
		PlayMusicResume(LanderMusic, NormalVolume);
	}

	if (WaitMode)
	{
		if (is3DO(uqm::UQMOptions::read().scrTrans) || uqm::UQMOptions::read().sphereType == 1)
		{
			ZoomInPlanetSphere();
		}
		DrawOrbitalDisplay(DrawOrbitalMode::Update);
	}
	else
	{
		DrawOrbitalDisplay(DrawOrbitalMode::Full);
	}
}

void FreePlanet(void)
{
	PlanetOrbit* Orbit = &pSolarSysState->Orbit;

	UninitSphereRotation();

	SetMusicPosition();

	StopMusic();

	for (auto& frame : pSolarSysState->PlanetSideFrame)
	{
		DestroyDrawable(
			ReleaseDrawable(frame));
		frame = 0;
	}

	//    FreeLanderData ();

	pSolarSysState->XlatTable.reset();
	DestroyDrawable(ReleaseDrawable(pSolarSysState->TopoFrame));
	pSolarSysState->TopoFrame = 0;

	if (uqm::UQMOptions::read().scanStyle == uqm::EmulationMode::PC)
	{
		for (uint16_t k {0}; k < ScanType::Count; k++)
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
	bool select = (bool)PulsedInputState.menu[*MenuKey::MenuSelect];

	if ((uqm::currentActivity() & (ActivityFlag::CheckAbort | ActivityFlag::CheckLoad))
		|| uqm::gameState().sis().CrewEnlisted == (uint16_t)~0)
	{
		return false;
	}

	// XXX: pMS actually refers to pSolarSysState->menuState
	const bool handled {DoMenuChooser(pMS, MenuString::Scan)};
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
		case *PlanetMenuItem::Scan:
			ScanSystem();
			if (uqm::currentActivity() & ActivityFlag::StartEncounter)
			{ // Found Fwiffo on Pluto
				return false;
			}
			break;
		case *PlanetMenuItem::EquipDevice:
			select = DevicesMenu();
			if (uqm::currentActivity() & ActivityFlag::StartEncounter)
			{ // Invoked Talking Pet, a Caster or Sun Device over Chmmr,
				// or a Caster for Ilwrath
				// Going into conversation
				return false;
			}
			break;
		case *PlanetMenuItem::Cargo:
			CargoMenu();
			break;
		case *PlanetMenuItem::Roster:
			select = RosterMenu();
			break;
		case *PlanetMenuItem::GameMenu:
			if (!GameOptions())
			{
				return false; // abort or load
			}
			break;
		case *PlanetMenuItem::Starmap:
			{
				bool AutoPilotSet {};
				{
					// Deactivate planet rotation
					const InputCallbackScope noCallback {nullptr};

					RepairSISBorder();

					AutoPilotSet = StarMap();
					if (uqm::currentActivity() & ActivityFlag::CheckAbort)
					{
						return false;
					}
				}

				if (!AutoPilotSet)
				{													  // Redraw the orbital display
					DrawOrbitalDisplay(DrawOrbitalMode::FromStarmap); //WAS FULL
					break;
				}
				FALLTHROUGH; // Fall through !!!
			}
		case *PlanetMenuItem::Navigation:
			return false;
	}

	if (!(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		if (select)
		{ // 3DO menu jumps to NAVIGATE after a successful submenu run
			if (uqm::UQMOptions::read().whichMenu != uqm::EmulationMode::PC)
			{
				pMS->CurState = *PlanetMenuItem::Navigation;
			}
			if (pMS->CurState != PlanetMenuItem::Starmap)
			{
				DrawMenuStateStrings(MenuString::Scan, pMS->CurState);
			}
		}
		SetFlashRect(SfrMenu3do(), false);
	}

	return true;
}

static void on_input_frame(void)
{
	if (!(uqm::currentActivity() & ActivityFlag::CheckAbort))
	{
		RotatePlanetSphere(true, nullptr);
	}
}

void PlanetOrbitMenu(void)
{
	MenuState menuState;

	memset(&menuState, 0, sizeof menuState);

	SetFlashRect(SfrMenu3do(), false);

	menuState.CurState = *PlanetMenuItem::Scan;
	SetMenuSounds(MenuSoundFlags::Arrows, MenuSoundFlags::Select);
	{
		const InputCallbackScope rotationCallback {on_input_frame};
		menuState.InputFunc = DoPlanetOrbit;
		DoInput(&menuState, true);
	}

	SetFlashRect(nullptr, false);
	if (!(uqm::currentActivity() & ActivityFlag::CheckLoad))
	{
		DrawMenuStateStrings(MenuString::Starmap, -PlanetMenuItem::Navigation);
	}
}

} // namespace uqm