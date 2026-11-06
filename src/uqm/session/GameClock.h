// The calendar of a game and its queue of dated events.
#pragma once
#ifndef UQM_SESSION_GAMECLOCK_H_
#define UQM_SESSION_GAMECLOCK_H_

#include <cstdint>
#include "core/stl/optional.h"
#include "core/stl/vector.h"
#include "uqm/clock.h"

namespace uqm
{

class GameClock
{
public:
	// Makes the event queue for this number of events and sets the date to the start of a game.
	[[nodiscard]] bool start(uint16_t eventCapacity);
	// Releases the event queue.
	void stop();

	[[nodiscard]] uint8_t day() const noexcept
	{
		return m_day;
	}

	[[nodiscard]] uint8_t month() const noexcept
	{
		return m_month;
	}

	[[nodiscard]] uint16_t year() const noexcept
	{
		return m_year;
	}

	// Ticks left in the current day.
	[[nodiscard]] int16_t tickCount() const noexcept
	{
		return m_tickCount;
	}

	// Ticks in one day. 0 if the clock does not run.
	[[nodiscard]] int16_t dayInTicks() const noexcept
	{
		return m_dayInTicks;
	}

	// Sets the date and the ticks, for example from a save.
	void restore(uint8_t day, uint8_t month, uint16_t year, int16_t tickCount, int16_t dayInTicks) noexcept;

	// Days since the start of the game, with the part of the current day.
	[[nodiscard]] float daysElapsed() const;

	// Sets the length of a day, and keeps the part of the current day that is done.
	void setDayInTicks(int16_t dayInTicks) noexcept;

	// Counts one tick. Returns true if a new day starts.
	[[nodiscard]] bool tick() noexcept;
	// Moves the date to the next day.
	void nextDay() noexcept;
	// Starts the current day again.
	void restartDay() noexcept;

	// Makes a relative date absolute. Returns false if the date is before today.
	[[nodiscard]] bool resolveDate(EVENT_TYPE type, uint16_t& month, uint16_t& day, uint16_t& year) const;
	// Adds an event at an absolute date, after the events on the same date. Returns 0 if the queue is full.
	HEVENT addEvent(uint16_t month, uint16_t day, uint16_t year, uint8_t funcIndex);
	// The first event in the queue, or nullopt if the queue is empty.
	[[nodiscard]] uqstl::optional<GameEvent> firstEvent() const;
	// The events in the queue, in date order.
	[[nodiscard]] uqstl::vector<GameEvent> eventList() const;
	// True if the clock date is before the date.
	[[nodiscard]] bool isBefore(uint16_t year, uint16_t month, uint16_t day) const noexcept;
	// Removes the first event of today or before from the queue and gives its function index.
	// Returns false if no event is due.
	[[nodiscard]] bool takeDueEvent(uint8_t& funcIndex);

	[[nodiscard]] Queue& events() noexcept
	{
		return m_events;
	}

private:
	uint8_t m_day {};
	uint8_t m_month {};
	uint16_t m_year {};
	int16_t m_tickCount {};
	int16_t m_dayInTicks {};
	LinkQueue m_events {};
};

} // namespace uqm

#endif // UQM_SESSION_GAMECLOCK_H_
