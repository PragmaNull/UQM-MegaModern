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

#include "displist.h"
#include "core/log/log.h"


namespace uqm
{
/*
 * This file contains code for generic doubly linked lists.
 * Each list has its own preallocated
 * pool of link structures. The size is specific on InitQueue(),
 * and poses a hard limit on the number of elements in the list.
 */

bool InitQueue(Queue* pq, uint16_t num_elements, ObjSize size)
{
	SetHeadLink(pq, nullptr);
	SetTailLink(pq, nullptr);
	SetLinkSize(pq, size);
	SetFreeList(pq, nullptr);
#if 0	
	log_add (log_Debug, "InitQueue(): num_elements = {} ({})",
			num_elements, (uint8_t)num_elements);
#endif
	if (AllocQueueTab(pq, num_elements) != nullptr)
	{
		do
		{
			FreeLink(pq, GetLinkAddr(pq, num_elements));
		} while (--num_elements);

		return (true);
	}

	return (false);
}

bool UninitQueue(Queue* pq)
{
	SetHeadLink(pq, nullptr);
	SetTailLink(pq, nullptr);
	SetFreeList(pq, nullptr);
	FreeQueueTab(pq);

	return (true);
}

// Empty the queue. The elements linked to in the queue are unchanged.
void ReinitQueue(Queue* pq)
{
	SetHeadLink(pq, nullptr);
	SetTailLink(pq, nullptr);
	{
		SetFreeList(pq, nullptr);

		uint16_t num_elements {SizeQueueTab(pq)};
		if (num_elements)
		{
			do
			{
				FreeLink(pq, GetLinkAddr(pq, num_elements));
			} while (--num_elements);
		}
	}
}

LinkHandle AllocLink(Queue* pq)
{
	LinkHandle hLink {GetFreeList(pq)};
	if (hLink)
	{
		const QueueLink* LinkPtr {LockLink(pq, hLink)};
		SetFreeList(pq, _GetSuccLink(LinkPtr));
		UnlockLink(pq, hLink);
	}
	else
	{
		uqm::log::debug("AllocLink(): No more elements");
	}

	return (hLink);
}

void FreeLink(Queue* pq, LinkHandle hLink)
{
	QueueLink* LinkPtr {LockLink(pq, hLink)};
	_SetSuccLink(LinkPtr, GetFreeList(pq));
	UnlockLink(pq, hLink);

	SetFreeList(pq, hLink);
}

void PutQueue(Queue* pq, LinkHandle hLink)
{
	if (GetHeadLink(pq) == 0)
	{
		SetHeadLink(pq, hLink);
	}
	else
	{
		LinkHandle hTail {GetTailLink(pq)};
		QueueLink* lpTail {LockLink(pq, hTail)};
		_SetSuccLink(lpTail, hLink);
		UnlockLink(pq, hTail);
	}

	QueueLink* LinkPtr {LockLink(pq, hLink)};
	_SetPredLink(LinkPtr, GetTailLink(pq));
	_SetSuccLink(LinkPtr, nullptr);
	UnlockLink(pq, hLink);

	SetTailLink(pq, hLink);
}

void InsertQueue(Queue* pq, LinkHandle hLink, LinkHandle hRefLink)
{
	if (hRefLink == 0)
	{
		PutQueue(pq, hLink);
	}
	else
	{
		QueueLink* LinkPtr {LockLink(pq, hLink)};
		QueueLink* RefLinkPtr {LockLink(pq, hRefLink)};
		_SetPredLink(LinkPtr, _GetPredLink(RefLinkPtr));
		_SetPredLink(RefLinkPtr, hLink);
		_SetSuccLink(LinkPtr, hRefLink);

		if (GetHeadLink(pq) == hRefLink)
		{
			SetHeadLink(pq, hLink);
		}
		else
		{
			LinkHandle hPredLink {_GetPredLink(LinkPtr)};
			QueueLink* PredLinkPtr {LockLink(pq, hPredLink)};
			_SetSuccLink(PredLinkPtr, hLink);
			UnlockLink(pq, hPredLink);
		}
		UnlockLink(pq, hRefLink);
		UnlockLink(pq, hLink);
	}
}

void RemoveQueue(Queue* pq, LinkHandle hLink)
{
	const QueueLink* LinkPtr {LockLink(pq, hLink)};
	if (GetHeadLink(pq) == hLink)
	{
		SetHeadLink(pq, _GetSuccLink(LinkPtr));
	}
	else
	{
		LinkHandle hPredLink {_GetPredLink(LinkPtr)};
		QueueLink* PredLinkPtr {LockLink(pq, hPredLink)};
		_SetSuccLink(PredLinkPtr, _GetSuccLink(LinkPtr));
		UnlockLink(pq, hPredLink);
	}
	if (GetTailLink(pq) == hLink)
	{
		SetTailLink(pq, _GetPredLink(LinkPtr));
	}
	else
	{
		LinkHandle hSuccLink {_GetSuccLink(LinkPtr)};
		QueueLink* SuccLinkPtr {LockLink(pq, hSuccLink)};
		LinkHandle hPredLink {_GetPredLink(LinkPtr)};
		_SetPredLink(SuccLinkPtr, hPredLink);
		UnlockLink(pq, hSuccLink);
	}
	UnlockLink(pq, hLink);
}

uint16_t CountLinks(Queue* pq)
{
	LinkHandle hNextLink {};

	uint16_t LinkCount {0};
	for (LinkHandle hLink {GetHeadLink(pq)}; hLink; hLink = hNextLink)
	{
		++LinkCount;

		const QueueLink* LinkPtr {LockLink(pq, hLink)};
		hNextLink = _GetSuccLink(LinkPtr);
		UnlockLink(pq, hLink);
	}

	return (LinkCount);
}

void ForAllLinks(Queue* pq, void (*callback)(QueueLink*, void*), void* arg)
{
	LinkHandle hNextLink {};

	for (LinkHandle hLink {GetHeadLink(pq)}; hLink; hLink = hNextLink)
	{
		QueueLink* LinkPtr {LockLink(pq, hLink)};
		hNextLink = _GetSuccLink(LinkPtr);
		(*callback)(LinkPtr, arg);
		UnlockLink(pq, hLink);
	}
}

} // namespace uqm
