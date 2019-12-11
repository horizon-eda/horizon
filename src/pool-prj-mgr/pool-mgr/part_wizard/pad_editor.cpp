#include "pad_editor.hpp"
#include "part_wizard.hpp"
#include "util/util.hpp"

namespace horizon {
void PadEditor::update_names()
{
    names.clear();
    for (auto &it : pads) {
        names.emplace_back(it->name);
    }

    std::sort(names.begin(), names.end(), [](const auto &a, const auto &b) { return strcmp_natural(a, b) < 0; });

    {
        std::stringstream s;
        std::copy(names.begin(), names.end(), std::ostream_iterator<std::string>(s, " "));
        pad_names_label->set_text(s.str());
    }
}

std::string PadEditor::get_gate_name()
{
    return combo_gate_entry->get_text();
}

PadEditor::PadEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, const Pad *p, PartWizard *pa)
    : Gtk::Box(cobject), parent(pa), pads({p})
{
    x->get_widget("pad_names", pad_names_label);
    x->get_widget("pin_name", pin_name_entry);
    x->get_widget("pin_names", pin_names_entry);
    x->get_widget("pin_direction", dir_combo);
    x->get_widget("combo_gate", combo_gate);

    auto propagate = [this](std::function<void(PadEditor *)> fn) {
        auto lb = dynamic_cast<Gtk::ListBox *>(get_ancestor(GTK_TYPE_LIST_BOX));
        auto this_row = dynamic_cast<Gtk::ListBoxRow *>(get_ancestor(GTK_TYPE_LIST_BOX_ROW));
        auto rows = lb->get_selected_rows();
        if (rows.size() > 1 && this_row->is_selected()) {
            for (auto &row : rows) {
                if (auto ed = dynamic_cast<PadEditor *>(row->get_child())) {
                    fn(ed);
                }
            }
        }
    };


    parent->sg_name->add_widget(*pad_names_label);

    for (const auto &it : Pin::direction_names) {
        dir_combo->append(std::to_string(static_cast<int>(it.first)), it.second);
    }
    dir_combo->set_active(0);

    dir_combo->signal_changed().connect([this, propagate] {
        propagate([this](PadEditor *ed) { ed->dir_combo->set_active_id(dir_combo->get_active_id()); });
    });


    combo_gate->set_model(parent->gate_name_store);
    combo_gate->set_entry_text_column(parent->list_columns.name);
    combo_gate->set_active(0);

    pin_name_entry->signal_changed().connect([this] { parent->update_pin_warnings(); });

    combo_gate_entry = dynamic_cast<Gtk::Entry *>(combo_gate->get_child());
    combo_gate_entry->signal_changed().connect([this, propagate] {
        std::cout << "ch " << combo_gate_entry->get_text() << std::endl;
        parent->update_gate_names();
        parent->update_pin_warnings();
        propagate([this](PadEditor *ed) { ed->combo_gate_entry->set_text(combo_gate_entry->get_text()); });
    });

    update_names();
    parent->update_pin_warnings();
}

PadEditor *PadEditor::create(const Pad *p, PartWizard *pa)
{
    PadEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource(
            "/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/part_wizard/"
            "part_wizard.ui");
    x->get_widget_derived("pad_editor", w, p, pa);
    w->reference();
    return w;
}
} // namespace horizon
