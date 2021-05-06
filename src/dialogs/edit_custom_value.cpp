#include "edit_custom_value.hpp"
#include "schematic/schematic_symbol.hpp"
#include "util/str_util.hpp"
#include "util/gtk_util.hpp"
#include "pool/part.hpp"

namespace horizon {

class InsertBox : public Gtk::Box {
public:
    InsertBox(const std::string &prefix, const std::string &label, const std::string &a_ins)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5), text_insert(a_ins)
    {
        {
            auto la = Gtk::manage(new Gtk::Label(prefix));
            la->set_xalign(0);
            la->show();
            pack_start(*la, false, false, 0);
        }
        {
            auto la = Gtk::manage(new Gtk::Label(label));
            la->set_xalign(1);
            la->show();
            pack_start(*la, true, true, 0);
        }
        auto img = Gtk::manage(new Gtk::Image());
        img->set_from_icon_name("go-next-symbolic", Gtk::ICON_SIZE_BUTTON);
        img->show();
        pack_start(*img, false, false, 0);
        property_margin() = 5;
    }

    const std::string text_insert;
};


EditCustomValueDialog::EditCustomValueDialog(Gtk::Window *parent, SchematicSymbol &asym)
    : Gtk::Dialog("Edit custom value", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      sym(asym)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(500, 360);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    auto sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

    {
        auto sc = Gtk::manage(new Gtk::ScrolledWindow);
        sc->set_shadow_type(Gtk::SHADOW_NONE);
        sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
        auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
        box2->property_margin() = 10;
        {
            auto la = Gtk::manage(new Gtk::Label);
            la->set_markup("<b>Insert</b>");
            la->get_style_context()->add_class("dim-label");
            box2->pack_start(*la, false, false, 0);
        }

        auto listbox = Gtk::manage(new Gtk::ListBox);
        listbox->set_header_func(&header_func_separator);
        listbox->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
            if (auto ch = dynamic_cast<InsertBox *>(row->get_child())) {
                view->get_buffer()->insert_at_cursor(ch->text_insert);
                view->grab_focus();
            }
        });

        auto fr = Gtk::manage(new Gtk::Frame);
        fr->add(*listbox);

        box2->pack_start(*fr, false, false, 0);
        listbox->set_selection_mode(Gtk::SELECTION_NONE);
        listbox->set_activate_on_single_click(true);
        auto make_insert_button = [listbox](const std::string &label, const std::string &ins,
                                            const std::string &prefix = "") {
            auto la = Gtk::manage(new InsertBox(prefix, label, ins));
            listbox->append(*la);
        };

        make_insert_button("Value", "${value}\n");
        if (sym.component->part) {
            make_insert_button("MPN", "${mpn}\n");
            make_insert_button("Manufacturer", "${mfr}\n");
            make_insert_button("Description", "${desc}\n");
            make_insert_button("Package", "${pkg}\n");
            for (const auto &[k, v] : sym.component->part->parametric_formatted) {
                make_insert_button(v.display_name, "${p:" + k + "}\n", "Param:");
            }
        }

        sc->add(*box2);
        box->pack_start(*sc, false, false, 0);
    }

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL));
        box->pack_start(*sep, false, false, 0);
    }
    {
        auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        {
            auto la = Gtk::manage(new Gtk::Label);
            la->set_markup("<b>Edit</b>");
            la->get_style_context()->add_class("dim-label");
            la->property_margin() = 10;
            box2->pack_start(*la, false, false, 0);
        }
        {
            auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
            box2->pack_start(*sep, false, false, 0);
        }
        auto sc = Gtk::manage(new Gtk::ScrolledWindow);
        sc->set_shadow_type(Gtk::SHADOW_NONE);
        sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
        view = Gtk::manage(new Gtk::TextView);
        view->set_top_margin(10);
        view->set_bottom_margin(10);
        view->set_left_margin(10);
        view->set_right_margin(10);
        sc->add(*view);
        sc->set_min_content_width(200);
        box2->pack_start(*sc, true, true, 0);
        box->pack_start(*box2, true, true, 0);
        if (sym.custom_value.size() == 0) {
            sym.custom_value = "${value}\n";
        }
        view->get_buffer()->set_text(sym.custom_value);
        view->get_buffer()->signal_changed().connect([this] {
            sym.custom_value = view->get_buffer()->get_text();
            trim(sym.custom_value);
            update_preview();
        });
    }
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL));
        box->pack_start(*sep, false, false, 0);
    }
    {
        auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
        {
            auto la = Gtk::manage(new Gtk::Label);
            la->set_markup("<b>Preview</b>");
            la->get_style_context()->add_class("dim-label");
            box2->pack_start(*la, false, false, 0);
            la->property_margin() = 10;
        }
        {
            auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
            box2->pack_start(*sep, false, false, 0);
        }
        preview_label = Gtk::manage(new Gtk::Label);
        preview_label->set_xalign(0);
        preview_label->property_margin() = 10;
        box2->pack_start(*preview_label, false, false, 0);

        box->pack_start(*box2, true, true, 0);
    }
    update_preview();

    get_content_area()->set_border_width(0);
    get_content_area()->pack_start(*box, true, true, 0);

    show_all();
}

void EditCustomValueDialog::update_preview()
{
    preview_label->set_text(sym.get_custom_value());
}

} // namespace horizon
