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

#define MAX_LOAD_ENTRIES 40

void DoShipSpin(uint16_t index, MUSIC_REF hMusic)
{
	GFXRECT old_r;

	GFXCONTEXT OldContext {SetContext(ScreenContext)};

	if (uqm::UQMOptions::read().shipSeed)
	{
		HMASTERSHIP hMaster = GetStarShipFromIndex(&master_q, index);
		const MASTER_SHIP_INFO* mPtr = LockMasterShip(&master_q, hMaster);
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
		if (IS_PAD || OutfitOrShipyard == 3)
		{
			SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond / 4));
			FlushColorXForms();
		}

		ShowPresentation(ACCESSDATA_STRTAB);
	}

	// TODO: It would be nice to have better resource names for these.
	char vnbuf[24] {}; // From 32 to 24
	fmt::format_to_n(vnbuf, sizeof(vnbuf) - 1, "slides.spins. {:02}", (unsigned)index);

	ShowPresentation(vnbuf);

	if (is3DO(uqm::UQMOptions::read().whichIntro))
	{
		SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond / 4));
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
		SleepThreadUntil(FadeScreen(FadeAllToColor, c_gameSecond / 4));
	}
	FlushColorXForms();
}

void SplashScreen(void (*DoProcessing)(TimeCount TimeOut))
{
	STAMP s;
	TimeCount TimeOut {};
	const TimeCount OverallWait = GetTimeCounter() + (c_gameSecond * 3);

	if (!uqm::UQMOptions::read().skipIntro)
	{
		SetContext(ScreenContext);
		s.origin.x = s.origin.y = 0;

		s.frame = CaptureDrawable(LoadGraphic(TITLE_ANIM));

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

		TimeOut = FadeScreen(FadeAllToColor, c_gameSecond / 2);
	}

	if (DoProcessing)
	{
		DoProcessing(TimeOut);
	}
	if (uqm::currentActivity() & CHECK_ABORT)
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
	if (uqm::currentActivity() & CHECK_ABORT)
	{
		return;
	}
	uqm::currentActivity() &= ~CHECK_ABORT;

	if (!uqm::UQMOptions::read().skipIntro)
	{
		SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond / 2));
	}
}

void Introduction(void)
{
	ShowPresentation(INTROPRES_STRTAB);
	SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond / 2));
}

void Victory(void)
{
	SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond / 2));

	/* by default we do 3DO cinematics; or PC slides when 3DO files are
	 * not present */
	ShowPresentation(FINALPRES_STRTAB);

	FadeScreen(FadeAllToBlack, TimePeriod {});
}

void GameOver(uint8_t DeathType)
{
	if (DeathType != SUICIDE)
	{
		SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond / 2));
	}

	switch (DeathType)
	{
		case DEATH_MARCH:
			ShowPresentation(DEATHMARCHPRES_STRTAB);
			break;
		case SUICIDE:
			SleepThreadUntil(FadeScreen(FadeAllToWhite, c_gameSecond / 2));
			ShowPresentation(SUICIDEPRES_STRTAB);
			break;
		case SURRENDERED:
			ShowPresentation(SURRENDEREDPRES_STRTAB);
			break;
		case DIED_IN_BATTLE:
		default:
			ShowPresentation(DEFEATEDPRES_STRTAB);
			break;
	}
	FadeScreen(FadeAllToBlack, TimePeriod {});
}

void Logo(void)
{
	ShowPresentation(LOGOPRES_STRTAB);
	FadeScreen(FadeAllToBlack, TimePeriod {});
}

void Drumall(void)
{
	ShowPresentation(DRUMALLPRES_STRTAB);
	SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond / 2));
}

void Reload(void)
{
	ShowPresentation(RELOADPRES_STRTAB);

	ReloadGameContent();
	SleepThreadUntil(FadeScreen(FadeAllToBlack, c_gameSecond / 2));
}

void AdvanceLoadProgress(void)
{
	if (optRequiresReload)
	{
		GFXRECT r;
		static uint16_t i = 0;

		r.corner.x = RES_SCALE(16);
		r.corner.y = CanvasHeight - DOS_BOOL_SCL(50, 30);
		r.extent.height = RES_SCALE(15);
		r.extent.width = RES_SCALE((i + 1) * 7);

		if (i == MAX_LOAD_ENTRIES)
		{
			SetContextForeGroundColor(BUILD_COLOR_RGBA(0, 192, 0, 255));
			i = 0;
		}
		else
		{
			SetContextForeGroundColor(
				BUILD_COLOR_RGBA(0, 128, 192, 255));
			i++;
		}
		DrawFilledRectangle(&r);
	}
}
