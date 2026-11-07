#pragma once
#ifndef UQM_SUPERMELEE_BUILDPICK_H
#define UQM_SUPERMELEE_BUILDPICK_H

#include <cstdint>
#include "melee.h"
#include "../races.h"

namespace uqm
{
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

} // namespace uqm

#endif // UQM_SUPERMELEE_BUILDPICK_H
