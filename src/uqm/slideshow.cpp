// Reads slideshow files: the JSON files with the commands of the cutscenes.
#include "slideshow.h"

#include "core/log/log.h"
#include "core/stl/limits.h"
#include "core/stl/ranges.h"

#include <yyjson.h>

namespace uqm
{

namespace
{

struct OpName
{
	uqstl::string_view name;
	SlideOp op;
};

constexpr uqstl::array c_opNames {
	OpName {"dims",				SlideOp::Dims			 },
	OpName {"font",				SlideOp::Font			 },
	OpName {"ani",			   SlideOp::Ani			   },
	OpName {"music",			 SlideOp::Music			   },
	OpName {"ditty",			 SlideOp::Ditty			   },
	OpName {"wait",				SlideOp::Wait			 },
	OpName {"waitDitty",		 SlideOp::WaitDitty		   },
	OpName {"spinWait",			SlideOp::SpinWait		 },
	OpName {"sync",				SlideOp::Sync			 },
	OpName {"resync",			  SlideOp::Resync			 },
	OpName {"deltaSync",		 SlideOp::DeltaSync		   },
	OpName {"backgroundColor",   SlideOp::BackgroundColor  },
	OpName {"textColor",		 SlideOp::TextColor		   },
	OpName {"textBackColor",	 SlideOp::TextBackColor	   },
	OpName {"textFadeColor",	 SlideOp::TextFadeColor	   },
	OpName {"textAlign",		 SlideOp::TextAlign		   },
	OpName {"textEffect",		  SlideOp::TextEffect		 },
	OpName {"text",				SlideOp::Text			 },
	OpName {"textSpin",			SlideOp::TextSpin		 },
	OpName {"spinStat",			SlideOp::SpinStat		 },
	OpName {"textFadeIn",		  SlideOp::TextFadeIn		 },
	OpName {"textFadeOut",	   SlideOp::TextFadeOut	   },
	OpName {"saveBackground",	  SlideOp::SaveBackground	 },
	OpName {"restoreBackground", SlideOp::RestoreBackground},
	OpName {"draw",				SlideOp::Draw			 },
	OpName {"drawSis",		   SlideOp::DrawSis		   },
	OpName {"batch",			 SlideOp::Batch			   },
	OpName {"unbatch",		   SlideOp::Unbatch		   },
	OpName {"fadeToColor",	   SlideOp::FadeToColor	   },
	OpName {"fadeToBlack",	   SlideOp::FadeToBlack	   },
	OpName {"fadeToWhite",	   SlideOp::FadeToWhite	   },
	OpName {"clearScreen",	   SlideOp::ClearScreen	   },
	OpName {"call",				SlideOp::Call			 },
	OpName {"line",				SlideOp::Line			 },
	OpName {"lineSpin",			SlideOp::LineSpin		 },
	OpName {"getRect",		   SlideOp::GetRect		   },
	OpName {"statBox",		   SlideOp::StatBox		   },
	OpName {"movie",			 SlideOp::Movie			   },
	OpName {"animate",		   SlideOp::Animate		   },
};

inline constexpr size_t c_maxFadeInLines {15};

// Reads the fields of one command. A missing or bad field clears 'm_ok'.
class FieldReader
{
public:
	explicit FieldReader(yyjson_val* obj)
		: m_obj {obj}
	{
	}

	[[nodiscard]] bool ok() const
	{
		return m_ok;
	}

	[[nodiscard]] bool has(const char* name) const
	{
		return yyjson_obj_get(m_obj, name) != nullptr;
	}

	int32_t number(const char* name, int32_t min = INT16_MIN, int32_t max = INT16_MAX)
	{
		yyjson_val* const value {yyjson_obj_get(m_obj, name)};
		if (!yyjson_is_int(value) || yyjson_get_sint(value) < min || yyjson_get_sint(value) > max)
		{
			m_ok = false;
			return 0;
		}
		return static_cast<int32_t>(yyjson_get_sint(value));
	}

	uqstl::string text(const char* name)
	{
		yyjson_val* const value {yyjson_obj_get(m_obj, name)};
		if (!yyjson_is_str(value))
		{
			m_ok = false;
			return {};
		}
		return {yyjson_get_str(value), yyjson_get_len(value)};
	}

	// Returns the index of the value of 'name' in 'choices'.
	size_t choice(const char* name, uqstl::span<const uqstl::string_view> choices)
	{
		const uqstl::string value {text(name)};
		for (size_t i {0}; i < choices.size(); ++i)
		{
			if (value == choices[i])
			{
				return i;
			}
		}
		m_ok = false;
		return 0;
	}

	uqstl::array<uint8_t, 3> color(const char* name)
	{
		yyjson_val* const value {yyjson_obj_get(m_obj, name)};
		uqstl::array<uint8_t, 3> result {};
		if (yyjson_arr_size(value) != result.size())
		{
			m_ok = false;
			return result;
		}
		for (size_t i {0}; i < result.size(); ++i)
		{
			yyjson_val* const component {yyjson_arr_get(value, i)};
			if (!yyjson_is_uint(component) || yyjson_get_uint(component) > UINT8_MAX)
			{
				m_ok = false;
				return result;
			}
			result[i] = static_cast<uint8_t>(yyjson_get_uint(component));
		}
		return result;
	}

	uqstl::vector<uqstl::string> lines(const char* name, size_t maxCount)
	{
		yyjson_val* const value {yyjson_obj_get(m_obj, name)};
		uqstl::vector<uqstl::string> result {};
		const size_t count {yyjson_arr_size(value)};
		if (!yyjson_is_arr(value) || count == 0 || count > maxCount)
		{
			m_ok = false;
			return result;
		}
		size_t index {};
		size_t max {};
		yyjson_val* line {};
		yyjson_arr_foreach(value, index, max, line)
		{
			if (!yyjson_is_str(line))
			{
				m_ok = false;
				return result;
			}
			result.emplace_back(yyjson_get_str(line), yyjson_get_len(line));
		}
		return result;
	}

	void fail()
	{
		m_ok = false;
	}

private:
	yyjson_val* m_obj {};
	bool m_ok {true};
};

// Reads the optional position, scale and angle of a draw command. A scale needs a position, and an angle needs a scale.
void readPlacement(FieldReader& reader, SlideCommand& command)
{
	command.hasPosition = reader.has("x") || reader.has("y");
	if (command.hasPosition)
	{
		command.x = reader.number("x");
		command.y = reader.number("y");
	}
	if (reader.has("scale"))
	{
		command.scale = reader.number("scale", 0, INT32_MAX);
		if (!command.hasPosition)
		{
			reader.fail();
		}
	}
	if (reader.has("angle"))
	{
		command.angle = reader.number("angle");
		if (!command.scale)
		{
			reader.fail();
		}
	}
}

uqstl::optional<SlideCommand> readCommand(yyjson_val* obj)
{
	const char* const opName {yyjson_get_str(yyjson_obj_get(obj, "op"))};
	if (opName == nullptr)
	{
		return uqstl::nullopt;
	}
	const auto found {uqstl::ranges::find(c_opNames, uqstl::string_view {opName}, &OpName::name)};
	if (found == c_opNames.end())
	{
		return uqstl::nullopt;
	}

	SlideCommand command {};
	command.op = found->op;
	FieldReader reader {obj};
	if (reader.has("resolution"))
	{
		static constexpr uqstl::array<uqstl::string_view, 2> c_resolutions {"sd", "hd"};
		command.resolution = reader.choice("resolution", c_resolutions) == 0 ? SlideResolution::Sd : SlideResolution::Hd;
	}

	switch (command.op)
	{
		case SlideOp::Dims:
			command.width = reader.number("width", 1);
			command.height = reader.number("height", 1);
			break;
		case SlideOp::Font:
			command.index = reader.number("index", 0, c_slideshowMaxFonts - 1);
			if (reader.has("file"))
			{
				command.text = reader.text("file");
			}
			break;
		case SlideOp::Ani:
		case SlideOp::Music:
		case SlideOp::Call:
			command.text = reader.text("file");
			break;
		case SlideOp::Ditty:
			command.text = reader.text("ship");
			break;
		case SlideOp::Wait:
		case SlideOp::SpinWait:
		case SlideOp::Sync:
		case SlideOp::DeltaSync:
		case SlideOp::FadeToColor:
		case SlideOp::FadeToBlack:
		case SlideOp::FadeToWhite:
			command.ms = reader.number("ms", 0, INT32_MAX);
			break;
		case SlideOp::BackgroundColor:
		case SlideOp::TextColor:
		case SlideOp::TextBackColor:
		case SlideOp::TextFadeColor:
			command.color = reader.color("color");
			break;
		case SlideOp::TextAlign:
			{
				static constexpr uqstl::array<uqstl::string_view, 3> c_aligns {"top", "middle", "bottom"};
				command.align = static_cast<SlideTextAlign>(reader.choice("align", c_aligns));
				break;
			}
		case SlideOp::TextEffect:
			{
				static constexpr uqstl::array<uqstl::string_view, 2> c_effects {"plain", "traced"};
				command.traced = reader.choice("effect", c_effects) == 1;
				break;
			}
		case SlideOp::Text:
		case SlideOp::TextSpin:
			command.x = reader.number("x");
			command.y = reader.number("y");
			command.text = reader.text("text");
			break;
		case SlideOp::SpinStat:
			command.filled = reader.number("filled", 0);
			command.empty = reader.number("empty", 0);
			command.text = reader.text("text");
			break;
		case SlideOp::TextFadeIn:
			command.lines = reader.lines("lines", c_maxFadeInLines);
			break;
		case SlideOp::Draw:
			command.index = reader.number("frame", 0, UINT16_MAX);
			readPlacement(reader, command);
			break;
		case SlideOp::DrawSis:
			readPlacement(reader, command);
			break;
		case SlideOp::Line:
		case SlideOp::LineSpin:
			command.x = reader.number("x1");
			command.y = reader.number("y1");
			command.x2 = reader.number("x2");
			command.y2 = reader.number("y2");
			break;
		case SlideOp::StatBox:
			command.x = reader.number("x");
			command.y = reader.number("y");
			break;
		case SlideOp::Movie:
			command.fps = reader.number("fps", 1);
			command.index = reader.number("from", 0, UINT16_MAX);
			command.lastIndex = reader.number("to", command.index, UINT16_MAX);
			break;
		case SlideOp::Animate:
			command.index = reader.number("first", 0, UINT16_MAX);
			command.lastIndex = reader.number("last", 0, UINT16_MAX);
			command.loops = reader.number("loops", 0);
			command.ms = reader.number("ms", 0, INT32_MAX);
			command.fps = reader.number("fps", 1);
			break;
		case SlideOp::WaitDitty:
		case SlideOp::Resync:
		case SlideOp::TextFadeOut:
		case SlideOp::SaveBackground:
		case SlideOp::RestoreBackground:
		case SlideOp::Batch:
		case SlideOp::Unbatch:
		case SlideOp::ClearScreen:
		case SlideOp::GetRect:
			break;
	}
	if (!reader.ok())
	{
		return uqstl::nullopt;
	}
	return command;
}

} // namespace

uqstl::optional<uqstl::vector<SlideCommand>> slideshowFromJson(uqstl::string_view json)
{
	yyjson_read_err err {};
	yyjson_doc* doc {yyjson_read_opts(const_cast<char*>(json.data()), json.size(), YYJSON_READ_NOFLAG, nullptr, &err)};
	if (doc == nullptr)
	{
		uqm::log::error("Slideshow: JSON parse error at byte {}: {}", err.pos, err.msg);
		return uqstl::nullopt;
	}

	uqstl::optional<uqstl::vector<SlideCommand>> result {};
	yyjson_val* const root {yyjson_doc_get_root(doc)};
	yyjson_val* const commands {yyjson_obj_get(root, "commands")};
	if (yyjson_get_int(yyjson_obj_get(root, "version")) != c_slideshowVersion || !yyjson_is_arr(commands))
	{
		uqm::log::error("Slideshow: the file does not have the expected structure.");
	}
	else
	{
		result.emplace();
		size_t index {};
		size_t max {};
		yyjson_val* obj {};
		yyjson_arr_foreach(commands, index, max, obj)
		{
			auto command {readCommand(obj)};
			if (!command)
			{
				uqm::log::error("Slideshow: command {} is not valid.", index);
				result.reset();
				break;
			}
			result->push_back(uqstl::move(*command));
		}
	}
	yyjson_doc_free(doc);
	return result;
}

} // namespace uqm
