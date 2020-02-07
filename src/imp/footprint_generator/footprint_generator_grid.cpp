#include "footprint_generator_grid.hpp"
#include "widgets/pool_browser_button.hpp"
#include "document/idocument_package.hpp"
#include "pool/pool.hpp"

namespace horizon {
FootprintGeneratorGrid::FootprintGeneratorGrid(IDocumentPackage *c)
    : Glib::ObjectBase(typeid(FootprintGeneratorGrid)),
      FootprintGeneratorBase("/org/horizon-eda/horizon/imp/footprint_generator/grid.svg", c)
{
    update_preview();

    sp_pitch_h = Gtk::manage(new SpinButtonDim());
    sp_pitch_h->set_range(0, 50_mm);
    sp_pitch_h->set_valign(Gtk::ALIGN_CENTER);
    sp_pitch_h->set_halign(Gtk::ALIGN_END);
    sp_pitch_h->set_value(3_mm);
    overlay->add_at_sub(*sp_pitch_h, "#pitch_h");
    sp_pitch_h->show();

    sp_pitch_v = Gtk::manage(new SpinButtonDim());
    sp_pitch_v->set_range(0, 50_mm);
    sp_pitch_v->set_valign(Gtk::ALIGN_CENTER);
    sp_pitch_v->set_halign(Gtk::ALIGN_START);
    sp_pitch_v->set_value(3_mm);
    sp_pitch_v->signal_value_changed().connect(sigc::mem_fun(*this, &FootprintGeneratorGrid::update_xy_lock));
    overlay->add_at_sub(*sp_pitch_v, "#pitch_v");
    sp_pitch_v->show();

    sp_pad_height = Gtk::manage(new SpinButtonDim());
    sp_pad_height->set_range(0, 50_mm);
    sp_pad_height->set_valign(Gtk::ALIGN_CENTER);
    sp_pad_height->set_halign(Gtk::ALIGN_START);
    sp_pad_height->set_value(.5_mm);
    sp_pad_height->signal_value_changed().connect(sigc::mem_fun(*this, &FootprintGeneratorGrid::update_xy_lock));
    overlay->add_at_sub(*sp_pad_height, "#pad_height");
    sp_pad_height->show();

    sp_pad_width = Gtk::manage(new SpinButtonDim());
    sp_pad_width->set_range(0, 50_mm);
    sp_pad_width->set_valign(Gtk::ALIGN_CENTER);
    sp_pad_width->set_halign(Gtk::ALIGN_START);
    sp_pad_width->set_value(.5_mm);
    overlay->add_at_sub(*sp_pad_width, "#pad_width");
    sp_pad_width->show();

    {
        auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
        auto la = Gtk::manage(new Gtk::Label("Count H:"));
        tbox->pack_start(*la, false, false, 0);

        sp_count_h = Gtk::manage(new Gtk::SpinButton());
        sp_count_h->set_range(4, 512);
        sp_count_h->set_increments(1, 1);
        tbox->pack_start(*sp_count_h, false, false, 0);

        box_top->pack_start(*tbox, false, false, 0);
    }
    {
        auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
        auto la = Gtk::manage(new Gtk::Label("Count V:"));
        tbox->pack_start(*la, false, false, 0);

        sp_count_v = Gtk::manage(new Gtk::SpinButton());
        sp_count_v->set_range(4, 512);
        sp_count_v->set_increments(1, 1);
        tbox->pack_start(*sp_count_v, false, false, 0);

        box_top->pack_start(*tbox, false, false, 0);
    }
    {
        cb_xy_lock = Gtk::manage(new Gtk::CheckButton("XY Lock"));
        cb_xy_lock->signal_toggled().connect(sigc::mem_fun(*this, &FootprintGeneratorGrid::update_xy_lock));
        cb_xy_lock->set_active(true);
        box_top->pack_start(*cb_xy_lock, false, false, 0);
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

void FootprintGeneratorGrid::update_xy_lock()
{
    auto locked = cb_xy_lock->get_active();
    sp_pad_width->set_sensitive(!locked);
    sp_pitch_h->set_sensitive(!locked);
    if (locked) {
        sp_pitch_h->set_value(sp_pitch_v->get_value());
        sp_pad_width->set_value(sp_pad_height->get_value());
    }
}

static const std::string bga_letters = "ABCDEFGHJKLMNPRTUVWY";

static std::string get_bga_letter(int x)
{
    x -= 1;
    int n_bga_letters = bga_letters.size();
    if (x < n_bga_letters) {
        char c[2] = {0};
        c[0] = bga_letters.at(x);
        return c;
    }
    else {
        char c[3] = {0};
        c[0] = bga_letters.at((x / n_bga_letters) - 1);
        c[1] = bga_letters.at(x % n_bga_letters);
        return c;
    }
}

bool FootprintGeneratorGrid::generate()
{
    if (!property_can_generate())
        return false;
    auto pkg = core->get_package();
    int64_t pitch_h = sp_pitch_h->get_value_as_int();
    int64_t pitch_v = sp_pitch_v->get_value_as_int();
    int64_t pad_width = sp_pad_width->get_value_as_int();
    int64_t pad_height = sp_pad_height->get_value_as_int();

    auto padstack = core->get_pool()->get_padstack(browser_button->property_selected_uuid());

    for (size_t x = 0; x < pad_count_h; x++) {
        for (size_t y = 0; y < pad_count_v; y++) {
            auto uu = UUID::random();
            auto &pad = pkg->pads.emplace(uu, Pad(uu, padstack)).first->second;
            pad.placement.shift.x = pitch_h * x - (pitch_h * (pad_count_h - 1)) / 2;
            pad.placement.shift.y = -pitch_v * y + (pitch_v * (pad_count_v - 1)) / 2;
            if (padstack->parameter_set.count(ParameterID::PAD_DIAMETER)) {
                pad.parameter_set[ParameterID::PAD_DIAMETER] = std::min(pad_width, pad_height);
            }
            else {
                pad.parameter_set[ParameterID::PAD_HEIGHT] = pad_height;
                pad.parameter_set[ParameterID::PAD_WIDTH] = pad_width;
            }
            pad.name = get_bga_letter(x + 1) + std::to_string(y + 1);
        }
    }
    return true;
}

void FootprintGeneratorGrid::update_preview()
{
    overlay->sub_texts["#padx1"] = "A";
    overlay->sub_texts["#padx2"] = "B";
    overlay->sub_texts["#padx3"] = get_bga_letter(pad_count_h - 1);
    overlay->sub_texts["#padx4"] = get_bga_letter(pad_count_h);

    overlay->sub_texts["#pady1"] = "1";
    overlay->sub_texts["#pady2"] = "2";
    overlay->sub_texts["#pady3"] = std::to_string(pad_count_v - 1);
    overlay->sub_texts["#pady4"] = std::to_string(pad_count_v);

    overlay->queue_draw();
}
} // namespace horizon
