// Reads the alignment and padding markers ("|n|" and ":n:") at the start of display strings.
#include "libs/strlib.h"

#include "core/log/log.h"
#include "uqm/units.h"

#include <charconv>

namespace
{

// Reads "<delimiter>n<delimiter>" at the start of 'str' and adds RES_SCALE(n) to 'offset'.
// Returns the text after the marker, or 'str' if it has no valid marker.
const char* skipMarker(const char* str, char delimiter, int16_t* offset)
{
	if (str == nullptr || uqm::utf8CharCount(str, static_cast<uqm::UniChar>(delimiter)) != 2)
	{
		return str;
	}

	const uqstl::string_view text {str};
	const size_t end {text.find(delimiter, 1)};
	if (text[0] != delimiter || end == uqstl::string_view::npos)
	{
		return str;
	}

	int value {};
	const uqstl::string_view number {text.substr(1, end - 1)};
	const auto [last, error] {uqstl::from_chars(number.data(), number.data() + number.size(), value)};
	if (error != uqstl::errc {} || last != number.data() + number.size())
	{
		uqm::log::debug("Variable between delimiters is missing, corrupt, or not an integer: {}", text);
		return str;
	}

	if (value != 0)
	{
		*offset += RES_SCALE(value);
	}
	return str + end + 1;
}

} // namespace

const char* AlignText(const char* str, int16_t* loc_x)
{
	return skipMarker(str, '|', loc_x);
}

const char* AddPadd(const char* str, int16_t* padding)
{
	return skipMarker(str, ':', padding);
}
