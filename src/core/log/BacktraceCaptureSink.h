#pragma once
#ifndef UQM_CORE_LOG_BACKTRACECAPTURESINK_H_
#define UQM_CORE_LOG_BACKTRACECAPTURESINK_H_

#include <spdlog/async.h>
#include <spdlog/sinks/base_sink.h>

#include "core/stl/bit.h"
#include "core/stl/stl.h"

namespace uqm
{

constexpr size_t BacktraceSize {64};

class BacktraceCaptureSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
	BacktraceCaptureSink();

	void getBufferLines(uqstl::string& dest) const;

protected:
	void sink_it_(const spdlog::details::log_msg& msg) final;
	void flush_() final {}

	spdlog::memory_buf_t& at(const size_t idx)
	{
		return m_messages[logicalToBufferIndex(idx)];
	}

	const spdlog::memory_buf_t& at(const size_t idx) const
	{
		return m_messages[logicalToBufferIndex(idx)];
	}

	size_t getIndex() const { return m_index; }

	size_t logicalToBufferIndex(const size_t logical) const
	{
		if constexpr (uqstl::has_single_bit(BacktraceSize))
		{
			return logical & (BacktraceSize - 1);
		}

		return logical % BacktraceSize;
	}

private:
	uqstl::array<spdlog::memory_buf_t, BacktraceSize> m_messages;
	size_t m_index {};
};

} // namespace uqm
#endif /* UQM_CORE_LOG_BACKTRACECAPTURESINK_H_ */