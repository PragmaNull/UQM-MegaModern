// Reads animation files: the JSON files that list the images of the frames of a drawable.
#include "anifile.h"

#include "core/log/log.h"

#include <yyjson.h>

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
		else if (yyjson_is_uint(transparency) && yyjson_get_uint(transparency) <= 255)
		{
			frame.transparentColor = static_cast<int32_t>(yyjson_get_uint(transparency));
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
		else if (yyjson_is_uint(colormap) && yyjson_get_uint(colormap) <= INT32_MAX)
		{
			frame.colormapIndex = static_cast<int32_t>(yyjson_get_uint(colormap));
		}
		else
		{
			return false;
		}
	}
	if (yyjson_val* const hotspot {yyjson_obj_get(obj, "hotspot")})
	{
		yyjson_val* const x {yyjson_arr_get(hotspot, 0)};
		yyjson_val* const y {yyjson_arr_get(hotspot, 1)};
		if (yyjson_arr_size(hotspot) != 2 || !yyjson_is_int(x) || !yyjson_is_int(y))
		{
			return false;
		}
		frame.hotspotX = static_cast<int32_t>(yyjson_get_int(x));
		frame.hotspotY = static_cast<int32_t>(yyjson_get_int(y));
	}
	return true;
}

} // namespace

uqstl::optional<uqstl::vector<AniFrame>> aniFromJson(uqstl::string_view json)
{
	yyjson_read_err err {};
	yyjson_doc* doc {yyjson_read_opts(const_cast<char*>(json.data()), json.size(), YYJSON_READ_NOFLAG, nullptr, &err)};
	if (doc == nullptr)
	{
		uqm::log::error("Animation file: JSON parse error at byte {}: {}", err.pos, err.msg);
		return uqstl::nullopt;
	}

	uqstl::optional<uqstl::vector<AniFrame>> result {};
	yyjson_val* const root {yyjson_doc_get_root(doc)};
	yyjson_val* const defaults {yyjson_obj_get(root, "defaults")};
	yyjson_val* const frames {yyjson_obj_get(root, "frames")};
	AniFrame defaultFrame {};
	if (yyjson_get_int(yyjson_obj_get(root, "version")) != c_aniFileVersion || !yyjson_is_arr(frames)
		|| (defaults != nullptr && (!yyjson_is_obj(defaults) || !readFields(defaults, defaultFrame))))
	{
		uqm::log::error("Animation file: the file does not have the expected structure.");
	}
	else
	{
		result.emplace();
		size_t index {};
		size_t max {};
		yyjson_val* frameObj {};
		yyjson_arr_foreach(frames, index, max, frameObj)
		{
			AniFrame frame {defaultFrame};
			yyjson_val* const image {yyjson_obj_get(frameObj, "image")};
			if (!yyjson_is_str(image) || !readFields(frameObj, frame))
			{
				uqm::log::error("Animation file: frame {} is not valid.", index);
				result.reset();
				break;
			}
			frame.image.assign(yyjson_get_str(image), yyjson_get_len(image));
			result->push_back(uqstl::move(frame));
		}
	}
	yyjson_doc_free(doc);
	return result;
}

} // namespace uqm
