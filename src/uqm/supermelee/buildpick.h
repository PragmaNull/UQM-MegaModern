#pragma once
#ifndef UQM_SUPERMELEE_BUILDPICK_H
#define UQM_SUPERMELEE_BUILDPICK_H

#include <cstdint>

#include "../races.h"
#include "melee.h"

namespace uqm
{
void BuildBuildPickFrame(void);
void DestroyBuildPickFrame(void);
bool BuildPickShip(MeleeState* pMS);
void GetBuildPickFrameRect(GfxRect* r);
void GetToolTipFrameRect(GfxRect* r);

void DrawPickFrame(MeleeState* pMS);
void DrawPickIcon(MeleeShip ship, bool DrawErase);

void DrawTooltip(ShipInfoDesc* SIPtr);

} // namespace uqm

#endif // UQM_SUPERMELEE_BUILDPICK_H
