// Reads slideshow files: the JSON files with the commands of the cutscenes.
#include "slideshow.h"

#include "core/log/log.h"
#include "libs/file/json.h"

namespace uqm
{

namespace
{

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
		const auto value {jsonInt<int32_t>(yyjson_obj_get(m_obj, name), min, max)};
		m_ok = m_ok && value.has_value();
		return value.value_or(0);
	}

	uqstl::string text(const char* name)
	{
		yyjson_val* const value {yyjson_obj_get(m_obj, name)};
		m_ok = m_ok && yyjson_is_str(value);
		return jsonString(value);
	}

	template <typename E>
	E choice(const char* name)
	{
		const auto value {enumFromJsonName<E>(text(name))};
		m_ok = m_ok && value.has_value();
		return value.value_or(E {});
	}

	uqstl::array<uint8_t, 3> color(const char* name)
	{
		yyjson_val* const value {yyjson_obj_get(m_obj, name)};
		uqstl::array<uint8_t, 3> result {};
		m_ok = m_ok && yyjson_arr_size(value) == result.size();
		for (size_t i {0}; i < result.size() && m_ok; ++i)
		{
			const auto component {jsonInt<uint8_t>(yyjson_arr_get(value, i))};
			m_ok = component.has_value();
			result[i] = component.value_or(0);
		}
		return result;
	}

	uqstl::vector<uqstl::string> lines(const char* name, size_t maxCount)
	{
		auto value {jsonStringArray(yyjson_obj_get(m_obj, name), maxCount)};
		m_ok = m_ok && value && !value->empty();
		return value.value_or(uqstl::vector<uqstl::string> {});
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
	const bool hasPosition {reader.has("x") || reader.has("y")};
	if (hasPosition)
	{
		command.x = reader.number("x");
		command.y = reader.number("y");
	}
	if (reader.has("scale"))
	{
		command.scale = reader.number("scale", 0, INT32_MAX);
		if (!hasPosition)
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
	const auto op {enumFromJsonName<SlideOp>(jsonString(yyjson_obj_get(obj, "op")))};
	if (!op)
	{
		return uqstl::nullopt;
	}

	SlideCommand command {};
	command.op = *op;
	FieldReader reader {obj};
	if (reader.has("resolution"))
	{
		command.resolution = reader.choice<SlideResolution>("resolution");
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
			command.align = reader.choice<SlideTextAlign>("align");
			break;
		case SlideOp::TextEffect:
			{
				const uqstl::string effect {reader.text("effect")};
				command.traced = effect == "traced";
				if (!command.traced && effect != "plain")
				{
					reader.fail();
				}
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
			command.lines = reader.lines("lines", c_slideshowMaxFadeInLines);
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
			command.index = reader.number("first", 0, UINT16_MAX - 1);
			command.lastIndex = reader.number("last", command.index + 1, UINT16_MAX);
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
	static constexpr uqstl::string_view c_what {"Slideshow"};
	const JsonDoc doc {parseJson(json, c_what)};
	yyjson_val* const commands {versionedMember(doc, c_slideshowVersion, "commands", YYJSON_TYPE_ARR, c_what)};
	if (commands == nullptr)
	{
		return uqstl::nullopt;
	}
	uqstl::vector<SlideCommand> result {};
	size_t index {};
	size_t max {};
	yyjson_val* obj {};
	yyjson_arr_foreach(commands, index, max, obj)
	{
		auto command {readCommand(obj)};
		if (!command)
		{
			uqm::log::error("{}: command {} is not valid.", c_what, index);
			return uqstl::nullopt;
		}
		result.push_back(uqstl::move(*command));
	}
	return result;
}

} // namespace uqm
