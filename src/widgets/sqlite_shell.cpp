#include "sqlite_shell.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
SQLiteShellWindow::SQLiteShellWindow(const std::string &db_path) : Gtk::Window(), db(db_path, SQLITE_OPEN_READWRITE)
{
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    auto header = Gtk::manage(new Gtk::HeaderBar());
    header->set_show_close_button(true);
    header->set_title("SQLite Shell");
    set_titlebar(*header);
    header->show();
    header->set_subtitle(db_path);
    set_default_geometry(600, 600);

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    entry = Gtk::manage(new Gtk::Entry);
    entry->signal_activate().connect(sigc::mem_fun(*this, &SQLiteShellWindow::run));
    box->pack_start(*entry, false, false, 0);
    entry->property_margin() = 10;
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*sep, false, false, 0);
    }
    auto sc = Gtk::manage(new Gtk::ScrolledWindow);
    box->pack_start(*sc, true, true, 0);

    store = Gtk::ListStore::create(list_columns);

    tree_view = Gtk::manage(new Gtk::TreeView(store));
    sc->add(*tree_view);

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        box->pack_start(*sep, false, false, 0);
    }

    status_label = Gtk::manage(new Gtk::Label());
    label_set_tnum(status_label);
    status_label->set_xalign(0);
    status_label->property_margin() = 2;
    status_label->show();
    box->pack_start(*status_label, false, false, 0);

    box->show_all();
    add(*box);
}

void SQLiteShellWindow::run()
{
    store->clear();
    tree_view->remove_all_columns();
    try {
        const std::string qs = entry->get_text();
        SQLite::Query q(db, qs);

        const size_t ncols = q.get_column_count();
        for (size_t i = 0; i < ncols; i++) {
            const auto column_name = q.get_column_name(i);
            auto cr = Gtk::manage(new Gtk::CellRendererText());

            auto tvc = Gtk::manage(new Gtk::TreeViewColumn(column_name, *cr));
            tvc->set_cell_data_func(*cr, [this, i](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
                Gtk::TreeModel::Row row = *it;
                auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
                mcr->property_text() = row.get_value(list_columns.columns).at(i);
            });
            tree_view->append_column(*tvc);
        }
        size_t nrows = 0;
        while (q.step()) {
            Gtk::TreeModel::Row row = *store->append();
            std::vector<std::string> cols;
            cols.reserve(ncols);
            for (size_t i = 0; i < ncols; i++) {
                cols.push_back(q.get<std::string>(i));
            }
            row[list_columns.columns] = cols;
            nrows++;
        }
        status_label->set_text(std::to_string(ncols) + " column(s), " + std::to_string(nrows) + " row(s)");
    }
    catch (const SQLite::Error &e) {
        status_label->set_text("SQLite error: " + std::string(e.what()) + " (" + std::to_string(e.rc) + ")");
    }
    catch (const std::exception &e) {
        status_label->set_text("error: " + std::string(e.what()));
    }
    catch (...) {
        status_label->set_text("unknown error");
    }
}

} // namespace horizon
