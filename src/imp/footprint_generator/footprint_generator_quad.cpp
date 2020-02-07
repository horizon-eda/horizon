#include "footprint_generator_quad.hpp"
#include "widgets/pool_browser_button.hpp"
#include "document/idocument_package.hpp"
#include "pool/pool.hpp"

namespace horizon {
FootprintGeneratorQuad::FootprintGeneratorQuad(IDocumentPackage *c)
    : Glib::ObjectBase(typeid(FootprintGeneratorQuad)),
      FootprintGeneratorBase("/org/horizon-eda/horizon/imp/footprint_generator/quad.svg", c)
{
    update_preview();
    sp_spacing_h = Gtk::manage(new SpinButtonDim());
    sp_spacing_h->set_range(0, 100_mm);
    sp_spacing_h->set_valign(Gtk::ALIGN_CENTER);
    sp_spacing_h->set_halign(Gtk::ALIGN_START);
    sp_spacing_h->set_value(40_mm);
    overlay->add_at_sub(*sp_spacing_h, "#spacing_h");
    sp_spacing_h->show();

    sp_spacing_v = Gtk::manage(new SpinButtonDim());
    sp_spacing_v->set_range(0, 100_mm);
    sp_spacing_v->set_valign(Gtk::ALIGN_CENTER);
    sp_spacing_v->set_halign(Gtk::ALIGN_START);
    sp_spacing_v->set_value(30_mm);
    overlay->add_at_sub(*sp_spacing_v, "#spacing_v");
    sp_spacing_v->show();

    sp_pitch = Gtk::manage(new SpinButtonDim());
    sp_pitch->set_range(0, 50_mm);
    sp_pitch->set_valign(Gtk::ALIGN_CENTER);
    sp_pitch->set_halign(Gtk::ALIGN_START);
    sp_pitch->set_value(3_mm);
    overlay->add_at_sub(*sp_pitch, "#pitch");
    sp_pitch->show();

    sp_pad_height = Gtk::manage(new SpinButtonDim());
    sp_pad_height->set_range(0, 50_mm);
    sp_pad_height->set_valign(Gtk::ALIGN_CENTER);
    sp_pad_height->set_halign(Gtk::ALIGN_END);
    sp_pad_height->set_value(.5_mm);
    overlay->add_at_sub(*sp_pad_height, "#pad_height");
    sp_pad_height->show();

    sp_pad_width = Gtk::manage(new SpinButtonDim());
    sp_pad_width->set_range(0, 50_mm);
    sp_pad_width->set_valign(Gtk::ALIGN_CENTER);
    sp_pad_width->set_halign(Gtk::ALIGN_END);
    sp_pad_width->set_value(.5_mm);
    overlay->add_at_sub(*sp_pad_width, "#pad_width");
    sp_pad_width->show();

    {
        auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
        auto la = Gtk::manage(new Gtk::Label("Count H:"));
        tbox->pack_start(*la, false, false, 0);

        sp_count_h = Gtk::manage(new Gtk::SpinButton());
        sp_count_h->set_range(1, 512);
        sp_count_h->set_increments(1, 1);
        tbox->pack_start(*sp_count_h, false, false, 0);

        box_top->pack_start(*tbox, false, false, 0);
    }
    {
        auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
        auto la = Gtk::manage(new Gtk::Label("Count V:"));
        tbox->pack_start(*la, false, false, 0);

        sp_count_v = Gtk::manage(new Gtk::SpinButton());
        sp_count_v->set_range(1, 512);
        sp_count_v->set_increments(1, 1);
        tbox->pack_start(*sp_count_v, false, false, 0);

        box_top->pack_start(*tbox, false, false, 0);
    }

    sp_count_h->signal_value_changed().connect([this] {
        pad_count_h = sp_count_h->get_value_as_int();
        update_preview();
    });
    sp_count_v->signal_value_changed().connect([this] {
        pad_count_v = sp_count_v->get_value_as_int();
        update_preview();
    });

    sp_count_h->set_value(4);
    sp_count_v->set_value(4);
}

bool FootprintGeneratorQuad::generate()
{
    if (!property_can_generate())
        return false;
    auto pkg = core->get_package();
    int64_t pitch = sp_pitch->get_value_as_int();
    int64_t spacing_h = sp_spacing_h->get_value_as_int();
    int64_t y0 = (pad_count_v - 1) * (pitch / 2);
    int64_t pad_width = sp_pad_width->get_value_as_int();
    int64_t pad_height = sp_pad_height->get_value_as_int();

    auto padstack = core->get_pool()->get_padstack(browser_button->property_selected_uuid());
    for (auto it : {-1, 1}) {
        for (unsigned int i = 0; i < pad_count_v; i++) {
            auto uu = UUID::random();
            auto &pad = pkg->pads.emplace(uu, Pad(uu, padstack)).first->second;
            pad.placement.shift = {it * spacing_h, y0 - pitch * i};
            if (padstack->parameter_set.count(ParameterID::PAD_DIAMETER)) {
                pad.parameter_set[ParameterID::PAD_DIAMETER] = std::min(pad_width, pad_height);
            }
            else {
                pad.parameter_set[ParameterID::PAD_HEIGHT] = pad_height;
                pad.parameter_set[ParameterID::PAD_WIDTH] = pad_width;
            }
            if (it < 0) {
                pad.placement.set_angle_deg(270);
                pad.name = std::to_string(i + 1);
            }
            else {
                pad.placement.set_angle_deg(90);
                pad.name = std::to_string(pad_count_v * 2 + pad_count_h - i);
            }
        }
    }

    int64_t spacing_v = sp_spacing_v->get_value_as_int();
    int64_t x0 = (pad_count_h - 1) * (pitch / 2) * -1;
    for (auto it : {-1, 1}) {
        for (unsigned int i = 0; i < pad_count_h; i++) {
            auto uu = UUID::random();
            auto &pad = pkg->pads.emplace(uu, Pad(uu, padstack)).first->second;
            pad.placement.shift = {x0 + pitch * i, it * spacing_v};
            if (padstack->parameter_set.count(ParameterID::PAD_DIAMETER)) {
                pad.parameter_set[ParameterID::PAD_DIAMETER] = std::min(pad_width, pad_height);
            }
            else {
                pad.parameter_set[ParameterID::PAD_HEIGHT] = pad_height;
                pad.parameter_set[ParameterID::PAD_WIDTH] = pad_width;
            }
            if (it < 0) {
                pad.placement.set_angle_deg(0);
                pad.name = std::to_string(i + 1 + pad_count_v);
            }
            else {
                pad.placement.set_angle_deg(180);
                pad.name = std::to_string(pad_count_v * 2 + pad_count_h * 2 - i);
            }
        }
    }
    return true;
}

void FootprintGeneratorQuad::update_preview()
{
    if (pad_count_v >= 4) {
        overlay->sub_texts["#pad1"] = "1";
        overlay->sub_texts["#pad2"] = "2";
        overlay->sub_texts["#pad3"] = std::to_string(pad_count_v - 1);
        overlay->sub_texts["#pad4"] = std::to_string(pad_count_v);

        overlay->sub_texts["#pad9"] = std::to_string(pad_count_h + pad_count_v + 1);
        overlay->sub_texts["#pad10"] = std::to_string(pad_count_h + pad_count_v + 2);
        overlay->sub_texts["#pad11"] = std::to_string(pad_count_h + pad_count_v * 2 - 1);
        overlay->sub_texts["#pad12"] = std::to_string(pad_count_h + pad_count_v * 2);
    }
    else if (pad_count_v == 3) {
        overlay->sub_texts["#pad1"] = "1";
        overlay->sub_texts["#pad2"] = "2";
        overlay->sub_texts["#pad3"] = "X";
        overlay->sub_texts["#pad4"] = "3";

        overlay->sub_texts["#pad9"] = std::to_string(pad_count_h + pad_count_v + 1);
        overlay->sub_texts["#pad10"] = std::to_string(pad_count_h + pad_count_v + 2);
        overlay->sub_texts["#pad11"] = "X";
        overlay->sub_texts["#pad12"] = std::to_string(pad_count_h + pad_count_v * 2);
    }
    else if (pad_count_v == 2) {
        overlay->sub_texts["#pad1"] = "1";
        overlay->sub_texts["#pad2"] = "X";
        overlay->sub_texts["#pad3"] = "X";
        overlay->sub_texts["#pad4"] = "2";

        overlay->sub_texts["#pad9"] = std::to_string(pad_count_h + pad_count_v + 1);
        overlay->sub_texts["#pad10"] = "X";
        overlay->sub_texts["#pad11"] = "X";
        overlay->sub_texts["#pad12"] = std::to_string(pad_count_h + pad_count_v * 2);
    }
    else if (pad_count_v == 1) {
        overlay->sub_texts["#pad1"] = "1";
        overlay->sub_texts["#pad2"] = "X";
        overlay->sub_texts["#pad3"] = "X";
        overlay->sub_texts["#pad4"] = "X";

        overlay->sub_texts["#pad9"] = "X";
        overlay->sub_texts["#pad10"] = "X";
        overlay->sub_texts["#pad11"] = "X";
        overlay->sub_texts["#pad12"] = std::to_string(pad_count_h + pad_count_v * 2);
    }

    if (pad_count_h >= 4) {
        overlay->sub_texts["#pad5"] = std::to_string(pad_count_v + 1);
        overlay->sub_texts["#pad6"] = std::to_string(pad_count_v + 2);
        overlay->sub_texts["#pad7"] = std::to_string(pad_count_v + pad_count_h - 1);
        overlay->sub_texts["#pad8"] = std::to_string(pad_count_v + pad_count_h);

        overlay->sub_texts["#pad13"] = std::to_string(pad_count_v * 2 + pad_count_h + 1);
        overlay->sub_texts["#pad14"] = std::to_string(pad_count_v * 2 + pad_count_h + 2);
        overlay->sub_texts["#pad15"] = std::to_string(pad_count_v * 2 + pad_count_h * 2 - 1);
        overlay->sub_texts["#pad16"] = std::to_string(pad_count_v * 2 + pad_count_h * 2);
    }
    else if (pad_count_h == 3) {
        overlay->sub_texts["#pad5"] = std::to_string(pad_count_v + 1);
        overlay->sub_texts["#pad6"] = std::to_string(pad_count_v + 2);
        overlay->sub_texts["#pad7"] = "X";
        overlay->sub_texts["#pad8"] = std::to_string(pad_count_v + pad_count_h);

        overlay->sub_texts["#pad13"] = std::to_string(pad_count_v * 2 + pad_count_h + 1);
        overlay->sub_texts["#pad14"] = std::to_string(pad_count_v * 2 + pad_count_h + 2);
        overlay->sub_texts["#pad15"] = "X";
        overlay->sub_texts["#pad16"] = std::to_string(pad_count_v * 2 + pad_count_h * 2);
    }
    else if (pad_count_h == 2) {
        overlay->sub_texts["#pad5"] = std::to_string(pad_count_v + 1);
        overlay->sub_texts["#pad6"] = "X";
        overlay->sub_texts["#pad7"] = "X";
        overlay->sub_texts["#pad8"] = std::to_string(pad_count_v + pad_count_h);

        overlay->sub_texts["#pad13"] = std::to_string(pad_count_v * 2 + pad_count_h + 1);
        overlay->sub_texts["#pad14"] = "X";
        overlay->sub_texts["#pad15"] = "X";
        overlay->sub_texts["#pad16"] = std::to_string(pad_count_v * 2 + pad_count_h * 2);
    }
    else if (pad_count_h == 1) {
        overlay->sub_texts["#pad5"] = std::to_string(pad_count_v + 1);
        overlay->sub_texts["#pad6"] = "X";
        overlay->sub_texts["#pad7"] = "X";
        overlay->sub_texts["#pad8"] = "X";

        overlay->sub_texts["#pad13"] = "X";
        overlay->sub_texts["#pad14"] = "X";
        overlay->sub_texts["#pad15"] = "X";
        overlay->sub_texts["#pad16"] = std::to_string(pad_count_v * 2 + pad_count_h * 2);
    }

    overlay->queue_draw();
}
} // namespace horizon
