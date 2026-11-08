// Reads animation files: the JSON files that list the images of the frames of a drawable.
#include "anifile.h"

#include "core/log/log.h"
#include "libs/file/json.h"

namespace uqm
{

namespace
{

// Sets the fields of 'frame' that 'obj' has. Returns false if a field is not valid.
bool readFields(yyjson_val* obj, AniFrame& frame)
{
	if (yyjson_val* const transparency {yyjson_obj_get(obj, "transparency")})
	{
		if (yyjson_equals_str(transparency, "png"))
		{
			frame.transparentColor = c_aniTransparencyPng;
		}
		else if (yyjson_equals_str(transparency, "none"))
		{
			frame.transparentColor = c_aniTransparencyNone;
		}
		else if (const auto index {jsonInt<int32_t>(transparency, 0, UINT8_MAX)})
		{
			frame.transparentColor = *index;
		}
		else
		{
			return false;
		}
	}
	if (yyjson_val* const colormap {yyjson_obj_get(obj, "colormap")})
	{
		if (yyjson_is_null(colormap))
		{
			frame.colormapIndex = c_aniNoColormap;
		}
		else if (const auto index {jsonInt<int32_t>(colormap, 0)})
		{
			frame.colormapIndex = *index;
		}
		else
		{
			return false;
		}
	}
	if (yyjson_val* const hotspot {yyjson_obj_get(obj, "hotspot")})
	{
		const auto x {jsonInt<int32_t>(yyjson_arr_get(hotspot, 0))};
		const auto y {jsonInt<int32_t>(yyjson_arr_get(hotspot, 1))};
		if (yyjson_arr_size(hotspot) != 2 || !x || !y)
		{
			return false;
		}
		frame.hotspotX = *x;
		frame.hotspotY = *y;
	}
	return true;
}

} // namespace

uqstl::optional<uqstl::vector<AniFrame>> aniFromJson(uqstl::string json)
{
	static constexpr uqstl::string_view c_what {"Animation file"};
	const JsonText doc {uqstl::move(json), c_what};
	yyjson_val* const frames {versionedMember(doc, c_aniFileVersion, "frames", YYJSON_TYPE_ARR, c_what)};
	if (frames == nullptr)
	{
		return uqstl::nullopt;
	}
	AniFrame defaultFrame {};
	yyjson_val* const defaults {yyjson_obj_get(yyjson_doc_get_root(doc.get()), "defaults")};
	if (defaults != nullptr && (!yyjson_is_obj(defaults) || !readFields(defaults, defaultFrame)))
	{
		uqm::log::error("{}: the defaults are not valid.", c_what);
		return uqstl::nullopt;
	}

	uqstl::vector<AniFrame> result {};
	size_t index {};
	size_t max {};
	yyjson_val* frameObj {};
	yyjson_arr_foreach(frames, index, max, frameObj)
	{
		AniFrame frame {defaultFrame};
		yyjson_val* const image {yyjson_obj_get(frameObj, "image")};
		if (!yyjson_is_str(image) || !readFields(frameObj, frame))
		{
			uqm::log::error("{}: frame {} is not valid.", c_what, index);
			return uqstl::nullopt;
		}
		frame.image = jsonString(image);
		result.push_back(uqstl::move(frame));
	}
	return result;
}

} // namespace uqm
