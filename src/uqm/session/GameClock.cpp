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
	static constexpr uint8_t c_daysInMonth[12] {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (month == 2 && isLeapYear(year))
	{
		return 29;
	}
	return c_daysInMonth[month - 1];
}

// True if the first date is before the second.
[[nodiscard]] bool isBefore(uint16_t year, uint16_t month, uint16_t day, uint16_t otherYear, uint16_t otherMonth,
							uint16_t otherDay)
{
	return year < otherYear || (year == otherYear && (month < otherMonth || (month == otherMonth && day < otherDay)));
}

[[nodiscard]] EVENT* lockEvent(QUEUE& events, HEVENT event)
{
	// An EVENT starts with the LINK fields.
	return reinterpret_cast<EVENT*>(LockLink(&events, event));
}

} // namespace

bool GameClock::start(uint16_t eventCapacity)
{
	if (!InitQueue(&m_events, eventCapacity, sizeof(EVENT)))
	{
		return false;
	}
	restore(c_startDay, c_startMonth, START_YEAR, 0, 0);
	return true;
}

void GameClock::stop()
{
	UninitQueue(&m_events);
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
	for (uint16_t year {START_YEAR}; year < m_year; ++year)
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

bool GameClock::resolveDate(EVENT_TYPE type, uint16_t& month, uint16_t& day, uint16_t& year) const
{
	if (type == RELATIVE_EVENT)
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
	return !isBefore(year, month, day, m_year, m_month, m_day);
}

HEVENT GameClock::addEvent(uint16_t month, uint16_t day, uint16_t year, uint8_t funcIndex)
{
	const HEVENT newEvent {AllocLink(&m_events)};
	if (!newEvent)
	{
		return 0;
	}
	EVENT* const newPtr {lockEvent(m_events, newEvent)};
	newPtr->day_index = static_cast<uint8_t>(day);
	newPtr->month_index = static_cast<uint8_t>(month);
	newPtr->year_index = year;
	newPtr->func_index = funcIndex;
	UnlockLink(&m_events, newEvent);

	HEVENT event {GetHeadLink(&m_events)};
	while (event != 0)
	{
		EVENT* const eventPtr {lockEvent(m_events, event)};
		const bool later {isBefore(year, month, day, eventPtr->year_index, eventPtr->month_index, eventPtr->day_index)};
		const HEVENT next {_GetSuccLink(eventPtr)};
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

bool GameClock::takeDueEvent(uint8_t& funcIndex)
{
	const HEVENT event {GetHeadLink(&m_events)};
	if (event == 0)
	{
		return false;
	}
	EVENT* const eventPtr {lockEvent(m_events, event)};
	const bool due {eventPtr->day_index == m_day && eventPtr->month_index == m_month && eventPtr->year_index == m_year};
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
