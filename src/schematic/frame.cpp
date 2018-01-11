#include "schematic/frame.hpp"
#include "common/lut.hpp"

namespace horizon {

	static const LutEnumStr<Frame::Format> format_lut = {
		{"a4_landscape",	Frame::Format::A4_LANDSCAPE},
		{"a3_landscape",	Frame::Format::A3_LANDSCAPE}
	};

	Frame::Frame() {}
	Frame::Frame(const json &j) :
		format(format_lut.lookup(j.value("format", "a4_landscape")))
		{}

	json Frame::serialize() const {
		json j;
		j["format"] = format_lut.lookup_reverse(format);
		return j;
	}

	static const std::map<Frame::Format,std::pair<uint64_t, uint64_t>> frame_sizes = {
		{Frame::Format::A4_LANDSCAPE, { 297_mm, 210_mm}},
		{Frame::Format::A3_LANDSCAPE, { 420_mm, 297_mm}}
	};

	uint64_t Frame::get_width() const {
		return frame_sizes.at(format).first;
	}

	uint64_t Frame::get_height() const  {
		return frame_sizes.at(format).second;
	}

	std::pair<Coordi, Coordi> Frame::get_bbox() const {
		Coordi a;
		std::tie(a.x, a.y) = frame_sizes.at(format);
		return {Coordi(), a};
	}
}
