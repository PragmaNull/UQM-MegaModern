#pragma once
#ifndef UQM_CORE_LOG_LOG_H
#define UQM_CORE_LOG_LOG_H

#include "core/log/logger.h"

//namespace uqmlog = spdlog;
namespace uqm
{
namespace log = ::spdlog;
}

// Disable logging at compile time using these instead of the uqm::log::[debug|trace|etc]
#define UQM_LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define UQM_LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define UQM_LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define UQM_LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define UQM_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define UQM_LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)


namespace error
{
template <typename... Args>
void saveError(uqm::log::format_string_t<Args...> fmt, Args&&... args)
{
	uqm::log::error(fmt, std::forward<Args>(args)...);
}
} // namespace error

#endif // UQM_CORE_LOG_LOG_H
