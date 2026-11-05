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

#ifndef UQM_DISPLIST_H_
#define UQM_DISPLIST_H_

#include <assert.h>
#include "port.h"
#include "core/misc/bittricks.h"
#include "core/stl/utility.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

// Each queue has a fixed table of elements. The size of encounter_q sets
// the maximum number of HyperSpace encounter globes.

typedef void* QUEUE_HANDLE;

typedef uint16_t OBJ_SIZE;
typedef QUEUE_HANDLE HLINK;

typedef struct link
{
	// Every queue element of any queue must have these
	// two as the first members
	HLINK pred;
	HLINK succ;
} LINK;

typedef struct /* queue */
{
	HLINK head;
	HLINK tail;
	uint8_t* pq_tab;
	HLINK free_list;
	uint16_t object_size;
	uint8_t num_objects;
} QUEUE;


extern HLINK AllocLink(QUEUE* pq);
extern void FreeLink(QUEUE* pq, HLINK hLink);

static inline LINK*
LockLink(const QUEUE* pq, HLINK h)
{
	if (h) // Apparently, h==0 is OK
	{	   // Make sure the link is actually in our queue!
		assert(pq->pq_tab && (uint8_t*)h >= pq->pq_tab && (uint8_t*)h < pq->pq_tab + pq->object_size * pq->num_objects);
	}
	return (LINK*)h;
}

static inline void
UnlockLink(const QUEUE* pq, HLINK h)
{
	if (h) // Apparently, h==0 is OK
	{	   // Make sure the link is actually in our queue!
		assert(pq->pq_tab && (uint8_t*)h >= pq->pq_tab && (uint8_t*)h < pq->pq_tab + pq->object_size * pq->num_objects);
	}
}

constexpr auto GetFreeList(const auto& pq) { return (pq)->free_list; }
constexpr auto SetFreeList(const auto& pq, const auto& h) { return (pq)->free_list = (h); }
constexpr auto AllocQueueTab(const auto& pq, const auto& n) { return ((pq)->pq_tab = new uint8_t[(uint16_t)(pq)->object_size * (uint16_t)((pq)->num_objects = (uint8_t)(n))]); }
#define FreeQueueTab(pq)   \
	delete[] (pq)->pq_tab; \
	(pq)->pq_tab = nullptr
constexpr auto SizeQueueTab(const auto& pq) { return (uint16_t)((pq)->num_objects); }
constexpr auto GetLinkAddr(const auto& pq, const auto& i) { return (HLINK)((pq)->pq_tab + ((pq)->object_size * ((i)-1))); }

constexpr auto SetLinkSize(const auto& pq, const auto& s) { return ((pq)->object_size = (uint16_t)(s)); }
constexpr auto GetLinkSize(const auto& pq) { return (uint16_t)((pq)->object_size); }
constexpr auto GetHeadLink(const auto& pq) { return ((pq)->head); }
constexpr auto SetHeadLink(const auto& pq, const auto& h) { return ((pq)->head = (h)); }
constexpr auto GetTailLink(const auto& pq) { return ((pq)->tail); }
constexpr auto SetTailLink(const auto& pq, const auto& h) { return ((pq)->tail = (h)); }
constexpr auto _GetPredLink(const auto& lpE) { return ((lpE)->pred); }
constexpr auto _SetPredLink(const auto& lpE, const auto& h) { return ((lpE)->pred = (h)); }
constexpr auto _GetSuccLink(const auto& lpE) { return ((lpE)->succ); }
constexpr auto _SetSuccLink(const auto& lpE, const auto& h) { return ((lpE)->succ = (h)); }

extern bool InitQueue(QUEUE* pq, uint16_t num_elements, OBJ_SIZE size);
extern bool UninitQueue(QUEUE* pq);
extern void ReinitQueue(QUEUE* pq);
extern void PutQueue(QUEUE* pq, HLINK hLink);
extern void InsertQueue(QUEUE* pq, HLINK hLink, HLINK hRefLink);
extern void RemoveQueue(QUEUE* pq, HLINK hLink);
extern uint16_t CountLinks(QUEUE* pq);
void ForAllLinks(QUEUE* pq, void (*callback)(LINK*, void*), void* arg);

namespace uqm
{

// A QUEUE that owns its table of links. Code that takes a QUEUE* can use it.
class LinkQueue : public QUEUE
{
public:
	LinkQueue() noexcept
		: QUEUE {}
	{
	}

	~LinkQueue()
	{
		release();
	}

	LinkQueue(const LinkQueue&) = delete;
	LinkQueue& operator=(const LinkQueue&) = delete;

	// Makes an empty table for 'capacity' links of 'linkSize' bytes. Releases the old table.
	bool reset(uint16_t capacity, OBJ_SIZE linkSize)
	{
		release();
		return InitQueue(this, capacity, linkSize);
	}

	// Releases the table. The queue has no links after this.
	void release() noexcept
	{
		UninitQueue(this);
	}

	friend void swap(LinkQueue& first, LinkQueue& second) noexcept
	{
		uqstl::swap(static_cast<QUEUE&>(first), static_cast<QUEUE&>(second));
	}
};

} // namespace uqm

#if 0 //defined(__cplusplus)
}
#endif

#endif /* UQM_DISPLIST_H_ */
