#include "logger.h"

#include <spdlog/async.h>

#include <spdlog/sinks/basic_file_sink.h>
#include "spdlog/sinks/stdout_color_sinks.h"

#include "core/log/BacktraceCaptureSink.h"
#include "core/stl/stl.h"
#include "core/stl/ring_buffer.h"

namespace uqm
{

//////////////////////////////////////////////////////////////////////////////
Logger* Logger::s_instance {};

Logger::Logger()
{
	assert(s_instance == nullptr);
	s_instance = this;
}

Logger::~Logger()
{
	assert(s_instance == this);
	s_instance = nullptr;

	if (m_displayBacktraceOnExit)
	{
		displayBacktraceDialogue();
	}
}

Logger& Logger::getInstance()
{
	assert(s_instance != nullptr);
	return *s_instance;
}

void Logger::init(uqstl::string_view logfile)
{
	try
	{
		spdlog::init_thread_pool(8192, 1);
		const spdlog::sink_ptr stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#if defined(DEBUG)
		stdoutSink->set_level(spdlog::level::debug);
#else
		stdoutSink->set_level(spdlog::level::info);
#endif
		stdoutSink->set_pattern("[%^%l%$] %v");

		const spdlog::filename_t logFileName {logfile};
		const spdlog::sink_ptr fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFileName, true);
		fileSink->set_level(spdlog::level::trace);
		fileSink->set_pattern("[%Y-%b-%d %T.%e] [%l] %v");

		m_backtraceBufferSink = {std::make_shared<BacktraceCaptureSink>()};

		std::vector<spdlog::sink_ptr> sinks {stdoutSink, fileSink, m_backtraceBufferSink};
		auto logger = std::make_shared<spdlog::async_logger>("UQMLogger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
		//spdlog::register_logger(logger);
		spdlog::set_default_logger(logger);

		spdlog::enable_backtrace(BacktraceSize);
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		fmt::print(stderr, "Log initialization failed: {}\n", ex.what());
	}
}

void Logger::displayBacktraceDialogue() const
{
	if (m_backtraceBufferSink)
	{
		// TODO: string builder
		uqstl::string msg {};
		dynamic_cast<BacktraceCaptureSink*>(m_backtraceBufferSink.get())->getBufferLines(msg);
	}
}

} // namespace uqm