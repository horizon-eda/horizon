#include "uuid.hpp"
#include "unit.hpp"
#include "symbol.hpp"
#include "json.hpp"
#include "lut.hpp"
#include "common.hpp"
#include "pool.hpp"
#include <iostream>
#include <fstream>
#include <memory>
#include <gtkmm.h>
#include "canvas/canvas.hpp"
#include "core/core.hpp"
#include "imp.hpp"
#include "imp_symbol.hpp"
#include "imp_schematic.hpp"
#include "imp_padstack.hpp"
#include "imp_package.hpp"
#include "imp_board.hpp"
#include "part.hpp"

using std::cout;
using horizon::UUID;
using horizon::LutEnumStr;
using horizon::Canvas;

using json = nlohmann::json;


int main(int argc, char *argv[]) {
	Glib::OptionContext options;
	options.set_summary("horizon interactive manipulator");
	options.set_help_enabled();

	Glib::OptionGroup group("imp", "imp");

	bool mode_symbol = false;
	Glib::OptionEntry entry;
	entry.set_long_name("symbol");
	entry.set_short_name('y');
	entry.set_description("Symbol mode");
	group.add_entry(entry, mode_symbol);

	bool mode_sch = false;
	Glib::OptionEntry entry2;
	entry2.set_long_name("schematic");
	entry2.set_short_name('c');
	entry2.set_description("Schematic mode");
	group.add_entry(entry2, mode_sch);

	bool mode_padstack = false;
	Glib::OptionEntry entry3;
	entry3.set_long_name("padstack");
	entry3.set_short_name('a');
	entry3.set_description("Padstack mode");
	group.add_entry(entry3, mode_padstack);

	bool mode_package = false;
	Glib::OptionEntry entry4;
	entry4.set_long_name("package");
	entry4.set_short_name('k');
	entry4.set_description("Package mode");
	group.add_entry(entry4, mode_package);

	bool mode_board = false;
	Glib::OptionEntry entry5;
	entry5.set_long_name("board");
	entry5.set_short_name('b');
	entry5.set_description("Board mode");
	group.add_entry(entry5, mode_board);

	std::vector<std::string> filenames;
	Glib::OptionEntry entry_f;
	entry_f.set_long_name(G_OPTION_REMAINING);
	entry_f.set_short_name(' ');
	entry_f.set_description("Filename");
	group.add_entry_filename(entry_f, filenames);

	options.set_main_group(group);
	options.parse(argc, argv);

	auto pool_base_path = Glib::getenv("HORIZON_POOL");

	std::unique_ptr<horizon::ImpBase> imp = nullptr;
	if(mode_sch) {
		imp.reset(new horizon::ImpSchematic(filenames.at(0), filenames.at(1), filenames.at(2), pool_base_path));
	}
	else if(mode_symbol) {
		imp.reset(new horizon::ImpSymbol(filenames.at(0), pool_base_path));
	}
	else if(mode_padstack) {
		imp.reset(new horizon::ImpPadstack(filenames.at(0), pool_base_path));
	}
	else if(mode_package) {
		imp.reset(new horizon::ImpPackage(filenames.at(0), pool_base_path));
	}
	else if(mode_board) {
		imp.reset(new horizon::ImpBoard(filenames.at(0), filenames.at(1), filenames.at(2), filenames.at(3), pool_base_path));
	}
	else {
		std::cout << "wrong invocation" << std::endl;
		return 1;
	}

	imp->run(argc, argv);
	return 0;
}
