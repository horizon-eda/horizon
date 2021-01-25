#include "check_column.hpp"
#include "widgets/cell_renderer_color_box.hpp"
#include "util/str_util.hpp"

namespace horizon {
void add_check_column(Gtk::TreeView &treeview, Gtk::TreeModelColumn<RulesCheckResult> &result_col)
{
    auto cr = Gtk::manage(new CellRendererColorBox());
    auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Checks"));
    tvc->pack_start(*cr, false);
    auto cr2 = Gtk::manage(new Gtk::CellRendererText());
    tvc->pack_start(*cr2, false);

    tvc->set_cell_data_func(*cr2, [&result_col](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
        Gtk::TreeModel::Row row = *it;
        auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
        const auto result = row.get_value(result_col);
        if (result.level == RulesCheckErrorLevel::NOT_RUN) {
            mcr->property_text() = "";
        }
        else {
            mcr->property_text() = rules_check_error_level_to_string(result.level);
        }
    });

    tvc->set_cell_data_func(*cr, [&result_col](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
        Gtk::TreeModel::Row row = *it;
        auto mcr = dynamic_cast<CellRendererColorBox *>(tcr);
        const auto result = row.get_value(result_col);
        auto co = rules_check_error_level_to_color(result.level);
        Gdk::RGBA va;
        if (result.level == RulesCheckErrorLevel::NOT_RUN)
            va.set_alpha(0);
        else
            va.set_alpha(1);
        va.set_red(co.r);
        va.set_green(co.g);
        va.set_blue(co.b);
        mcr->property_color() = va;
    });

    treeview.append_column(*tvc);


    auto popover = Gtk::manage(new Gtk::Popover);
    popover->set_relative_to(treeview);
    popover->set_position(Gtk::POS_BOTTOM);
    auto check_label = Gtk::manage(new Gtk::Label("foo"));
    check_label->property_margin() = 10;
    popover->add(*check_label);
    check_label->show();
    treeview.signal_button_press_event().connect_notify([&treeview, popover, tvc, check_label, cr,
                                                         &result_col](GdkEventButton *ev) {
        if (ev->button == 1) {
            Gdk::Rectangle rect;
            Gtk::TreeModel::Path path;
            Gtk::TreeViewColumn *col;
            int cell_x, cell_y;
            if (treeview.get_path_at_pos(ev->x, ev->y, path, col, cell_x, cell_y)) {
                if (col == tvc) {
                    treeview.get_cell_area(path, *col, rect);
                    int rx, ry;
                    rx = rect.get_x();
                    ry = rect.get_y();
                    treeview.convert_bin_window_to_widget_coords(rx, ry, rx, ry);
                    rect.set_x(rx);
                    rect.set_y(ry);
                    int cell_start, cell_width;
                    if (tvc->get_cell_position(*cr, cell_start, cell_width)) {
                        rect.set_x(rect.get_x() + cell_start);
                        rect.set_width(cell_width);
                    }

                    const auto &result = treeview.get_model()->get_iter(path)->get_value(result_col);
                    if (result.level == RulesCheckErrorLevel::WARN || result.level == RulesCheckErrorLevel::FAIL) {
                        std::string s;
                        for (const auto &it : result.errors) {
                            s += "• " + it.comment + "\n";
                        }
                        trim(s);
                        check_label->set_text(s);

                        popover->set_pointing_to(rect);
                        popover->popup();
                    }
                }
            }
        }
    });
}
} // namespace horizon
