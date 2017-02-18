#include "util.hpp"
#include "json.hpp"
#include <fstream>
#include <unistd.h>
#include <glibmm/miscutils.h>
#ifdef G_OS_WIN32
	#include <windows.h>
#endif


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

	#ifdef G_OS_WIN32
	std::string get_exe_dir() {
		TCHAR szPath[MAX_PATH];
		if(!GetModuleFileName(NULL, szPath, MAX_PATH)) {
			throw std::runtime_error("can't find executable");
			return "";
        }
		else {
			return Glib::path_get_dirname(szPath);
		}
	}

	#else
	std::string get_exe_dir() {

		char buf[1024];
		ssize_t len;
        if((len = readlink("/proc/self/exe", buf, sizeof(buf)-1)) != -1) {
        	buf[len] = '\0';
        	return Glib::path_get_dirname(buf);
        }
        else {
        	throw std::runtime_error("can't find executable");
			return "";
        }
	}
	#endif
}
