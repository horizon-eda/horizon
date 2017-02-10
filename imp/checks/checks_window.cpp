#include "checks_window.hpp"
#include "checks/check_runner.hpp"
#include "check_settings_dialog.hpp"
#include "widgets/cell_renderer_layer_display.hpp"
#include "canvas/canvas.hpp"
#include "json.hpp"
#include "util.hpp"

namespace horizon {

	class ColorBox: public Gtk::DrawingArea {
		public:
			ColorBox(unsigned int size=16);
			void set_color(float r, float g, float b);
			void set_color(const Color &c);

		protected:
			bool on_draw(const Cairo::RefPtr<Cairo::Context >& cr) override;

		private:
			Color color;

	};

	ColorBox::ColorBox(unsigned int size): Gtk::DrawingArea() {
		set_size_request(size, size);
	}

	bool ColorBox::on_draw(const Cairo::RefPtr<Cairo::Context >& cr) {
		cr->set_source_rgb(color.r,color.g,color.b);
		cr->paint();

		Gtk::DrawingArea::on_draw(cr);
		return true;
	}

	void ColorBox::set_color(const Color &c) {
		color = c;
		queue_draw();
	}

	void ColorBox::set_color(float r, float g, float b) {
		color.r = r;
		color.g = g;
		color.b = b;
	}

	class CheckStatusDisplay: public Gtk::Box {
		public:
			CheckStatusDisplay();
			void set_level(CheckErrorLevel lev);

		private:
			ColorBox *color_box = nullptr;
			Gtk::Label *la = nullptr;
	};

	CheckStatusDisplay::CheckStatusDisplay(): Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 2) {
		color_box = Gtk::manage(new ColorBox());
		la = Gtk::manage(new Gtk::Label("fixme"));
		la->set_xalign(0);
		color_box->show();
		la->show();
		pack_start(*color_box, false, false, 0);
		pack_start(*la, true, true, 0);
		set_level(CheckErrorLevel::NOT_RUN);
	}

	void CheckStatusDisplay::set_level(CheckErrorLevel lev) {
		la->set_text(check_error_level_to_string(lev));
		color_box->set_color(check_error_level_to_color(lev));
	}


	class CheckBox: public Gtk::Box{
		public:
			CheckBox(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x):	Gtk::Box(cobject) {
			}
			static CheckBox* create(CheckBase *ch) {
				CheckBox* w;

				Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
				std::vector<Glib::ustring> objs = {"check_box", "image1"};
				x->add_from_resource("/net/carrotIndustries/horizon/imp/checks/checks_window.ui", objs);
				x->get_widget_derived("check_box", w);
				x->get_widget("check_name", w->w_name);
				x->get_widget("check_description", w->w_description);
				x->get_widget("check_status_box", w->w_status_box);
				x->get_widget("check_enable", w->w_enable);
				x->get_widget("check_settings_button", w->w_settings_button);
				w->check = ch;


				w->w_name->set_text(w->check->name);
				w->w_description->set_text(w->check->description);

				w->w_status = Gtk::manage(new CheckStatusDisplay());
				w->w_status_box->pack_start(*w->w_status, true, true, 0);
				w->w_enable->set_active(true);

				w->w_settings_button->signal_clicked().connect([w] {
					auto top = dynamic_cast<Gtk::Window*>(w->get_ancestor(GTK_TYPE_WINDOW));
					auto dia = CheckSettingsDialog(top, w->check);
					dia.run();
				});

				w->reference();
				return w;
			}

			void update_result() {
				w_status->set_level(check->result.level);
			}

			bool is_enabled() {
				return w_enable->get_active();
			}

			void set_enabled(bool e) {
				w_enable->set_active(e);
			}

			CheckBase *check;
		private :
			Gtk::Label *w_name;
			Gtk::Label *w_description;
			Gtk::Box *w_status_box;
			Gtk::CheckButton *w_enable;
			Gtk::Button *w_settings_button;
			CheckStatusDisplay *w_status;


	};

	ChecksWindow::ChecksWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x) :
		Gtk::Window(cobject) {
	}

	ChecksWindow* ChecksWindow::create(Gtk::Window *p, CheckRunner *runner, CanvasGL *ca) {
		ChecksWindow* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/imp/checks/checks_window.ui");
		x->get_widget_derived("window", w);
		x->get_widget("checks_listbox", w->checks_listbox);
		x->get_widget("tv_result", w->checks_treeview);
		x->get_widget("run_button", w->run_button);
		x->get_widget("load_button", w->load_button);
		x->get_widget("save_button", w->save_button);
		w->set_transient_for(*p);
		w->check_runner = runner;
		w->canvas = ca;
		w->signal_show().connect([w]{w->canvas->markers.set_domain_visible(MarkerDomain::CHECK, true);});
		w->signal_hide().connect([w]{w->canvas->markers.set_domain_visible(MarkerDomain::CHECK, false);});

		for(const auto &it: runner->get_checks()) {
			auto box = CheckBox::create(it.second.get());
			w->checks_listbox->append(*box);
			box->unreference();
			box->show_all();
		}

		w->result_store = Gtk::TreeStore::create(w->tree_columns);
		w->checks_treeview->set_model(w->result_store);

		w->checks_treeview->append_column("Check", w->tree_columns.name);
		{
			auto cr = Gtk::manage(new CellRendererLayerDisplay());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Result", *cr));
			auto cr2 = Gtk::manage(new Gtk::CellRendererText());
			cr2->property_text()="hallo";

			tvc->set_cell_data_func(*cr2, [w](Gtk::CellRenderer* tcr, const Gtk::TreeModel::iterator &it){
				Gtk::TreeModel::Row row = *it;
				auto mcr = dynamic_cast<Gtk::CellRendererText*>(tcr);
				mcr->property_text() = check_error_level_to_string(row[w->tree_columns.result]);
			});
			tvc->set_cell_data_func(*cr, [w](Gtk::CellRenderer* tcr, const Gtk::TreeModel::iterator &it){
				Gtk::TreeModel::Row row = *it;
				auto mcr = dynamic_cast<CellRendererLayerDisplay*>(tcr);
				auto co = check_error_level_to_color(row[w->tree_columns.result]);
				Gdk::RGBA va;
				va.set_red(co.r);
				va.set_green(co.g);
				va.set_blue(co.b);
				mcr->property_color() = va;

			});
			tvc->pack_start(*cr2, false);
			w->checks_treeview->append_column(*tvc);
		}
		w->checks_treeview->append_column("Comment", w->tree_columns.comment);

		w->checks_treeview->signal_row_activated().connect([w] (const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
			auto it = w->result_store->get_iter(path);
			if(it) {
				Gtk::TreeModel::Row row = *it;
				if(row[w->tree_columns.has_location]) {
					w->s_signal_goto.emit(row[w->tree_columns.location], row[w->tree_columns.sheet]);
				}

			}
		});


		w->run_button->signal_clicked().connect([w] {
			auto ids = w->get_selected_checks();
			w->check_runner->run_some(ids);
			w->update_result();
		});

		w->save_button->signal_clicked().connect([w] {
			auto ids = w->get_selected_checks();
			auto j = w->check_runner->serialize(ids);
			Gtk::FileChooserDialog fc(*w, "Save Check Settings", Gtk::FILE_CHOOSER_ACTION_SAVE);
			fc.set_do_overwrite_confirmation(true);
			if(w->last_filename.size()) {
				fc.set_filename(w->last_filename);
			}
			else {
				fc.set_current_name("checks.json");
			}
			fc.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
			fc.add_button("_Save", Gtk::RESPONSE_ACCEPT);
			if(fc.run()==Gtk::RESPONSE_ACCEPT) {
				std::string fn = fc.get_filename();
				w->last_filename = fn;
				save_json_to_file(fn, j);
			}
		});
		w->load_button->signal_clicked().connect([w] {
			Gtk::FileChooserDialog fc(*w, "Load Check Settings", Gtk::FILE_CHOOSER_ACTION_OPEN);
			if(w->last_filename.size()) {
				fc.set_filename(w->last_filename);
			}
			fc.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
			fc.add_button("_Open", Gtk::RESPONSE_ACCEPT);
			if(fc.run()==Gtk::RESPONSE_ACCEPT) {
				std::string fn = fc.get_filename();
				w->last_filename = fn;
				json j;
				std::ifstream ifs(fn);
				if(!ifs.is_open()) {
					throw std::runtime_error("file "  +fn+ " not opened");
				}
				ifs>>j;
				ifs.close();

				auto ids = w->check_runner->load_from_json(j);
				for(auto it: w->checks_listbox->get_children()) {
					if(auto chbox = dynamic_cast<CheckBox*>(dynamic_cast<Gtk::ListBoxRow*>(it)->get_child())) {
						chbox->set_enabled(ids.count(chbox->check->id));
					}
				}

			}
		});
		return w;
	}

	std::set<CheckID> ChecksWindow::get_selected_checks() {
		std::set<CheckID> ids;
		for(auto it: checks_listbox->get_children()) {
			if(auto chbox = dynamic_cast<CheckBox*>(dynamic_cast<Gtk::ListBoxRow*>(it)->get_child())) {
				if(chbox->check->id != CheckID::NONE && chbox->is_enabled()) {
					ids.insert(chbox->check->id);
				}
			}
		}
		return ids;
	}

	void ChecksWindow::update_result() {
		result_store->freeze_notify();
		result_store->clear();
		auto &dom = canvas->markers.get_domain(MarkerDomain::CHECK);
		dom.clear();
		for(auto it: checks_listbox->get_children()) {
			if(auto chbox = dynamic_cast<CheckBox*>(dynamic_cast<Gtk::ListBoxRow*>(it)->get_child())) {
				chbox->update_result();
				if(chbox->check->result.level != CheckErrorLevel::NOT_RUN) {
					Gtk::TreeModel::iterator iter = result_store->append();
					Gtk::TreeModel::Row row = *iter;
					row[tree_columns.name] = chbox->check->name;
					row[tree_columns.result] = chbox->check->result.level;
					row[tree_columns.has_location] = false;
					for(const auto &it_err: chbox->check->result.errors) {
						iter = result_store->append(row.children());
						Gtk::TreeModel::Row row_err = *iter;
						row_err[tree_columns.result] = it_err.level;
						row_err[tree_columns.comment] = it_err.comment;
						row_err[tree_columns.has_location] = it_err.has_location;
						row_err[tree_columns.location] = it_err.location;
						row_err[tree_columns.sheet] = it_err.sheet;

						if(it_err.has_location) {
							dom.emplace_back(it_err.location, check_error_level_to_color(it_err.level), it_err.sheet);
						}
					}
				}
			}
		}
		result_store->thaw_notify();



		canvas->update_markers();
	}
}
