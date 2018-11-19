#include "select_group_tag.hpp"
#include "block/block.hpp"
#include "util/util.hpp"
#include <iostream>

namespace horizon {

void SelectGroupTagDialog::ok_clicked()
{
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        selection_valid = true;
        selected_uuid = row[list_columns.uuid];
    }
}

void SelectGroupTagDialog::row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        selection_valid = true;
        selected_uuid = row[list_columns.uuid];
        response(Gtk::ResponseType::RESPONSE_OK);
    }
}

SelectGroupTagDialog::SelectGroupTagDialog(Gtk::Window *parent, const Block *block, bool tag_mode)
    : Gtk::Dialog(tag_mode ? "Select tag" : "Select group", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(300, 300);

    button_ok->signal_clicked().connect(sigc::mem_fun(this, &SelectGroupTagDialog::ok_clicked));

    const auto &names = tag_mode ? block->tag_names : block->group_names;

    std::vector<std::pair<UUID, std::string>> names_sorted;
    for (const auto &it : names) {
        names_sorted.emplace_back(it.first, it.second);
    }
    std::sort(names_sorted.begin(), names_sorted.end(),
              [](const auto &a, const auto &b) { return strcmp_natural(a.second, b.second) < 0; });


    store = Gtk::ListStore::create(list_columns);
    for (const auto &it : names_sorted) {
        if (it.first) {
            Gtk::TreeModel::Row row = *(store->append());
            row[list_columns.uuid] = it.first;
            row[list_columns.name] = it.second;
            std::vector<const Component *> comps;
            for (const auto &it_comp : block->components) {
                if (tag_mode) {
                    if (it_comp.second.tag == it.first) {
                        comps.push_back(&it_comp.second);
                    }
                }
                else {
                    if (it_comp.second.group == it.first) {
                        comps.push_back(&it_comp.second);
                    }
                }
            }
            std::sort(comps.begin(), comps.end(),
                      [](auto a, auto b) { return strcmp_natural(a->refdes, b->refdes) < 0; });
            std::string comps_str;
            for (const auto &it_comp : comps) {
                comps_str += it_comp->refdes + ", ";
            }
            if (comps_str.size()) {
                comps_str.pop_back();
                comps_str.pop_back();
                row[list_columns.components] = comps_str;
            }
        }
    }

    view = Gtk::manage(new Gtk::TreeView(store));
    view->append_column(tag_mode ? "Tag" : "Group", list_columns.name);
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Components", *cr));
        tvc->add_attribute(cr->property_text(), list_columns.components);
        cr->property_ellipsize() = Pango::ELLIPSIZE_END;
        view->append_column(*tvc);
    }
    view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
    view->signal_row_activated().connect(sigc::mem_fun(this, &SelectGroupTagDialog::row_activated));


    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->add(*view);
    get_content_area()->pack_start(*sc, true, true, 0);
    get_content_area()->set_border_width(0);

    show_all();
}
} // namespace horizon
