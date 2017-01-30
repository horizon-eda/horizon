#include "util.hpp"
#include "json.hpp"
#include <fstream>


namespace horizon {
	void save_json_to_file(const std::string &filename, const json &j) {
		std::ofstream ofs(filename);
		if(!ofs.is_open()) {
			std::cout << "can't save json " << filename <<std::endl;
			return;
		}
		ofs << std::setw(4) << j;
		ofs.close();
	}

	int orientation_to_angle(Orientation o) {
		int angle=0;
		switch(o) {
			case Orientation::RIGHT: angle=0; break;
			case Orientation::LEFT: angle=32768; break;
			case Orientation::UP: angle=16384; break;
			case Orientation::DOWN: angle=49152; break;
		}
		return angle;
	}
}
