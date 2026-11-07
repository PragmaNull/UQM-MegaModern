#pragma once
#ifndef UQM_CORE_LOG_LOGGER_H
#define UQM_CORE_LOG_LOGGER_H

// set the global log level limit.
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>

#include "core/stl/stl.h"

namespace uqm
{

class Logger
{
public:
	Logger();
	~Logger();
	static Logger& getInstance();

	void init(uqstl::string_view logfile);
	void showLogOnExit(bool showLog, bool errorEncountered)
	{
		m_displayBacktraceOnExit = showLog;
		m_exitDueToError = errorEncountered;
	}

protected:
	void displayBacktraceDialogue() const;

private:
	bool m_displayBacktraceOnExit {false};
	bool m_exitDueToError {false};
	spdlog::sink_ptr m_backtraceBufferSink {};
	static Logger* s_instance;
};

} // namespace uqm

#endif // UQM_CORE_LOG_LOGGER_H
