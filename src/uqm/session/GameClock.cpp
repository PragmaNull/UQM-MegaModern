// The calendar of a game and its queue of dated events.

#include "GameClock.h"


namespace uqm
{
namespace
{

constexpr uint8_t c_startMonth {2};
constexpr uint8_t c_startDay {17};

[[nodiscard]] bool isLeapYear(uint16_t year)
{
	return (year & 3) == 0 && ((year % 100) != 0 || (year % 400) == 0);
}

// 'month' is 1 for January.
[[nodiscard]] uint8_t daysInMonth(uint16_t month, uint16_t year)
{
	static constexpr uqstl::array<uint8_t, 12> c_daysInMonth {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (month == 2 && isLeapYear(year))
	{
		return 29;
	}
	return c_daysInMonth[month - 1];
}

// True if the first date is before the second.
[[nodiscard]] bool dateIsBefore(uint16_t year, uint16_t month, uint16_t day, uint16_t otherYear, uint16_t otherMonth,
								uint16_t otherDay)
{
	return year < otherYear || (year == otherYear && (month < otherMonth || (month == otherMonth && day < otherDay)));
}

[[nodiscard]] GameEvent* lockEvent(const Queue& events, EventHandle event)
{
	// An GameEvent starts with the QueueLink fields.
	return reinterpret_cast<GameEvent*>(LockLink(&events, event));
}

} // namespace

bool GameClock::start(uint16_t eventCapacity)
{
	if (!m_events.reset(eventCapacity, sizeof(GameEvent)))
	{
		return false;
	}
	restore(c_startDay, c_startMonth, StartYear, 0, 0);
	return true;
}

void GameClock::stop()
{
	m_events.release();
}

void GameClock::restore(uint8_t day, uint8_t month, uint16_t year, int16_t tickCount, int16_t dayInTicks) noexcept
{
	m_day = day;
	m_month = month;
	m_year = year;
	m_tickCount = tickCount;
	m_dayInTicks = dayInTicks;
}

float GameClock::daysElapsed() const
{
	float days {0};
	for (uint16_t year {StartYear}; year < m_year; ++year)
	{
		days += isLeapYear(year) ? 366 : 365;
	}
	if (m_month == 1)
	{
		days -= 31;
	}
	for (uint16_t month {2}; month < m_month; ++month)
	{
		days += daysInMonth(month, m_year);
	}
	days += m_day - c_startDay;
	days += (m_dayInTicks - m_tickCount) / static_cast<float>(m_dayInTicks);
	return days;
}

void GameClock::setDayInTicks(int16_t dayInTicks) noexcept
{
	int16_t tickCount {};
	if (m_dayInTicks == 0)
	{
		tickCount = dayInTicks;
	}
	else if (m_tickCount <= 0)
	{
		tickCount = 0;
	}
	else
	{
		tickCount = static_cast<int16_t>(static_cast<uint32_t>(m_tickCount) * dayInTicks / m_dayInTicks);
		if (tickCount == 0)
		{
			tickCount = 1;
		}
	}
	m_dayInTicks = dayInTicks;
	m_tickCount = tickCount;
}

bool GameClock::tick() noexcept
{
	--m_tickCount;
	if (m_tickCount > 0)
	{
		return false;
	}
	m_tickCount = m_dayInTicks;
	// The clock does not run until it has a day length.
	return m_dayInTicks > 0;
}

void GameClock::nextDay() noexcept
{
	++m_day;
	if (m_day > daysInMonth(m_month, m_year))
	{
		m_day = 1;
		++m_month;
		if (m_month > 12)
		{
			m_month = 1;
			++m_year;
		}
	}
}

void GameClock::restartDay() noexcept
{
	m_tickCount = m_dayInTicks;
}

bool GameClock::resolveDate(EventType type, uint16_t& month, uint16_t& day, uint16_t& year) const
{
	if (type == EventType::Relative)
	{
		month += m_month - 1;
		year += m_year + (month / 12);
		month = (month % 12) + 1;

		day += m_day;
		while (day > daysInMonth(month, year))
		{
			day -= daysInMonth(month, year);
			if (++month > 12)
			{
				month = 1;
				++year;
			}
		}
	}
	return !dateIsBefore(year, month, day, m_year, m_month, m_day);
}

EventHandle GameClock::addEvent(uint16_t month, uint16_t day, uint16_t year, GameEventId funcIndex)
{
	const EventHandle newEvent {AllocLink(&m_events)};
	if (!newEvent)
	{
		return 0;
	}
	GameEvent* const newPtr {lockEvent(m_events, newEvent)};
	newPtr->day_index = static_cast<uint8_t>(day);
	newPtr->month_index = static_cast<uint8_t>(month);
	newPtr->year_index = year;
	newPtr->func_index = funcIndex;
	UnlockLink(&m_events, newEvent);

	EventHandle event {GetHeadLink(&m_events)};
	while (event != 0)
	{
		const GameEvent* const eventPtr {lockEvent(m_events, event)};
		const bool later {dateIsBefore(year, month, day, eventPtr->year_index, eventPtr->month_index, eventPtr->day_index)};
		const EventHandle next {_GetSuccLink(eventPtr)};
		UnlockLink(&m_events, event);
		if (later)
		{
			break;
		}
		event = next;
	}
	InsertQueue(&m_events, newEvent, event);
	return newEvent;
}

uqstl::optional<GameEvent> GameClock::firstEvent() const
{
	const EventHandle event {GetHeadLink(&m_events)};
	if (event == nullptr)
	{
		return uqstl::nullopt;
	}
	const GameEvent result {*lockEvent(m_events, event)};
	UnlockLink(&m_events, event);
	return result;
}

uqstl::vector<GameEvent> GameClock::eventList() const
{
	uqstl::vector<GameEvent> events {};
	for (EventHandle event {GetHeadLink(&m_events)}; event != nullptr;)
	{
		const GameEvent* const eventPtr {lockEvent(m_events, event)};
		events.push_back(*eventPtr);
		const EventHandle next {_GetSuccLink(eventPtr)};
		UnlockLink(&m_events, event);
		event = next;
	}
	return events;
}

bool GameClock::isBefore(uint16_t year, uint16_t month, uint16_t day) const noexcept
{
	return dateIsBefore(m_year, m_month, m_day, year, month, day);
}

bool GameClock::takeDueEvent(GameEventId& funcIndex)
{
	const EventHandle event {GetHeadLink(&m_events)};
	if (event == 0)
	{
		return false;
	}
	const GameEvent* const eventPtr {lockEvent(m_events, event)};
	const bool due {!dateIsBefore(m_year, m_month, m_day, eventPtr->year_index, eventPtr->month_index, eventPtr->day_index)};
	funcIndex = eventPtr->func_index;
	UnlockLink(&m_events, event);
	if (!due)
	{
		return false;
	}
	RemoveQueue(&m_events, event);
	FreeLink(&m_events, event);
	return true;
}

} // namespace uqm
