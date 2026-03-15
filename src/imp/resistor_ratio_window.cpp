#include "resistor_ratio_window.hpp"

namespace horizon {

class ResistorRatioWindow::FractionLabel : public Gtk::Box {
public:
    FractionLabel() : Gtk::Box(Gtk::ORIENTATION_VERTICAL)
    {
        la_num = Gtk::manage(new Gtk::Label());
        la_num->get_style_context()->add_class("frac-num");
        la_den = Gtk::manage(new Gtk::Label());
        la_num->show();
        la_den->show();

        pack_start(*la_num, true, true, 0);
        pack_start(*la_den, true, true, 0);
    }

    void set_text(const std::string &num, const std::string &den)
    {
        la_num->set_text(num);
        la_den->set_text(den);
    }

private:
    Gtk::Label *la_num = nullptr;
    Gtk::Label *la_den = nullptr;
};

Gtk::SpinButton &ResistorRatioWindow::make_sp()
{
    auto sp = Gtk::manage(new Gtk::SpinButton);
    sp->set_digits(3);
    sp->set_range(-1000, 1000);
    sp->set_increments(.1, 1);

    return *sp;
}

namespace {
class ChangingLock {
public:
    [[nodiscard]]
    ChangingLock(bool &ch)
        : changing(ch)
    {
        if (!changing) {
            did_lock = true;
            changing = true;
        }
    }

    operator bool() const
    {
        return did_lock;
    }

    ~ChangingLock()
    {
        changing = false;
    }

private:
    bool &changing;
    bool did_lock = false;
};
} // namespace

ResistorRatioWindow::ResistorRatioWindow(const ResistorInfo &r1a, const ResistorInfo &r2a)
    : Gtk::Window(), r1(r1a), r2(r2a), state_store(this, "resistor-ratio")
{
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto header = Gtk::manage(new Gtk::HeaderBar());
    header->set_show_close_button(true);
    header->set_title("Resistor ratio");
    set_titlebar(*header);
    header->show();

    {
        auto swap_button = Gtk::manage(new Gtk::Button("Swap"));
        header->pack_start(*swap_button);
        swap_button->show();
        swap_button->signal_clicked().connect([this] {
            std::swap(r1, r2);
            update();
            update_rhs();
        });
    }


    auto grid = Gtk::manage(new Gtk::Grid);
    grid->set_column_spacing(5);
    grid->set_row_spacing(5);
    int top = 0;

    r1_label = Gtk::manage(new Gtk::Label());
    r1_label->set_margin_bottom(5);
    r1_label->set_xalign(0);
    grid->attach(*r1_label, 2, top, 1, 1);
    r2_label = Gtk::manage(new Gtk::Label());
    r2_label->set_xalign(0);
    r2_label->set_margin_bottom(5);
    grid->attach(*r2_label, 4, top, 1, 1);

    top++;

    {
        vdiv_fraction_label = Gtk::manage(new FractionLabel);
        grid->attach(*vdiv_fraction_label, 0, top, 1, 1);

        {
            auto la = Gtk::manage(new Gtk::Label("×"));
            grid->attach(*la, 1, top, 1, 1);
        }

        vdiv_lhs_sp = &make_sp();
        grid->attach(*vdiv_lhs_sp, 2, top, 1, 1);

        {
            auto la = Gtk::manage(new Gtk::Label("="));
            grid->attach(*la, 3, top, 1, 1);
        }

        vdiv_rhs_sp = &make_sp();
        grid->attach(*vdiv_rhs_sp, 4, top, 1, 1);

        top++;

        ratio_fraction_label = Gtk::manage(new FractionLabel);
        grid->attach(*ratio_fraction_label, 0, top, 1, 1);

        {
            auto la = Gtk::manage(new Gtk::Label("×"));
            grid->attach(*la, 1, top, 1, 1);
        }

        ratio_lhs_sp = &make_sp();
        grid->attach(*ratio_lhs_sp, 2, top, 1, 1);

        {
            auto la = Gtk::manage(new Gtk::Label("="));
            grid->attach(*la, 3, top, 1, 1);
        }

        ratio_rhs_sp = &make_sp();
        grid->attach(*ratio_rhs_sp, 4, top, 1, 1);
    }

    vdiv_lhs_sp->signal_value_changed().connect([this] { update_rhs(); });

    vdiv_rhs_sp->signal_value_changed().connect([this] { update_lhs(); });

    ratio_lhs_sp->signal_value_changed().connect([this] { update_rhs(); });

    ratio_rhs_sp->signal_value_changed().connect([this] { update_lhs(); });

    vdiv_lhs_sp->set_value(1);
    ratio_lhs_sp->set_value(1);

    grid->show_all();
    add(*grid);
    grid->property_margin() = 10;

    update();
}

void ResistorRatioWindow::update_lhs()
{
    if (auto ch = ChangingLock(changing)) {
        vdiv_lhs_sp->set_value(vdiv_rhs_sp->get_value() / (r2.value / (r1.value + r2.value)));
        ratio_lhs_sp->set_value(ratio_rhs_sp->get_value() / (r1.value / r2.value));
    }
}

void ResistorRatioWindow::update_rhs()
{
    if (auto ch = ChangingLock(changing)) {
        vdiv_rhs_sp->set_value(vdiv_lhs_sp->get_value() * (r2.value / (r1.value + r2.value)));
        ratio_rhs_sp->set_value(ratio_lhs_sp->get_value() * (r1.value / r2.value));
    }
}

void ResistorRatioWindow::update()
{
    r1_label->set_text(r1.refdes + " = " + r1.value_formatted);
    r2_label->set_text(r2.refdes + " = " + r2.value_formatted);
    vdiv_fraction_label->set_text(r2.refdes, r1.refdes + " + " + r2.refdes);
    ratio_fraction_label->set_text(r1.refdes, r2.refdes);
}

} // namespace horizon
