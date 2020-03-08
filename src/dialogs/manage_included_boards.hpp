#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
namespace horizon {


class ManageIncludedBoardsDialog : public Gtk::Dialog {
public:
    friend class BoardEditor;
    ManageIncludedBoardsDialog(Gtk::Window *parent, class Board &b);

private:
    class Board &board;
    Gtk::ListBox *listbox = nullptr;
    void handle_add_board();
    std::string ask_filename(const std::string &fn = "");
};
} // namespace horizon
