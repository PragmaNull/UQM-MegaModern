#pragma once
#ifndef UQM_GENDEF_H
#define UQM_GENDEF_H

// JSD planets/generate.h no longer needed once GenerateFunctions is moved there.  It's where it belongs
// and the include of generate.h is causing circular definitions elsewhere.
// Added ARILOU_DEFINED = 0 (for location of quasi-portal and arilou zone of influence).
// Split RAINBOW_DEFINED into separate entries (like MELNORME0_DEFINED is) so they can be scattered.
// Added NUM_PLOTS for plot iteration.
// These are used in the plot randomizer.
//#include "planets/generate.h"
#include "core/types/enum.h"
#include "core/misc/bittricks.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

//const GenerateFunctions *getGenerateFunctions (uint8_t Index);

enum class PlotId : uint8_t
{
	Arilou = 0,
	Sol = 1,	 // gensol.c, Super hardcoded, must be yellow dwarf
	Shofixti,	 // gensho.c, planet 1* randomly METAL and SELENIC
	Maidens,	 // genvux.c, planet 1 [habitable] (redux)
	StartColony, // gencol.c, planet 1 [habitable] (water)
	Spathi,		 // genspa.c, planet 1 [habitable] (water)
	Zoqfot,		 // genzoq.c, planet 1 [habitable] (redux)

	Melnorme0,
	Melnorme1, // genmel.c, planet 3/rand [crystal]
	Melnorme2,
	Melnorme3,
	Melnorme4,
	Melnorme5,
	Melnorme6,
	Melnorme7, // genmel.c, planet 4/rand [any rocky]
	Melnorme8,

	TalkingPet,		// genpet.c, planet 1 [habitable] (telluric)
	Chmmr,			// genchmmr.c, planet 2/rand [crystal] (sapphire)
	Syreen,			// gensyr.c, planet 1 [habitable] (water)
	Burvixese,		// genburv.c, planet 1 [habitable] (redux) A [rand]
	Slylandro,		// gensly.c, planet 4/rand [gas giant]
	Druuge,			// gendru.c, planet 1 [desolate] (dust)
	Bomb,			// genutw.c, planet 6 [gas] B [rand] (carbide)
	AquaHelix,		// genthrad.c planet 1 [habitable] (primordial)
	SunDevice,		// genmyc.c, planet 1 shattered
	TaaloProtector, // genorz.c, planet 2 [gas] rand [crystal] (treas)
	ShipVault,		// genvault.c, planet 1 [lg rocky] A [rand] (green)
	UrquanWreck,	// genwreck.c, planet 7 [sm rocky] (noble)
	VuxBeast,		// genvux.c, planet 1 [habitable] (water)
	Samatra,		// gensam.c, planet 5/rand [random]
	ZoqScout,
	Mycon,	  // genmyc.c, planet 1 shattered
	EggCase0, // genmyc.c, planet 1 shattered
	EggCase1, // genmyc.c, planet 1 shattered
	EggCase2, // genmyc.c, planet 1 shattered
	Pkunk,	  // genpku.c, planet 1 [habitable] (water)
	Utwig,	  // genutw.c, planet 1 [habitable] (water????)
	Supox,	  // gensup.c, planet 1 [habitable] (water)
	Yehat,	  // genyeh.c, planet 1 [habitable] (water)
	Vux,	  // genvux.c, planet 1 [habitable] (redux)
	Orz,	  // genorz.c, planet 1 [habitable] (water)
	Thradd,	  // genthrad.c, planet 1 [habitable] (water)
	Rainbow0,
	Rainbow1,
	Rainbow2,
	Rainbow3,
	Rainbow4,
	Rainbow5,
	Rainbow6,
	Rainbow7,
	Rainbow8,
	Rainbow9,
	Ilwrath,		   // genilw.c, planet 1 [habitable] (primordial)
	Androsynth,		   // genand.c, planet 2/rand [habitable] (telluric)
	MyconTrap,		   // gentrap.c, planet 1 [habitable] (telluric)
	Urquan0,		   // gensam.c, planet random
	Urquan1,		   // gensam.c, planet random
	Urquan2,		   // gensam.c, planet random
	Kohrah0,		   // gensam.c, planet random
	Kohrah1,		   // gensam.c, planet random
	Kohrah2,		   // gensam.c, planet random
	DestroyedStarbase, // gensam.c, planet 1 [habitable] (primordial)
	MotherArk,		   // genchmmr.c, planet 4/rand [crystal] (emerald)
	ZoqColony0,		   // genzoq.c, planet 0 [habitable] (redux)
	ZoqColony1,		   // genzoq.c, planet 1 [habitable] (redux)
	ZoqColony2,		   // genzoq.c, planet 1 moon rand [habitable] (redux)
	ZoqColony3,		   // genzoq.c, planet 0 [habitable] (redux)
	Algolites,		   // genspa.c, planet 4 [habitable, airless]
	SpathiMonument,	   // genspa.c, planet random [crystal]
	ExcavationSite,	   // genand.c, planet 1/rand [any rocky]
	NumPlots,		   // Must be right after last valid entry.  Any new
	WarEra = NumPlots, // entries need plot min/max added in gendef.c
	Success = NumPlots,
	Home = NumPlots + 1
};
consteval bool isArithmeticEnum(PlotId) { return true; }

inline constexpr auto UmgahDefined {PlotId::TalkingPet};

#if 0 //defined(__cplusplus)
}
#endif

#endif // UQM_GENDEF_H
