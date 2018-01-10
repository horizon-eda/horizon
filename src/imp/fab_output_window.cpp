#include "fab_output_window.hpp"
#include "board.hpp"
#include "gtk_util.hpp"
#include "widgets/spin_button_dim.hpp"
#include "export_gerber/gerber_export.hpp"

namespace horizon {


	class GerberLayerEditor: public Gtk::Box {
		public:
			GerberLayerEditor(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, FabOutputWindow *pa, FabOutputSettings::GerberLayer *la);
			static GerberLayerEditor* create(FabOutputWindow *pa, FabOutputSettings::GerberLayer *la);
			FabOutputWindow *parent;

		private :
			Gtk::CheckButton *gerber_layer_checkbutton = nullptr;
			Gtk::Entry *gerber_layer_filename_entry = nullptr;

			FabOutputSettings::GerberLayer *layer = nullptr;

	};

	GerberLayerEditor::GerberLayerEditor(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, FabOutputWindow *pa, FabOutputSettings::GerberLayer *la) :
		Gtk::Box(cobject), parent(pa), layer(la) {
		x->get_widget("gerber_layer_checkbutton", gerber_layer_checkbutton);
		x->get_widget("gerber_layer_filename_entry", gerber_layer_filename_entry);
		parent->sg_layer_name->add_widget(*gerber_layer_checkbutton);

		gerber_layer_checkbutton->set_label(parent->brd->get_layers().at(layer->layer).name);
		bind_widget(gerber_layer_checkbutton, layer->enabled);
		bind_widget(gerber_layer_filename_entry, layer->filename);



	}

	GerberLayerEditor* GerberLayerEditor::create(FabOutputWindow *pa, FabOutputSettings::GerberLayer *la) {
		GerberLayerEditor* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/imp/fab_output.ui", "gerber_layer_editor");
		x->get_widget_derived("gerber_layer_editor", w, pa, la);
		w->reference();
		return w;
	}

	FabOutputWindow::FabOutputWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, Board *bo, FabOutputSettings *s) :
		Gtk::Window(cobject), brd(bo), settings(s) {
		x->get_widget("gerber_layers_box", gerber_layers_box);
		x->get_widget("prefix_entry", prefix_entry);
		x->get_widget("directory_entry", directory_entry);
		x->get_widget("npth_filename_entry", npth_filename_entry);
		x->get_widget("pth_filename_entry", pth_filename_entry);
		x->get_widget("npth_filename_label", npth_filename_label);
		x->get_widget("pth_filename_label", pth_filename_label);
		x->get_widget("generate_button", generate_button);
		x->get_widget("directory_button", directory_button);
		x->get_widget("drill_mode_combo", drill_mode_combo);
		x->get_widget("log_textview", log_textview);

		bind_widget(prefix_entry, settings->prefix);
		bind_widget(directory_entry, settings->output_directory);
		bind_widget(npth_filename_entry, settings->drill_npth_filename);
		bind_widget(pth_filename_entry, settings->drill_pth_filename);

		drill_mode_combo->set_active_id(FabOutputSettings::mode_lut.lookup_reverse(settings->drill_mode));
		drill_mode_combo->signal_changed().connect([this] {
			settings->drill_mode = FabOutputSettings::mode_lut.lookup(drill_mode_combo->get_active_id());
			update_drill_visibility();
		});
		update_drill_visibility();

		generate_button->signal_clicked().connect(sigc::mem_fun(this, &FabOutputWindow::generate));
		directory_button->signal_clicked().connect([this]{
			GtkFileChooserNative *native = gtk_file_chooser_native_new ("Select output directory",
				GTK_WINDOW(gobj()),
				GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				"Select",
				"_Cancel");
			auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
			chooser->set_do_overwrite_confirmation(true);
			chooser->set_filename(directory_entry->get_text());

			if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
				directory_entry->set_text(chooser->get_filename());
			}
		});

		outline_width_sp = Gtk::manage(new SpinButtonDim());
		outline_width_sp->set_range(.01_mm, 10_mm);
		outline_width_sp->show();
		bind_widget(outline_width_sp, settings->outline_width);
		{
			Gtk::Box *b = nullptr;
			x->get_widget("gerber_outline_width_box", b);
			b->pack_start(*outline_width_sp, true, true, 0);
		}

		sg_layer_name =  Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

		std::vector<FabOutputSettings::GerberLayer*> layers_sorted;
		layers_sorted.reserve(settings->layers.size());
		for(auto &la: settings->layers) {
			layers_sorted.push_back(&la.second);
		}
		std::sort(layers_sorted.begin(), layers_sorted.end(), [](const auto a, const auto b){return b->layer < a->layer;});


		for(auto la: layers_sorted) {
			auto ed = GerberLayerEditor::create(this, la);
			gerber_layers_box->add(*ed);
			ed->show();
			ed->unreference();
		}

	}

	void FabOutputWindow::update_drill_visibility() {
		if(settings->drill_mode == FabOutputSettings::DrillMode::INDIVIDUAL) {
			npth_filename_entry->set_visible(true);
			npth_filename_label->set_visible(true);
			pth_filename_label->set_text("PTH suffix");
		}
		else {
			npth_filename_entry->set_visible(false);
			npth_filename_label->set_visible(false);
			pth_filename_label->set_text("Drill suffix");
		}
	}

	void FabOutputWindow::generate() {
			try {
				GerberExporter ex(brd, settings);
				ex.generate();
				log_textview->get_buffer()->set_text(ex.get_log());
			}
			catch (const std::exception& e) {
				log_textview->get_buffer()->set_text(std::string("Error: ")+e.what());
			}
			catch (const Gio::Error& e) {
				log_textview->get_buffer()->set_text(std::string("Error: ")+e.what());
			}
			catch (...) {
				log_textview->get_buffer()->set_text("Other error");
			}
	}

	void FabOutputWindow::set_can_generate(bool v) {
		generate_button->set_sensitive(v);
	}

	FabOutputWindow* FabOutputWindow::create(Gtk::Window *p, Board *b, FabOutputSettings *s) {
		FabOutputWindow* w;
		Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
		x->add_from_resource("/net/carrotIndustries/horizon/imp/fab_output.ui");
		x->get_widget_derived("window", w, b, s);

		w->set_transient_for(*p);

		return w;
	}
}
