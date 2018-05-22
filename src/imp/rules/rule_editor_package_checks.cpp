#include "rule_editor_package_checks.hpp"
#include "package/rule_package_checks.hpp"

namespace horizon {

static const char *text_pkg =
        "This rule checks if the package contains all required objects:\n"
        "• Refdes text on silkscreen\n"
        "• Refdes text on package\n"
        "• Silkscreen refdes width and size\n"
        "• Courtyards polygon\n"
        "• Pad naming convention\n"
        "• No polygons on silkscreen\n";

static const char *text_preflight =
        "This rule checks if the board is ready for fabrication output:\n"
        "• No airwires\n"
        "• No empty planes\n"
        "• No components without part\n"
        "• No unplaced components\n"
        "• No tracks without net\n";

void RuleEditorPackageChecks::populate()
{
    Gtk::Label *editor = Gtk::manage(new Gtk::Label);
    if (dynamic_cast<RulePackageChecks *>(rule)) {
        editor->set_text(text_pkg);
    }
    else {
        editor->set_text(text_preflight);
    }
    editor->set_xalign(0);
    editor->set_valign(Gtk::ALIGN_START);
    editor->set_margin_start(20);
    pack_start(*editor, true, true, 0);
    editor->show();
}
} // namespace horizon
