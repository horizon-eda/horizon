#include "pool_notebook.hpp"
#include "editors/editor_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "util/util.hpp"
#include "create_part_dialog.hpp"
#include "nlohmann/json.hpp"
#include "pool-prj-mgr/pool-prj-mgr-app_win.hpp"
#include "widgets/pool_browser_part.hpp"
#include "pool_remote_box.hpp"
#include "widgets/part_preview.hpp"
#include "widgets/where_used_box.hpp"
#include "part_wizard/part_wizard.hpp"

namespace horizon {
void PoolNotebook::handle_edit_part(const UUID &uu)
{
    if (!uu)
        return;
    UUID item_pool_uuid;
    auto path = pool.get_filename(ObjectType::PART, uu, &item_pool_uuid);
    appwin->spawn(PoolProjectManagerProcess::Type::PART, {path}, {}, pool_uuid && (item_pool_uuid != pool_uuid));
}

void PoolNotebook::handle_create_part()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    auto entity_uuid = browsers.at(ObjectType::ENTITY)->get_selected();
    auto package_uuid = browsers.at(ObjectType::PACKAGE)->get_selected();
    {
        CreatePartDialog dia(top, &pool, entity_uuid, package_uuid);
        if (dia.run() == Gtk::RESPONSE_OK) {
            entity_uuid = dia.get_entity();
            package_uuid = dia.get_package();
        }
        else {
            entity_uuid = UUID();
            package_uuid = UUID();
        }
    }
    if (!(entity_uuid && package_uuid))
        return;

    auto entity = pool.get_entity(entity_uuid);

    Part part(horizon::UUID::random());
    auto package = pool.get_package(package_uuid);
    part.attributes[Part::Attribute::MPN] = {false, entity->name};
    part.attributes[Part::Attribute::MANUFACTURER] = {false, entity->manufacturer};
    part.package = package;
    part.entity = entity;
    std::string fn = pool.get_tmp_filename(ObjectType::PART, part.uuid);
    save_json_to_file(fn, part.serialize());
    appwin->spawn(PoolProjectManagerProcess::Type::PART, {fn}, {}, false, true);
}

void PoolNotebook::handle_create_part_from_part(const UUID &uu)
{
    if (!uu)
        return;
    auto base_part = pool.get_part(uu);
    Part part(horizon::UUID::random());
    part.base = base_part;
    part.attributes[Part::Attribute::MPN] = {true, base_part->get_MPN()};
    part.attributes[Part::Attribute::MANUFACTURER] = {true, base_part->get_manufacturer()};
    part.attributes[Part::Attribute::VALUE] = {true, base_part->get_value()};
    part.attributes[Part::Attribute::DESCRIPTION] = {true, base_part->get_description()};
    part.attributes[Part::Attribute::DATASHEET] = {true, base_part->get_datasheet()};
    std::string fn = pool.get_tmp_filename(ObjectType::PART, part.uuid);
    save_json_to_file(fn, part.serialize());
    appwin->spawn(PoolProjectManagerProcess::Type::PART, {fn, pool.get_filename(ObjectType::PART, uu)}, {}, false,
                  true);
}

void PoolNotebook::handle_part_wizard()
{
    if (!part_wizard) {
        auto package_uuid = browsers.at(ObjectType::PACKAGE)->get_selected();
        part_wizard = PartWizard::create(package_uuid, base_path, &pool, appwin);
        part_wizard->present();
        part_wizard->signal_hide().connect([this] {
            auto files_saved = part_wizard->get_files_saved();
            if (files_saved.size()) {
                pool_update(nullptr, files_saved);
            }
            delete part_wizard;
            part_wizard = nullptr;
        });
    }
    else {
        part_wizard->present();
    }
}

void PoolNotebook::handle_duplicate_part(const UUID &uu)
{
    if (!uu)
        return;
    show_duplicate_window(ObjectType::PART, uu);
}

void PoolNotebook::construct_parts()
{
    auto br = Gtk::manage(new PoolBrowserPart(&pool));
    br->set_show_path(true);
    br->signal_activated().connect([this, br] { handle_edit_part(br->get_selected()); });

    br->show();
    browsers.emplace(ObjectType::PART, br);

    auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));

    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
    auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
    bbox->set_margin_bottom(8);
    bbox->set_margin_top(8);
    bbox->set_margin_start(8);
    bbox->set_margin_end(8);

    add_action_button("Create", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_create_part));
    add_action_button("Edit", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_edit_part));
    add_action_button("Duplicate", bbox, br, sigc::mem_fun(*this, &PoolNotebook::handle_duplicate_part));
    add_action_button("Create Part from Part", bbox, br,
                      sigc::mem_fun(*this, &PoolNotebook::handle_create_part_from_part));

    add_action_button("Part Wizard...", bbox, sigc::mem_fun(*this, &PoolNotebook::handle_part_wizard))
            ->get_style_context()
            ->add_class("suggested-action");

    if (remote_repo.size())
        add_action_button("Merge", bbox, br, [this](const UUID &uu) { remote_box->merge_item(ObjectType::PART, uu); });

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

    auto preview = Gtk::manage(new PartPreview(pool));
    preview->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    auto where_used = Gtk::manage(new WhereUsedBox(pool));
    where_used->property_margin() = 10;
    where_used->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));

    stack->add(*preview, "preview");
    stack->add(*where_used, "info");

    paned->add2(*stack);
    paned->show_all();

    br->signal_selected().connect([this, br, preview, where_used] {
        auto sel = br->get_selected();
        where_used->load(ObjectType::PART, sel);
        if (!sel) {
            preview->load(nullptr);
            return;
        }
        auto part = pool.get_part(sel);
        preview->load(part);
    });

    append_page(*paned, "Parts");
    install_search_once(paned, br);
}
} // namespace horizon
