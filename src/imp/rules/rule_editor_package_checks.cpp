#include "rule_editor_package_checks.hpp"

namespace horizon {

static const char *text_pkg =
        "This rule checks if the package contains all required objects:\n"
        "• Refdes text on silkscreen\n"
        "• Refdes text on package\n"
        "• Silkscreen refdes width and size\n"
        "• Courtyards polygon\n"
        "• Pad naming convention\n"
        "• No polygons on silkscreen\n"
        "• Parameter program not empty\n";

static const char *text_preflight =
        "This rule checks if the board is ready for fabrication output:\n"
        "• No airwires\n"
        "• No empty planes\n"
        "• No components without part\n"
        "• No unplaced components\n"
        "• No tracks without net\n"
        "• No polygons without usage on copper layers\n"
        "• Board outline is drawn using polygons\n";

static const char *text_sym =
        "This rule checks if the symbol contains all required objects:\n"
        "• Refdes & value texts\n"
        "• No off-grid pins\n"
        "• No unplaced pins\n"
        "• No overlapping pins\n"
        "• Pin placement\n";

static const char *text_net_ties =
        "This rule checks if all net ties are correctly represented on the board:\n"
        "• No missing net ties\n"
        "• Net tie connects to its designated nets\n";

static const char *text_board_connectivity =
        "This rule checks if all copper features of each net are connected to one another:\n"
        "• Connected by PTH holes such as vias or through-hole pins\n"
        "• Connected by shorted pads as specfied in the the shorted pads rule\n";

static const char *text_height_restrictions =
        "This rule checks the height of all packages in a height restriction polygon:\n"
        "• Packages must have a height set\n"
        "• Height must not exceed height restriction\n";

void RuleEditorPackageChecks::populate()
{
    Gtk::Label *editor = Gtk::manage(new Gtk::Label);
    switch (rule.get_id()) {
    case RuleID::PACKAGE_CHECKS:
        editor->set_text(text_pkg);
        break;

    case RuleID::PREFLIGHT_CHECKS:
        editor->set_text(text_preflight);
        break;

    case RuleID::SYMBOL_CHECKS:
        editor->set_text(text_sym);
        break;

    case RuleID::NET_TIES:
        editor->set_text(text_net_ties);
        break;

    case RuleID::BOARD_CONNECTIVITY:
        editor->set_text(text_board_connectivity);
        break;

    case RuleID::HEIGHT_RESTRICTIONS:
        editor->set_text(text_height_restrictions);
        break;

    default:
        break;
    }
    editor->set_xalign(0);
    editor->set_valign(Gtk::ALIGN_START);
    editor->set_margin_start(20);
    pack_start(*editor, true, true, 0);
    editor->show();
}
} // namespace horizon
