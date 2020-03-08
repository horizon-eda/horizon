#include "manage_included_boards.hpp"
#include <iostream>
#include <deque>
#include <algorithm>
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "board/board.hpp"

namespace horizon {

class BoardEditor : public Gtk::Box {
public:
    BoardEditor(IncludedBoard &ib, ManageIncludedBoardsDialog &p)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4), inc(ib), parent(p), brd(parent.board)
    {

        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));

        auto lbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
        la = Gtk::manage(new Gtk::Label);
        la->set_xalign(0);
        la->set_margin_start(4);
        lbox->pack_start(*la, true, true, 0);
        update_label();

        auto reload_button = Gtk::manage(new Gtk::Button("Reload"));
        reload_button->signal_clicked().connect([this] {
            inc.reload();
            update_label();
        });
        lbox->pack_end(*reload_button, false, false, 0);

        box->pack_start(*lbox, true, true, 0);

        auto ebox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
        ebox->get_style_context()->add_class("linked");
        auto entry = Gtk::manage(new Gtk::Entry);
        bind_widget(entry, inc.project_filename);
        ebox->pack_start(*entry, true, true, 0);
        auto button = Gtk::manage(new Gtk::Button("Browse..."));
        button->signal_clicked().connect([this, entry] {
            auto fn = parent.ask_filename(entry->get_text());
            if (fn.size()) {
                entry->set_text(fn);
                inc.reload();
                update_label();
            }
        });
        ebox->pack_start(*button, false, false, 0);

        box->pack_start(*ebox, true, true, 0);
        box->show_all();

        pack_start(*box, true, true, 0);
        auto delete_button = Gtk::manage(new Gtk::Button());
        delete_button->set_valign(Gtk::ALIGN_CENTER);
        delete_button->set_margin_start(4);
        delete_button->set_image_from_icon_name("list-remove-symbolic", Gtk::ICON_SIZE_BUTTON);
        delete_button->set_sensitive(
                std::count_if(brd.board_panels.begin(), brd.board_panels.end(),
                              [this](const auto &x) { return x.second.included_board->uuid == inc.uuid; })
                == 0);
        pack_start(*delete_button, false, false, 0);
        delete_button->signal_clicked().connect([this] {
            brd.included_boards.erase(inc.uuid);
            delete this->get_parent();
        });

        set_margin_start(8);
        set_margin_end(8);
        set_margin_top(4);
        set_margin_bottom(4);
        show_all();
    }

private:
    IncludedBoard &inc;
    ManageIncludedBoardsDialog &parent;
    Board &brd;
    Gtk::Label *la = nullptr;
    void update_label()
    {
        la->set_label(inc.get_name());
    }
};

ManageIncludedBoardsDialog::ManageIncludedBoardsDialog(Gtk::Window *parent, Board &br)
    : Gtk::Dialog("Manage boards", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      board(br)
{
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(400, 300);


    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    auto add_button = Gtk::manage(new Gtk::Button("Add board"));
    add_button->signal_clicked().connect(sigc::mem_fun(*this, &ManageIncludedBoardsDialog::handle_add_board));
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
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    listbox = Gtk::manage(new Gtk::ListBox());
    listbox->set_selection_mode(Gtk::SELECTION_NONE);
    listbox->set_header_func(sigc::ptr_fun(header_func_separator));
    sc->add(*listbox);
    box->pack_start(*sc, true, true, 0);

    for (auto &it : board.included_boards) {
        auto ed = Gtk::manage(new BoardEditor(it.second, *this));
        listbox->add(*ed);
    }

    get_content_area()->pack_start(*box, true, true, 0);
    get_content_area()->set_border_width(0);
    show_all();
}

void ManageIncludedBoardsDialog::handle_add_board()
{
    auto fn = ask_filename();
    if (fn.size()) {
        auto uu = UUID::random();
        auto &x = board.included_boards
                          .emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, fn))
                          .first->second;
        auto ed = Gtk::manage(new BoardEditor(x, *this));
        listbox->add(*ed);
    }
}

std::string ManageIncludedBoardsDialog::ask_filename(const std::string &fn)
{
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Select Project", GTK_WINDOW(gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Project");
    filter->add_pattern("*.hprj");
    chooser->add_filter(filter);
    if (fn.size())
        chooser->set_filename(fn);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        return chooser->get_filename();
    }
    else {
        return "";
    }
}

} // namespace horizon
