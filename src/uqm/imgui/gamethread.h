// Runs work from the ImGui windows on the game thread.

#pragma once
#ifndef UQM_IMGUI_GAMETHREAD_H
#define UQM_IMGUI_GAMETHREAD_H

#include "core/stl/functional.h"
#include "core/stl/optional.h"
#include "core/stl/utility.h"
#include "core/thread/Mutex.h"


namespace uqm
{
// Runs the task on the game thread the next time that it processes callbacks.
void runOnGameThread(uqstl::function<void()> task);

// The result of work that a window on the main thread runs on the game thread. The object must outlive the work.
template <typename T>
class GameThreadResult
{
public:
	// Runs the work on the game thread. The result replaces a result that is not taken yet.
	void start(uqstl::function<T()> work)
	{
		m_pending = true;
		runOnGameThread([this, work {uqstl::move(work)}] {
			T result {work()};
			const LockGuard guard {m_lock};
			m_result = uqstl::move(result);
		});
	}

	// True from start() until take() gives the result.
	[[nodiscard]] bool pending() const noexcept
	{
		return m_pending;
	}

	// The result, or nullopt if the work has not finished.
	[[nodiscard]] uqstl::optional<T> take()
	{
		const LockGuard guard {m_lock};
		if (!m_result)
		{
			return uqstl::nullopt;
		}
		m_pending = false;
		return uqstl::exchange(m_result, uqstl::nullopt);
	}

private:
	Mutex m_lock {};
	uqstl::optional<T> m_result UQM_GUARDED_BY(m_lock) {};
	bool m_pending {};
};

} // namespace uqm


#endif // UQM_IMGUI_GAMETHREAD_H
