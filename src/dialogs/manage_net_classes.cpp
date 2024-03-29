#include "manage_net_classes.hpp"
#include "block/block.hpp"
#include "blocks/iblock_provider.hpp"

namespace horizon {

class NetClassEditor : public Gtk::Box {
public:
    NetClassEditor(NetClass &nc, IBlockProvider &bl)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4), net_class(nc), blocks(bl)
    {
        entry = Gtk::manage(new Gtk::Entry());
        entry->set_text(nc.name);
        entry->signal_changed().connect([this] { net_class.name = entry->get_text(); });
        pack_start(*entry, true, true, 0);

        delete_button = Gtk::manage(new Gtk::Button());
        delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
        delete_button->set_sensitive(blocks.get_top_block().net_class_default != &nc);
        pack_start(*delete_button, false, false, 0);
        delete_button->signal_clicked().connect([this] {
            for (auto [uu_block, block] : blocks.get_blocks()) {
                for (auto &[uu, net] : block->nets) {
                    if (net.net_class->uuid == net_class.uuid) {
                        net.net_class = block->net_class_default;
                    }
                }
                block->net_classes.erase(net_class.uuid);
            }
            delete this->get_parent();
        });


        set_margin_start(4);
        set_margin_end(4);
        set_margin_top(4);
        set_margin_bottom(4);
        show_all();
    }
    Gtk::Button *delete_button = nullptr;

    void focus()
    {
        entry->grab_focus();
    }

private:
    NetClass &net_class;
    IBlockProvider &blocks;
    Gtk::Entry *entry = nullptr;
};

static void header_fun(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before)
{
    if (before && !row->get_header()) {
        auto ret = Gtk::manage(new Gtk::Separator);
        row->set_header(*ret);
    }
}

ManageNetClassesDialog::ManageNetClassesDialog(Gtk::Window *parent, IBlockProvider &bl)
    : Gtk::Dialog("Manage net classes", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      blocks(bl)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(400, 300);


    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    auto add_button = Gtk::manage(new Gtk::Button("Add net class"));
    add_button->signal_clicked().connect(sigc::mem_fun(*this, &ManageNetClassesDialog::handle_add_net_class));
    add_button->set_halign(Gtk::ALIGN_START);
    add_button->set_margin_bottom(8);
    add_button->set_margin_top(8);
    add_button->set_margin_start(8);
    add_button->set_margin_end(8);

    box->pack_start(*add_button, false, false, 0);

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*sep, false, false, 0);
    }

    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    listbox = Gtk::manage(new Gtk::ListBox());
    listbox->set_selection_mode(Gtk::SELECTION_NONE);
    listbox->set_header_func(&header_fun);
    sc->add(*listbox);
    box->pack_start(*sc, true, true, 0);


    for (auto &it : blocks.get_top_block().net_classes) {
        auto nce = Gtk::manage(new NetClassEditor(it.second, blocks));
        listbox->add(*nce);
    }

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);
    show_all();
}

void ManageNetClassesDialog::handle_add_net_class()
{
    auto uu = UUID::random();
    auto &x =
            blocks.get_top_block()
                    .net_classes.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu))
                    .first->second;
    x.name = "fixme";

    auto nce = Gtk::manage(new NetClassEditor(x, blocks));
    listbox->prepend(*nce);
    nce->focus();
}
} // namespace horizon
