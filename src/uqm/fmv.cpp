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

#include "fmv.h"
#include "controls.h"
#include "hyper.h"
#include "options.h"
#include "master.h"
#include "resinst.h"
#include "nameref.h"
#include "settings.h"
#include "setup.h"
#include "libs/vidlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/sound/sound.h"
#include "init.h"
#include "gameopt.h"
#include "build.h"

namespace uqm
{
static constexpr auto MaxLoadEntries {40};

void DoShipSpin(uint16_t index, MUSIC_REF hMusic)
{
	GfxRect old_r;

	GFXCONTEXT OldContext {SetContext(ScreenContext)};

	if (uqm::currentSeed().ships)
	{
		HMASTERSHIP hMaster = GetStarShipFromIndex(&master_q, index);
		const MasterShipInfo* mPtr = LockMasterShip(&master_q, hMaster);
		index = FindMasterShipIndex(SeedShip(mPtr->SpeciesID, false));
		UnlockMasterShip(&master_q, hMaster);
	}

	LoadIntoExtraScreen(nullptr);

	if (hMusic)
	{
		StopMusic();
	}

	FreeHyperData();

	if (isPC(uqm::UQMOptions::read().whichIntro))
	{
		if (IsPad() || OutfitOrShipyard == 3)
		{
			SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 4));
			FlushColorXForms();
		}

		ShowPresentation(AccessdataStrtabRes);
	}

	// TODO: It would be nice to have better resource names for these.
	char vnbuf[24] {}; // From 32 to 24
	fmt::format_to_n(vnbuf, sizeof(vnbuf) - 1, "slides.spins. {:02}", (unsigned)index);

	ShowPresentation(vnbuf);

	if (is3DO(uqm::UQMOptions::read().whichIntro))
	{
		SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 4));
	}
	FlushColorXForms();

	SetContext(OldContext);

	GetContextClipRect(&old_r);
	SetContextClipRect(nullptr);
	DrawFromExtraScreen(nullptr);
	SetContextClipRect(&old_r);

	if (hMusic)
	{
		PlayMusic(hMusic, true, 1);
	}

	if (is3DO(uqm::UQMOptions::read().whichIntro))
	{
		SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToColor, c_gameSecond / 4));
	}
	FlushColorXForms();
}

void SplashScreen(void (*DoProcessing)(TimeCount TimeOut))
{
	GfxStamp s;
	TimeCount TimeOut {};
	const TimeCount OverallWait = GetTimeCounter() + (c_gameSecond * 3);

	if (!uqm::UQMOptions::read().skipIntro)
	{
		SetContext(ScreenContext);
		s.origin.x = s.origin.y = 0;

		s.frame = CaptureDrawable(LoadGraphic(TitleAnimRes));

		if (uqm::UQMOptions::read().flagshipColor == uqm::EmulationMode::Console3DO)
		{

			s.frame = SetAbsFrameIndex(s.frame, 1);
		}
		else
		{
			s.frame = SetAbsFrameIndex(s.frame, 0);
		}

		DrawStamp(&s);
		DestroyDrawable(ReleaseDrawable(s.frame));

		TimeOut = FadeScreen(ScreenFadeType::FadeAllToColor, c_gameSecond / 2);
	}

	if (DoProcessing)
	{
		DoProcessing(TimeOut);
	}
	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		return;
	}

	/* There was a forcible setting of CHECK_ABORT here.  I cannot
	 * find any purpose for this that DoRestart doesn't handle
	 * better (forcing all other threads but this one to quit out,
	 * I believe), and have thus removed it.  It was interfering
	 * with the proper operation of the quit operation.
	 * --Michael */

	if (!uqm::UQMOptions::read().skipIntro)
	{
		SleepThreadUntil(OverallWait);
	}
	if (uqm::currentActivity() & ActivityFlag::CheckAbort)
	{
		return;
	}
	uqm::currentActivity() &= ~ActivityFlag::CheckAbort;

	if (!uqm::UQMOptions::read().skipIntro)
	{
		SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
	}
}

void Introduction(void)
{
	ShowPresentation(IntropresStrtabRes);
	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
}

void Victory(void)
{
	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));

	/* by default we do 3DO cinematics; or PC slides when 3DO files are
	 * not present */
	ShowPresentation(FinalpresStrtabRes);

	FadeScreen(ScreenFadeType::FadeAllToBlack, TimePeriod {});
}

void GameOver(EndgameScenario DeathType)
{
	if (DeathType != EndgameScenario::Suicide)
	{
		SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
	}

	switch (DeathType)
	{
		case EndgameScenario::DeathMarch:
			ShowPresentation(DeathmarchpresStrtabRes);
			break;
		case EndgameScenario::Suicide:
			SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToWhite, c_gameSecond / 2));
			ShowPresentation(SuicidepresStrtabRes);
			break;
		case EndgameScenario::Surrendered:
			ShowPresentation(SurrenderedpresStrtabRes);
			break;
		case EndgameScenario::DiedInBattle:
		default:
			ShowPresentation(DefeatedpresStrtabRes);
			break;
	}
	FadeScreen(ScreenFadeType::FadeAllToBlack, TimePeriod {});
}

void Logo(void)
{
	ShowPresentation(LogopresStrtabRes);
	FadeScreen(ScreenFadeType::FadeAllToBlack, TimePeriod {});
}

void Drumall(void)
{
	ShowPresentation(DrumallpresStrtabRes);
	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
}

void Reload(void)
{
	ShowPresentation(ReloadpresStrtabRes);

	ReloadGameContent();
	SleepThreadUntil(FadeScreen(ScreenFadeType::FadeAllToBlack, c_gameSecond / 2));
}

void AdvanceLoadProgress(void)
{
	if (optRequiresReload)
	{
		GfxRect r;
		static uint16_t i = 0;

		r.corner.x = ResScale(16);
		r.corner.y = CanvasHeight - DosBoolScl(50, 30);
		r.extent.height = ResScale(15);
		r.extent.width = ResScale((i + 1) * 7);

		if (i == MaxLoadEntries)
		{
			SetContextForeGroundColor(BuildColorRgba(0, 192, 0, 255));
			i = 0;
		}
		else
		{
			SetContextForeGroundColor(
				BuildColorRgba(0, 128, 192, 255));
			i++;
		}
		DrawFilledRectangle(&r);
	}
}

} // namespace uqm
