#include "imp_symbol.hpp"
#include "canvas/canvas_gl.hpp"
#include "header_button.hpp"
#include "pool/part.hpp"
#include "symbol_preview/symbol_preview_window.hpp"
#include "symbol_preview/symbol_preview_expand_window.hpp"
#include "widgets/unplaced_box.hpp"
#include "core/tool_id.hpp"
#include "widgets/action_button.hpp"
#include "widgets/help_button.hpp"
#include "help_texts.hpp"

namespace horizon {
ImpSymbol::ImpSymbol(const std::string &symbol_filename, const std::string &pool_path)
    : ImpBase(pool_path), core_symbol(symbol_filename, *pool), symbol(core_symbol.get_symbol()), searcher(core_symbol)
{
    core = &core_symbol;
    core_symbol.signal_tool_changed().connect(sigc::mem_fun(*this, &ImpBase::handle_tool_change));
}

void ImpSymbol::canvas_update()
{
    canvas->update(core_symbol.get_canvas_data());
    symbol_preview_window->update(core_symbol.get_canvas_data());
    update_bbox_annotation();
}

void ImpSymbol::update_monitor()
{
    set_monitor_items({{ObjectType::UNIT, symbol.unit->uuid}});
}

void ImpSymbol::apply_preferences()
{
    symbol_preview_window->set_canvas_appearance(preferences.canvas_non_layer.appearance);
    symbol_preview_expand_window->set_canvas_appearance(preferences.canvas_non_layer.appearance);
    ImpBase::apply_preferences();
}

void ImpSymbol::construct()
{

    state_store = std::make_unique<WindowStateStore>(main_window, "imp-symbol");

    symbol_preview_window = new SymbolPreviewWindow(main_window);
    symbol_preview_window->set_text_placements(symbol.text_placements);
    symbol_preview_window->signal_changed().connect([this] { core_symbol.set_needs_save(); });
    core_symbol.signal_tool_changed().connect(
            [this](ToolID tool_id) { symbol_preview_window->set_can_load(tool_id == ToolID::NONE); });
    symbol_preview_window->signal_load().connect([this](int angle, bool mirror) {
        if (core_symbol.tool_is_active())
            return;
        auto pl = symbol_preview_window->get_text_placements();
        for (auto &it : symbol.texts) {
            auto key = std::make_tuple(angle, mirror, it.first);
            if (pl.count(key))
                it.second.placement = pl.at(key);
        }
        core_symbol.set_needs_save();
        core_symbol.rebuild();
        auto sel = canvas->get_selection();
        canvas_update();
        canvas->set_selection(sel);
    });

    symbol_preview_expand_window = new SymbolPreviewExpandWindow(main_window, core_symbol.get_symbol());
    core->signal_rebuilt().connect([this] { symbol_preview_expand_window->update(); });

    {
        auto button = Gtk::manage(new Gtk::Button("Preview…"));
        main_window->header->pack_start(*button);
        button->show();
        button->signal_clicked().connect([this] { symbol_preview_window->present(); });
    }

    unplaced_box = Gtk::manage(new UnplacedBox("Pin"));
    unplaced_box->show();
    main_window->left_panel->pack_end(*unplaced_box, true, true, 0);
    unplaced_box->signal_place().connect([this](const auto &items) {
        std::set<SelectableRef> sel;
        for (const auto &it : items) {
            sel.emplace(it.at(0), ObjectType::SYMBOL_PIN);
        }
        this->tool_begin(ToolID::MAP_PIN, true, sel);
    });

    core_symbol.signal_rebuilt().connect(sigc::mem_fun(*this, &ImpSymbol::update_unplaced));
    update_unplaced();
    core_symbol.signal_tool_changed().connect(
            [this](ToolID tool_id) { unplaced_box->set_sensitive(tool_id == ToolID::NONE); });

    header_button = Gtk::manage(new HeaderButton);
    main_window->header->set_custom_title(*header_button);
    header_button->show();
    header_button->signal_closed().connect(sigc::mem_fun(*this, &ImpSymbol::update_header));

    name_entry = header_button->add_entry("Name");
    name_entry->set_text(symbol.name);
    name_entry->set_width_chars(std::min<int>(symbol.name.size(), 20));
    name_entry->signal_changed().connect([this] { core_symbol.set_needs_save(); });
    name_entry->signal_activate().connect(sigc::mem_fun(*this, &ImpSymbol::update_header));

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
        unit_label = Gtk::manage(new Gtk::Label(symbol.unit->name));
        unit_label->set_xalign(0);
        unit_label->set_selectable(true);
        box->pack_start(*unit_label, true, true, 0);
        auto button = create_action_button(make_action(ToolID::CHANGE_UNIT));
        button->set_label("Change");
        box->pack_start(*button, false, false, 0);
        box->show_all();
        header_button->add_widget("Unit", box);
    }

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));

        can_expand_switch = Gtk::manage(new Gtk::Switch);
        can_expand_switch->set_active(symbol.can_expand);
        can_expand_switch->set_halign(Gtk::ALIGN_START);
        can_expand_switch->set_valign(Gtk::ALIGN_CENTER);
        can_expand_switch->property_active().signal_changed().connect([this] {
            core_symbol.set_needs_save();
            const bool a = can_expand_switch->get_active();
            expand_preview_button->set_sensitive(a);
            if (!a)
                symbol_preview_expand_window->hide();
        });
        can_expand_switch->show();
        box->pack_start(*can_expand_switch, false, false, 0);
        auto help_button = Gtk::manage(new HelpButton(HelpTexts::SYMBOL_CAN_EXPAND));
        help_button->show();
        box->pack_start(*help_button, false, false, 0);

        expand_preview_button = Gtk::manage(new Gtk::Button("Preview"));
        expand_preview_button->signal_clicked().connect([this] {
            symbol_preview_expand_window->present();
            symbol_preview_expand_window->update();
            symbol_preview_expand_window->zoom_to_fit();
        });
        expand_preview_button->show();
        expand_preview_button->set_sensitive(symbol.can_expand);
        box->pack_end(*expand_preview_button, false, false, 0);

        header_button->add_widget("Can expand", box);
    }

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        box->get_style_context()->add_class("linked");
        auto rb_primary = Gtk::manage(new Gtk::RadioButton("Primary"));
        rb_primary->set_mode(false);
        auto rb_alt = Gtk::manage(new Gtk::RadioButton("Alt."));
        rb_alt->set_mode(false);
        auto rb_both = Gtk::manage(new Gtk::RadioButton("Both"));
        rb_both->set_mode(false);
        rb_alt->join_group(*rb_primary);
        rb_both->join_group(*rb_primary);
        box->pack_start(*rb_primary, true, true, 0);
        box->pack_start(*rb_alt, true, true, 0);
        box->pack_start(*rb_both, true, true, 0);
        box->show_all();
        header_button->add_widget("Pin display", box);
        rb_primary->signal_toggled().connect([this, rb_primary] {
            if (rb_primary->get_active()) {
                core_symbol.set_pin_display_mode(Symbol::PinDisplayMode::PRIMARY);
                canvas_update_from_pp();
            }
        });
        rb_alt->signal_toggled().connect([this, rb_alt] {
            if (rb_alt->get_active()) {
                core_symbol.set_pin_display_mode(Symbol::PinDisplayMode::ALT);
                canvas_update_from_pp();
            }
        });
        rb_both->signal_toggled().connect([this, rb_both] {
            if (rb_both->get_active()) {
                core_symbol.set_pin_display_mode(Symbol::PinDisplayMode::BOTH);
                canvas_update_from_pp();
            }
        });
        core_symbol.signal_tool_changed().connect(
                [box](ToolID tool_id) { box->set_sensitive(tool_id == ToolID::NONE); });
    }

    core->signal_rebuilt().connect([this] { unit_label->set_text(symbol.unit->name); });

    core->signal_save().connect([this] {
        symbol.name = name_entry->get_text();
        symbol.text_placements = symbol_preview_window->get_text_placements();
        symbol.can_expand = can_expand_switch->get_active();
    });
    grid_controller->disable();
    update_monitor();

    connect_action(ActionID::EDIT_UNIT,
                   [this](const auto &a) { this->edit_pool_item(ObjectType::UNIT, symbol.unit->uuid); });
    set_action_sensitive(make_action(ActionID::EDIT_UNIT), sockets_connected);

    hamburger_menu->append("Change unit", "win.change_unit");
    add_tool_action(ToolID::CHANGE_UNIT, "change_unit");

    if (sockets_connected) {
        hamburger_menu->append("Edit unit", "win.edit_unit");
        main_window->add_action("edit_unit", [this] { trigger_action(ActionID::EDIT_UNIT); });
    }

    update_header();

    bbox_annotation = canvas->create_annotation();
    bbox_annotation->set_visible(true);
    bbox_annotation->on_top = false;
    bbox_annotation->set_display(LayerDisplay(true, LayerDisplay::Mode::OUTLINE));

    add_action_button(make_action(ToolID::DRAW_LINE_RECTANGLE));
    add_action_button(make_action(ToolID::PLACE_REFDES_AND_VALUE));
    {
        auto &x = add_action_button(make_action(ToolID::DRAW_LINE));
        x.add_action(make_action(ToolID::DRAW_LINE_CIRCLE));
        x.add_action(make_action(ToolID::DRAW_ARC));
    }
    add_action_button_polygon();
    add_action_button(make_action(ToolID::PLACE_DOT));
    add_action_button(make_action(ToolID::PLACE_TEXT));
    add_action_button(make_action(ToolID::RESIZE_SYMBOL));
}

void ImpSymbol::update_header()
{
    header_button->set_label(name_entry->get_text());
    set_window_title(name_entry->get_text());
}

void ImpSymbol::update_unplaced()
{
    const auto &pins_from_symbol = symbol.pins;
    std::map<UUIDPath<2>, std::string> unplaced;
    for (const auto &it : symbol.unit->pins) {
        if (pins_from_symbol.count(it.first) == 0) {
            unplaced.emplace(it.first, it.second.primary_name);
        }
    }
    unplaced_box->update(unplaced);
}

void ImpSymbol::update_bbox_annotation()
{
    if (!bbox_annotation)
        return;
    Coordi a;
    for (const auto &it : symbol.pins) {
        const auto &p = it.second.position;
        a = Coordi::max(a, {std::abs(p.x), std::abs(p.y)});
    }
    Coordi b = a * -1;
    bbox_annotation->clear();
    bbox_annotation->draw_line(a, Coordf(b.x, a.y), ColorP::PIN_HIDDEN, 0);
    bbox_annotation->draw_line(Coordf(b.x, a.y), b, ColorP::PIN_HIDDEN, 0);
    bbox_annotation->draw_line(b, Coordf(a.x, b.y), ColorP::PIN_HIDDEN, 0);
    bbox_annotation->draw_line(Coordf(a.x, b.y), a, ColorP::PIN_HIDDEN, 0);
}


void ImpSymbol::handle_selection_cross_probe()
{
    json j;
    j["op"] = "symbol-select";
    j["unit"] = (std::string)symbol.unit->uuid;
    auto pins = json::array();
    for (const auto &it : canvas->get_selection()) {
        if (it.type == ObjectType::SYMBOL_PIN) {
            pins.push_back((std::string)it.uuid);
        }
    }
    j["pins"] = pins;
    send_json(j);
}
} // namespace horizon
