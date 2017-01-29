#include "frame.hpp"

namespace horizon {
	Frame::Frame() {}
	Frame::Frame(const json &j){}

	static const std::map<Frame::Format,std::pair<uint64_t, uint64_t>> frame_sizes = {
		{Frame::Format::A4_LANDSCAPE, { 297_mm, 210_mm}}
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
