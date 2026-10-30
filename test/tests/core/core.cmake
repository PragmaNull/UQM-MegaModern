############################################################


set(uqm_tests_core_master_files "")

include("../src/core/string/string.cmake")
list(APPEND uqm_tests_core_master_files 
	"../src/core/log/BacktraceCaptureSink.h"
	"../src/core/log/BacktraceCaptureSink.cpp"
	"../src/core/stl/ring_buffer.h"
	"../src/core/string/StringUtils.h"
	"../src/core/string/StringUtils.cpp"
	"../src/core/string/Utf8.h"
	"../src/core/string/Utf8.cpp"
	"../src/core/thread/Mutex.h"
	"../src/core/thread/Thread.cpp"
	"../src/core/thread/Thread.h"
	"../src/core/thread/ThreadSafety.h"
)

############################################################
# Local files
set(uqm_tests_core_files
	"tests/core/test_log_BacktraceCaptureSink.cpp"
	"tests/core/test_StringUtils.cpp"
	"tests/core/test_Utf8.cpp"
	"tests/core/test_ring_buffer.cpp"
	"tests/core/test_thread.cpp"
)

source_group("tests\\core" FILES ${uqm_tests_core_files})
list(APPEND uqm_tests_core_master_files ${uqm_tests_core_files})
