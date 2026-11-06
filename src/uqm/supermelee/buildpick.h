#ifndef BUILDPICK_H
#define BUILDPICK_H

#include <cstdint>
#include "melee.h"
#include "../races.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

void BuildBuildPickFrame(void);
void DestroyBuildPickFrame(void);
bool BuildPickShip(MeleeState* pMS);
void GetBuildPickFrameRect(GfxRect* r);
void GetToolTipFrameRect(GfxRect* r);

void DrawPickFrame(MeleeState* pMS);
void DrawPickIcon(MeleeShip ship, bool DrawErase);

void DrawTooltip(ShipInfoDesc* SIPtr);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* BUILDPICK_H */
