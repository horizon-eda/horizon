#include "footprint_generator_dual.hpp"
#include "widgets/chooser_buttons.hpp"
#include "util/util.hpp"

namespace horizon {
	FootprintGeneratorDual::FootprintGeneratorDual(CorePackage *c): Glib::ObjectBase (typeid(FootprintGeneratorDual)), FootprintGeneratorBase("/net/carrotIndustries/horizon/imp/footprint_generator/dual.svg", c) {


			{
				auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
				auto la = Gtk::manage(new Gtk::Label("Count:"));
				tbox->pack_start(*la, false, false, 0);

				sp_count = Gtk::manage(new Gtk::SpinButton());
				sp_count->set_range(2, 512);
				sp_count->set_increments(2, 2);
				sp_count->signal_input().connect([this] (double *v) {
					auto txt = sp_count->get_text();
					int64_t va = 0;
					try {
						va = MAX(round_multiple(std::stoi(txt), 2), 1);
						*v = va;
					}
					catch (const std::exception& e) {
						return false;
					}
					return true;
				});
				tbox->pack_start(*sp_count, false, false, 0);

				box_top->pack_start(*tbox, false, false, 0);
			}

			update_preview();
			sp_spacing = Gtk::manage(new SpinButtonDim());
			sp_spacing->set_range(0, 100_mm);
			sp_spacing->set_valign(Gtk::ALIGN_CENTER);
			sp_spacing->set_halign(Gtk::ALIGN_START);
			sp_spacing->set_value(3_mm);
			overlay->add_at_sub(*sp_spacing, "#spacing");
			sp_spacing->show();

			sp_pitch = Gtk::manage(new SpinButtonDim());
			sp_pitch->set_range(0, 50_mm);
			sp_pitch->set_valign(Gtk::ALIGN_CENTER);
			sp_pitch->set_halign(Gtk::ALIGN_START);
			sp_pitch->set_value(1_mm);
			overlay->add_at_sub(*sp_pitch, "#pitch");
			sp_pitch->show();

			sp_pad_height = Gtk::manage(new SpinButtonDim());
			sp_pad_height->set_range(0, 50_mm);
			sp_pad_height->set_valign(Gtk::ALIGN_CENTER);
			sp_pad_height->set_halign(Gtk::ALIGN_START);
			sp_pad_height->set_value(.5_mm);
			overlay->add_at_sub(*sp_pad_height, "#pad_height");
			sp_pad_height->show();

			sp_pad_width = Gtk::manage(new SpinButtonDim());
			sp_pad_width->set_range(0, 50_mm);
			sp_pad_width->set_valign(Gtk::ALIGN_CENTER);
			sp_pad_width->set_halign(Gtk::ALIGN_START);
			sp_pad_width->set_value(.5_mm);
			overlay->add_at_sub(*sp_pad_width, "#pad_width");
			sp_pad_width->show();

			sp_count->signal_value_changed().connect([this]{pad_count = sp_count->get_value_as_int(); update_preview();});

			{
				auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
				auto la = Gtk::manage(new Gtk::Label("Mode:"));
				tbox->pack_start(*la, false, false, 0);

				auto rbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
				rbox->get_style_context()->add_class("linked");
				auto rb1 = Gtk::manage(new Gtk::RadioButton("Circular"));
				rb1->set_mode(false);
				auto rb2 = Gtk::manage(new Gtk::RadioButton("Zigzag"));
				rb2->signal_toggled().connect([this, rb2]{zigzag = rb2->get_active(); update_preview();});
				//sp_count->signal_changed().connect([this]{set_pad_count(sp_count->get_value_as_int());});
				rb2->set_mode(false);
				rb2->join_group(*rb1);
				rbox->pack_start(*rb1, false, false, 0);
				rbox->pack_start(*rb2, false, false, 0);
				rb1->set_active(true);


				tbox->pack_start(*rbox, false, false, 0);


				box_top->pack_start(*tbox, false, false, 0);
			}
			sp_count->set_value(4);

		}

		bool FootprintGeneratorDual::generate() {
			if(!property_can_generate())
				return false;
			auto pkg = core->get_package();
			int64_t spacing = sp_spacing->get_value_as_int();
			int64_t pitch = sp_pitch->get_value_as_int();
			int64_t y0 = (pad_count/2-1)*(pitch/2);
			int64_t pad_width = sp_pad_width->get_value_as_int();
			int64_t pad_height = sp_pad_height->get_value_as_int();
			auto padstack = core->m_pool->get_padstack(browser_button->property_selected_uuid());
			for(auto it: {-1, 1}) {
				for(unsigned int i = 0; i<pad_count/2; i++) {
					auto uu = UUID::random();

					auto &pad = pkg->pads.emplace(uu, Pad(uu, padstack)).first->second;
					pad.placement.shift = {it*spacing, y0-pitch*i};
					if(padstack->parameter_set.count(ParameterID::PAD_DIAMETER)) {
						pad.parameter_set[ParameterID::PAD_DIAMETER] = std::min(pad_width, pad_height);
					}
					else {
						pad.parameter_set[ParameterID::PAD_HEIGHT] = pad_height;
						pad.parameter_set[ParameterID::PAD_WIDTH] = pad_width;
					}
					if(it < 0)
						pad.placement.set_angle_deg(270);
					else
						pad.placement.set_angle_deg(90);
					if(!zigzag) {
						if(it < 0) {
							pad.name = std::to_string(i+1);
						}
						else {
							pad.name = std::to_string(pad_count-i);
						}
					}
					else {
						if(it < 0) {
							pad.name = std::to_string(i*2+1);
						}
						else {
							pad.name = std::to_string(i*2+2);
						}
					}
				}
			}

			core->commit();
			return true;
		}

		void FootprintGeneratorDual::update_preview() {
			auto n = pad_count;
			n &= ~1;
			if(n<2)
				return;
			if(!zigzag) {
				if(n>=8) {
					overlay->sub_texts["#pad1"] = "1";
					overlay->sub_texts["#pad2"] = "2";
					overlay->sub_texts["#pad3"] = std::to_string(n/2-1);
					overlay->sub_texts["#pad4"] = std::to_string(n/2);
					overlay->sub_texts["#pad5"] = std::to_string(n/2+1);
					overlay->sub_texts["#pad6"] = std::to_string(n/2+2);
					overlay->sub_texts["#pad7"] = std::to_string(n-1);
					overlay->sub_texts["#pad8"] = std::to_string(n);
				}
				else if(n==6) {
					overlay->sub_texts["#pad1"] = "1";
					overlay->sub_texts["#pad2"] = "2";
					overlay->sub_texts["#pad3"] = "3";
					overlay->sub_texts["#pad4"] = "X";
					overlay->sub_texts["#pad5"] = "X";
					overlay->sub_texts["#pad6"] = "4";
					overlay->sub_texts["#pad7"] = "5";
					overlay->sub_texts["#pad8"] = "6";
				}
				else if(n==4) {
					overlay->sub_texts["#pad1"] = "1";
					overlay->sub_texts["#pad2"] = "2";
					overlay->sub_texts["#pad3"] = "X";
					overlay->sub_texts["#pad4"] = "X";
					overlay->sub_texts["#pad5"] = "X";
					overlay->sub_texts["#pad6"] = "X";
					overlay->sub_texts["#pad7"] = "3";
					overlay->sub_texts["#pad8"] = "4";
				}
				else if(n==2) {
					overlay->sub_texts["#pad1"] = "1";
					overlay->sub_texts["#pad2"] = "X";
					overlay->sub_texts["#pad3"] = "X";
					overlay->sub_texts["#pad4"] = "X";
					overlay->sub_texts["#pad5"] = "X";
					overlay->sub_texts["#pad6"] = "X";
					overlay->sub_texts["#pad7"] = "X";
					overlay->sub_texts["#pad8"] = "2";
				}
			}
			else {
				if(n>=8) {
					overlay->sub_texts["#pad1"] = "1";
					overlay->sub_texts["#pad8"] = "2";
					overlay->sub_texts["#pad2"] = "3";
					overlay->sub_texts["#pad7"] = "4";
					overlay->sub_texts["#pad5"] = std::to_string(n);
					overlay->sub_texts["#pad4"] = std::to_string(n-1);
					overlay->sub_texts["#pad6"] = std::to_string(n-2);
					overlay->sub_texts["#pad3"] = std::to_string(n-3);
				}
				else if(n==6) {
					overlay->sub_texts["#pad1"] = "1";
					overlay->sub_texts["#pad8"] = "2";
					overlay->sub_texts["#pad2"] = "3";
					overlay->sub_texts["#pad7"] = "4";
					overlay->sub_texts["#pad5"] = "X";
					overlay->sub_texts["#pad4"] = "X";
					overlay->sub_texts["#pad6"] = "6";
					overlay->sub_texts["#pad3"] = "5";
				}
				else if(n==4) {
					overlay->sub_texts["#pad1"] = "1";
					overlay->sub_texts["#pad8"] = "2";
					overlay->sub_texts["#pad2"] = "3";
					overlay->sub_texts["#pad7"] = "4";
					overlay->sub_texts["#pad5"] = "X";
					overlay->sub_texts["#pad4"] = "X";
					overlay->sub_texts["#pad6"] = "X";
					overlay->sub_texts["#pad3"] = "X";
				}
				else if(n==2) {
					overlay->sub_texts["#pad1"] = "1";
					overlay->sub_texts["#pad8"] = "2";
					overlay->sub_texts["#pad2"] = "X";
					overlay->sub_texts["#pad7"] = "X";
					overlay->sub_texts["#pad5"] = "X";
					overlay->sub_texts["#pad4"] = "X";
					overlay->sub_texts["#pad6"] = "X";
					overlay->sub_texts["#pad3"] = "X";
				}
			}
			overlay->queue_draw();
		}
}
