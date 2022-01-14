#include "rule_editor_connectivity.hpp"
#include "schematic/rule_connectivity.hpp"

namespace horizon {
void RuleEditorConnectivity::populate()
{
    rule2 = &dynamic_cast<RuleConnectivity &>(rule);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    box->set_margin_start(20);
    box->set_margin_end(20);
    box->set_margin_bottom(20);
    {
        auto la = Gtk::manage(new Gtk::Label("• One-pin nets"));
        la->set_halign(Gtk::ALIGN_START);
        box->pack_start(*la, true, true, 0);
    }
    {
        Gtk::CheckButton *editor = Gtk::manage(new Gtk::CheckButton("Include unnamed nets"));
        editor->set_halign(Gtk::ALIGN_START);
        editor->set_margin_start(10);
        box->pack_start(*editor, true, true, 0);
        editor->set_active(rule2->include_unnamed);
        editor->signal_toggled().connect([this, editor] {
            rule2->include_unnamed = editor->get_active();
            s_signal_updated.emit();
        });
    }
    {
        auto la = Gtk::manage(new Gtk::Label("• One-label nets"));
        la->set_margin_top(5);
        la->set_halign(Gtk::ALIGN_START);
        box->pack_start(*la, true, true, 0);
    }
    {
        auto la = Gtk::manage(new Gtk::Label("• Unconnected pins"));
        la->set_margin_top(5);
        la->set_halign(Gtk::ALIGN_START);
        box->pack_start(*la, true, true, 0);
    }
    {
        auto la = Gtk::manage(new Gtk::Label("• Unconnected ports"));
        la->set_margin_top(5);
        la->set_halign(Gtk::ALIGN_START);
        box->pack_start(*la, true, true, 0);
    }

    box->show_all();
    pack_start(*box, false, false, 0);
}
} // namespace horizon
