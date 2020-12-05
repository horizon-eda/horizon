#include "import.hpp"
#include "board/board_rules_import.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "widgets/net_class_button.hpp"

namespace horizon {

using NetClassMap = std::map<UUID, UUID>;

class NetClassMapper : public Gtk::Box {
public:
    NetClassMapper(const Block &block, const BoardRulesImportInfo &import_info);
    const NetClassMap &get_net_classes() const
    {
        return net_classes;
    }

private:
    NetClassMap net_classes;
    const Block &block;
    const BoardRulesImportInfo &import_info;
};

NetClassMapper::NetClassMapper(const Block &b, const BoardRulesImportInfo &i)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), block(b), import_info(i)
{
    auto header = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
    auto sg1 = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    auto sg2 = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    header->get_style_context()->add_class("rule-import-header");
    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_markup("<b>Rules net class</b>");
        la->get_style_context()->add_class("dim-label");
        header->pack_start(*la, false, false, 0);
        la->show();
        la->set_xalign(0);
        sg1->add_widget(*la);
    }
    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_markup("<b>Board net class</b>");
        la->get_style_context()->add_class("dim-label");
        header->pack_start(*la, false, false, 0);
        la->show();
        la->set_xalign(0);
        sg2->add_widget(*la);
    }
    header->show();
    pack_start(*header, false, false, 0);
    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    pack_start(*sc, true, true, 0);
    sc->show();

    auto listbox = Gtk::manage(new Gtk::ListBox);
    listbox->set_selection_mode(Gtk::SELECTION_NONE);
    sc->add(*listbox);
    listbox->show();

    for (const auto &[uu, name] : import_info.net_classes) {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
        box->property_margin() = 5;
        box->set_margin_start(10);
        box->set_margin_end(10);
        {
            auto la = Gtk::manage(new Gtk::Label(name));
            la->show();
            la->set_xalign(1);
            la->set_margin_end(21);
            sg1->add_widget(*la);
            box->pack_start(*la, false, false, 0);
        }
        {
            auto bu = Gtk::manage(new NetClassButton(block));
            bu->show();
            const UUID nc_import = uu;
            bu->signal_net_class_changed().connect(
                    [this, nc_import](const UUID &nc_board) { net_classes[nc_import] = nc_board; });
            UUID nc = block.net_class_default->uuid;

            // try to match net class by name
            for (const auto &[uu2, net_class] : block.net_classes) {
                if (net_class.name == name) {
                    nc = uu2;
                    break;
                }
            }
            bu->set_net_class(nc);

            sg2->add_widget(*bu);
            box->pack_start(*bu, true, true, 0);
        }

        box->show();
        listbox->append(*box);
    }
}

class MyRuleImportMap : public RuleImportMap {
public:
    MyRuleImportMap(const NetClassMap &m) : net_class_map(m)
    {
    }

    UUID get_net_class(const UUID &uu) const override
    {
        return net_class_map.at(uu);
    }
    int get_order(int order) const override
    {
        return order + 10000;
    }
    bool is_imported() const override
    {
        return true;
    }

private:
    NetClassMap net_class_map;
};

class BoardRuleImportDialog : public RuleImportDialog {
public:
    BoardRuleImportDialog(const BoardRulesImportInfo &info, const Board &brd)
        : RuleImportDialog("Import board rules", Gtk::DIALOG_MODAL | Gtk::DIALOG_USE_HEADER_BAR)
    {
        get_content_area()->set_border_width(0);
        if (info.n_inner_layers != brd.get_n_inner_layers()) {
            add_button("Cancel", Gtk::RESPONSE_CANCEL);
            auto la = Gtk::manage(new Gtk::Label);
            la->set_text("Can't import rules\nBoard inner layers: " + std::to_string(brd.get_n_inner_layers())
                         + "\nRules inner layers: " + std::to_string(info.n_inner_layers));
            la->property_margin() = 20;
            la->show();
            get_content_area()->pack_start(*la, false, false, 0);
            return;
        }
        set_default_size(-1, 300);
        add_button("Import", Gtk::RESPONSE_OK);
        add_button("Cancel", Gtk::RESPONSE_CANCEL);
        set_default_response(Gtk::RESPONSE_OK);

        net_class_mapper = Gtk::manage(new NetClassMapper(*brd.block, info));
        net_class_mapper->show();
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
        box->show();
        box->pack_start(*net_class_mapper, false, false, 0);

        {
            auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL));
            sep->show();
            box->pack_start(*sep, false, false, 0);
        }

        auto box2 = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
        box2->property_margin() = 10;
        box->pack_start(*box2, false, false, 0);
        box2->show_all();

        auto name_label = Gtk::manage(new Gtk::Label(info.name));
        {
            Pango::AttrList list;
            auto attr = Pango::Attribute::create_attr_weight(Pango::WEIGHT_BOLD);
            list.insert(attr);
            name_label->set_attributes(list);
        }
        name_label->set_xalign(0);
        box2->pack_start(*name_label, false, false, 0);

        auto notes_label = Gtk::manage(new Gtk::Label(info.notes));
        notes_label->set_xalign(0);
        notes_label->set_yalign(0);
        notes_label->set_line_wrap_mode(Pango::WRAP_WORD);
        box2->pack_start(*notes_label, false, false, 0);
        box2->show_all();


        get_content_area()->pack_start(*box, true, true, 0);
    }
    std::unique_ptr<RuleImportMap> get_import_map() const override
    {
        return std::make_unique<MyRuleImportMap>(net_class_mapper->get_net_classes());
    }

private:
    NetClassMapper *net_class_mapper = nullptr;
};

std::unique_ptr<RuleImportDialog> make_rule_import_dialog(const RulesImportInfo &info, class IDocument &doc)
{
    const auto pinfo = &info;
    if (auto rinfo = dynamic_cast<const BoardRulesImportInfo *>(pinfo)) {
        auto &doc_board = dynamic_cast<IDocumentBoard &>(doc);
        auto dia = std::make_unique<BoardRuleImportDialog>(*rinfo, *doc_board.get_board());
        return dia;
    }

    throw std::runtime_error("unsupported info");
}
} // namespace horizon
