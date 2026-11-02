############################################################


set(uqm_src_core_stl_master_files "")


############################################################
# Local files
set(uqm_src_core_stl_files
	"src/core/stl/array.h"
	"src/core/stl/bit.h"
	"src/core/stl/atomic.h"
	"src/core/stl/charconv.h"
	"src/core/stl/chrono.h"
	"src/core/stl/concepts.h"
	"src/core/stl/condition_variable.h"
	"src/core/stl/filesystem.h"
	"src/core/stl/fstream.h"
	"src/core/stl/functional.h"
	"src/core/stl/istream.h"
	"src/core/stl/iterator.h"
	"src/core/stl/limits.h"
	"src/core/stl/map.h"
	"src/core/stl/memory.h"
	"src/core/stl/mutex.h"
	"src/core/stl/expected.h"
	"src/core/stl/optional.h"
	"src/core/stl/ranges.h"
	"src/core/stl/regex.h"
	"src/core/stl/ring_buffer.h"
	"src/core/stl/semaphore.h"
	"src/core/stl/set.h"
	"src/core/stl/sstream.h"
	"src/core/stl/stdexcept.h"
	"src/core/stl/stl.cmake"
	"src/core/stl/stl.cpp"
	"src/core/stl/stl.h"
	"src/core/stl/string.h"
	"src/core/stl/system_error.h"
	"src/core/stl/thread.h"
	"src/core/stl/utility.h"
	"src/core/stl/vector.h"
	"src/core/stl/variant.h"
)

source_group("src\\core\\stl" FILES ${uqm_src_core_stl_files})
list(APPEND uqm_src_core_stl_master_files ${uqm_src_core_stl_files})
