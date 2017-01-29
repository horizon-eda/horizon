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
}
