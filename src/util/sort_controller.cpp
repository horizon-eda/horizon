#include "util/sort_controller.hpp"
#include <sstream>

namespace horizon {
	SortController::SortController(Gtk::TreeView *tv):treeview(tv) {
		tv->set_headers_clickable();
	}

	void SortController::add_column(unsigned int index, const std::string &name) {
		columns.insert({index, {name, Sort::NONE}});
		treeview->get_column(index)->signal_clicked().connect(sigc::bind<unsigned int>(sigc::mem_fun(this, &SortController::handle_click), index));
		update_treeview();
	}

	void SortController::set_simple(bool s) {
		is_simple = s;
		update_treeview();
	}

	void SortController::update_treeview() {
		for(const auto &it: columns) {
			treeview->get_column(it.first)->set_sort_indicator(it.second.second!=Sort::NONE);
			treeview->get_column(it.first)->set_sort_order(it.second.second==Sort::ASC?Gtk::SORT_ASCENDING:Gtk::SORT_DESCENDING);
		}
	}

	void SortController::handle_click(unsigned int index) {
		for(auto &it: columns) {
			auto &c = it.second;
			if(it.first == index) {
				if(is_simple) {
					switch(c.second) {
						case Sort::ASC: c.second = Sort::DESC; break;
						case Sort::DESC: c.second = Sort::ASC; break;
						case Sort::NONE: c.second = Sort::ASC; break;
					}
				}
				else {
					switch(c.second) {
						case Sort::ASC: c.second = Sort::DESC; break;
						case Sort::DESC: c.second = Sort::NONE; break;
						case Sort::NONE: c.second = Sort::ASC; break;
					}
				}
			}
			else if(is_simple){
				c.second = Sort::NONE;
			}
		}

		update_treeview();
		s_signal_changed.emit();
	}

	void SortController::set_sort(unsigned int index, Sort s) {
		for(auto &it: columns) {
			auto &c = it.second;
			if(it.first == index) {
				c.second = s;
			}
			else if(is_simple){
				c.second = Sort::NONE;
			}
		}

		update_treeview();
		s_signal_changed.emit();
	}

	std::string SortController::get_order_by() const {
		if(is_simple) {
			auto x = std::find_if(columns.cbegin(), columns.cend(), [](const auto &a){return a.second.second != Sort::NONE;});
			if(x == columns.cend()) {
				return "";
			}
			else {
				std::stringstream ss;
				ss << " ORDER BY " << x->second.first << " COLLATE naturalCompare ";
				if(x->second.second == Sort::ASC) {
					ss << "ASC";
				}
				else {
					ss << "DESC";
				}
				return ss.str();
			}
		}
		else {
			return "";
		}
	}

}
