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

/*
 * This file seems to be a collection of functions that don't do
 * much.
 */

#include "core/types/enum.h"
#include "dummy.h"

#include "coderes.h"
#include "globdata.h"
#include "races.h"

#include "core/misc/bittricks.h"
#include "core/log/log.h"

#include <stdlib.h>
#include <ctype.h>


struct CodeResData
{
	RaceDesc data;
};

enum class ShipCodeRes
{
	Androsyn,
	Arilou,
	Blackurq,
	Chenjesu,
	Chmmr,
	Druuge,
	Human,
	Ilwrath,
	Melnorme,
	Mmrnmhrm,
	Mycon,
	Orz,
	Pkunk,
	Shofixti,
	Slylandr,
	Spathi,
	Supox,
	Syreen,
	Thradd,
	Umgah,
	Urquan,
	Utwig,
	Vux,
	Yehat,
	Zoqfot,

	Samatra,
	Sis,
	Probe
};
consteval bool isArithmeticEnum(ShipCodeRes) { return true; }

typedef RaceDesc* (*RaceDescInitFunc)(void);

static RaceDescInitFunc CodeResToInitFunc(ShipCodeRes res)
{
	switch (res)
	{
		case ShipCodeRes::Androsyn:
			return &init_androsynth;
		case ShipCodeRes::Arilou:
			return &init_arilou;
		case ShipCodeRes::Blackurq:
			return &init_black_urquan;
		case ShipCodeRes::Chenjesu:
			return &init_chenjesu;
		case ShipCodeRes::Chmmr:
			return &init_chmmr;
		case ShipCodeRes::Druuge:
			return &init_druuge;
		case ShipCodeRes::Human:
			return &init_human;
		case ShipCodeRes::Ilwrath:
			return &init_ilwrath;
		case ShipCodeRes::Melnorme:
			return &init_melnorme;
		case ShipCodeRes::Mmrnmhrm:
			return &init_mmrnmhrm;
		case ShipCodeRes::Mycon:
			return &init_mycon;
		case ShipCodeRes::Orz:
			return &init_orz;
		case ShipCodeRes::Pkunk:
			return &init_pkunk;
		case ShipCodeRes::Shofixti:
			return &init_shofixti;
		case ShipCodeRes::Slylandr:
			return &init_slylandro;
		case ShipCodeRes::Spathi:
			return &init_spathi;
		case ShipCodeRes::Supox:
			return &init_supox;
		case ShipCodeRes::Syreen:
			return &init_syreen;
		case ShipCodeRes::Thradd:
			return &init_thraddash;
		case ShipCodeRes::Umgah:
			return &init_umgah;
		case ShipCodeRes::Urquan:
			return &init_urquan;
		case ShipCodeRes::Utwig:
			return &init_utwig;
		case ShipCodeRes::Vux:
			return &init_vux;
		case ShipCodeRes::Yehat:
			return &init_yehat;
		case ShipCodeRes::Zoqfot:
			return &init_zoqfotpik;
		case ShipCodeRes::Samatra:
			return &init_samatra;
		case ShipCodeRes::Sis:
			return &init_sis;
		case ShipCodeRes::Probe:
			return &init_probe;
		default:
			{
				uqm::log::warn("Unknown SHIP identifier '{}'", (int)res);
				return nullptr;
			}
	}
}

static void GetCodeResData(const ResourceArg& arg, RESOURCE_DATA* resdata)
{
	const int64_t* const ship_id {uqstl::get_if<int64_t>(&arg)};
	if (ship_id == nullptr)
	{
		uqm::log::warn("A SHIP resource has no number.");
		resdata->ptr = nullptr;
		return;
	}
	const auto which_res {static_cast<uint8_t>(*ship_id)};
	RaceDescInitFunc initFunc = CodeResToInitFunc((ShipCodeRes)which_res);
	const RaceDesc* RDPtr = (initFunc == nullptr) ? nullptr : (*initFunc)();
	resdata->ptr = RDPtr ? new CodeResData {*RDPtr} : nullptr;
}

static bool _ReleaseCodeResData(void* data)
{
	delete static_cast<CodeResData*>(data);
	return true;
}

bool InstallCodeResType()
{
	return (InstallResTypeVectors("SHIP",
								  GetCodeResData, _ReleaseCodeResData));
}


void* LoadCodeResInstance(RESOURCE res)
{
	void* hData {res_GetResource(res)};
	if (hData)
	{
		res_DetachResource(res);
	}

	return hData;
}


bool DestroyCodeRes(void* hCode)
{
	delete static_cast<CodeResData*>(hCode);
	return true;
}


void* CaptureCodeRes(void* hCode, void* pData, void** ppLocData)
{
	if (hCode == nullptr)
	{
		uqm::log::critical("dummy.c::CaptureCodeRes() hCode==nullptr! FATAL!");
		return (nullptr);
	}

	CodeResData* cs {(CodeResData*)hCode};
	*ppLocData = &cs->data;

	(void)pData; /* Satisfying compiler (unused parameter) */
	return cs;
}


void* ReleaseCodeRes(void* CodeRef)
{
	return CodeRef;
}
