#include "pool_notebook.hpp"
#include "util/util.hpp"
#include <nlohmann/json.hpp>
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "widgets/pool_browser_padstack.hpp"
#include "widgets/padstack_preview.hpp"
#include "widgets/where_used_box.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
void PoolNotebook::handle_edit_padstack(const UUID &uu)
{
    edit_item(ObjectType::PADSTACK, uu);
}

void PoolNotebook::handle_create_padstack()
{
    Padstack padstack(horizon::UUID::random());
    std::string fn = pool.get_tmp_filename(ObjectType::PADSTACK, padstack.uuid);
    save_json_to_file(fn, padstack.serialize());

    appwin.spawn(PoolProjectManagerProcess::Type::IMP_PADSTACK, {fn}, PoolProjectManagerAppWindow::SpawnFlags::TEMP);
}

void PoolNotebook::handle_duplicate_padstack(const UUID &uu)
{
    handle_duplicate_item(ObjectType::PADSTACK, uu);
}

void PoolNotebook::construct_padstacks()
{
    auto br = Gtk::manage(new PoolBrowserPadstack(pool));
    br->set_show_path(true);
    using PT = Padstack::Type;
    br->set_padstacks_included({PT::BOTTOM, PT::HOLE, PT::MECHANICAL, PT::THROUGH, PT::TOP, PT::VIA});
    br->signal_activated().connect([this, br] { handle_edit_padstack(br->get_selected()); });

    br->show();
    browsers.emplace(ObjectType::PADSTACK, br);

    auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    bbox->set_margin_bottom(8);
    bbox->set_margin_top(8);
    bbox->set_margin_start(8);
    bbox->set_margin_end(8);

    add_action_button("Create", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_create_padstack));
    add_action_button("Edit", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_edit_padstack));
    add_action_button("Duplicate", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_duplicate_padstack));
    add_merge_button(bbox, br);

    auto stack = Gtk::manage(new Gtk::Stack);
    add_preview_stack_switcher(bbox, stack);

    bbox->show_all();

    box->pack_start(*bbox, false, false, 0);

    auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
    sep->show();
    box->pack_start(*sep, false, false, 0);
    box->pack_start(*br, true, true, 0);
    box->show();

    paned->add1(*box);
    paned->child_property_shrink(*box) = false;


    auto preview = Gtk::manage(new PadstackPreview(pool));
    preview->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    auto where_used = Gtk::manage(new WhereUsedBox(pool));
    where_used->property_margin() = 10;
    where_used->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    stack->add(*preview, "preview");
    stack->add(*where_used, "info");

    paned->add2(*stack);
    paned->show_all();

    br->signal_selected().connect([br, preview, where_used] {
        auto sel = br->get_selected();
        preview->load(sel);
        where_used->load(ObjectType::PADSTACK, sel);
    });

    append_page(*paned, "Padstacks");
    install_search_once(paned, br);
    create_paned_state_store(paned, "padstacks");
}

} // namespace horizon
