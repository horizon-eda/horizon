#include "imp.hpp"
#include "widgets/action_button.hpp"
#include "core/tool_id.hpp"
#include "util/util.hpp"
#include "in_tool_action_catalog.hpp"
#include "logger/logger.hpp"

namespace horizon {
void ImpBase::init_action()
{
    for (const auto &it : action_catalog) {
        if ((it.first.first == ActionID::TOOL) && (it.second.availability & get_editor_type_for_action())) {
            connect_action(it.first.second);
        }
    }
    connect_action(ActionID::PAN_LEFT, sigc::mem_fun(*this, &ImpBase::handle_pan_action));
    connect_action(ActionID::PAN_RIGHT, sigc::mem_fun(*this, &ImpBase::handle_pan_action));
    connect_action(ActionID::PAN_UP, sigc::mem_fun(*this, &ImpBase::handle_pan_action));
    connect_action(ActionID::PAN_DOWN, sigc::mem_fun(*this, &ImpBase::handle_pan_action));

    connect_action(ActionID::ZOOM_IN, sigc::mem_fun(*this, &ImpBase::handle_zoom_action));
    connect_action(ActionID::ZOOM_OUT, sigc::mem_fun(*this, &ImpBase::handle_zoom_action));
}

ActionButton &ImpBase::add_action_button(ActionToolID action)
{
    main_window->set_use_action_bar(true);
    auto ab = Gtk::manage(new ActionButton(action, action_connections));
    ab->show();
    ab->signal_action().connect([this](auto act) {
        force_end_tool();
        this->trigger_action(act);
    });
    main_window->action_bar_box->pack_start(*ab, false, false, 0);
    action_buttons.push_back(ab);
    return *ab;
}

ActionButtonMenu &ImpBase::add_action_button_menu(const char *icon_name)
{
    main_window->set_use_action_bar(true);
    auto ab = Gtk::manage(new ActionButtonMenu(icon_name, action_connections));
    ab->show();
    ab->signal_action().connect([this](auto act) {
        force_end_tool();
        this->trigger_action(act);
    });
    main_window->action_bar_box->pack_start(*ab, false, false, 0);
    action_buttons.push_back(ab);
    return *ab;
}

ActionButton &ImpBase::add_action_button_polygon()
{
    auto &b = add_action_button(make_action(ToolID::DRAW_POLYGON));
    b.add_action(make_action(ToolID::DRAW_POLYGON_RECTANGLE));
    b.add_action(make_action(ToolID::DRAW_POLYGON_CIRCLE));
    return b;
}

ActionButton &ImpBase::add_action_button_line()
{
    auto &b = add_action_button(make_action(ToolID::DRAW_LINE));
    b.add_action(make_action(ToolID::DRAW_LINE_RECTANGLE));
    b.add_action(make_action(ToolID::DRAW_LINE_CIRCLE));
    b.add_action(make_action(ToolID::DRAW_ARC));
    return b;
}

void ImpBase::tool_bar_set_actions(const std::vector<ActionLabelInfo> &labels)
{
    if (in_tool_action_label_infos != labels) {
        tool_bar_clear_actions();
        for (const auto &it : labels) {
            tool_bar_append_action(it.action1, it.action2, it.label);
        }

        in_tool_action_label_infos = labels;
    }
}

void ImpBase::tool_bar_append_action(InToolActionID action1, InToolActionID action2, const std::string &s)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
    if (any_of(action1, {InToolActionID::LMB, InToolActionID::RMB})) {
        std::string icon_name = "action-";
        if (action1 == InToolActionID::LMB) {
            icon_name += "lmb";
        }
        else {
            icon_name += "rmb";
        }
        icon_name += "-symbolic";
        auto img = Gtk::manage(new Gtk::Image);
        img->set_from_icon_name(icon_name, Gtk::ICON_SIZE_BUTTON);
        img->show();
        box->pack_start(*img, false, false, 0);
    }
    else {
        auto key_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
        for (const auto action : {action1, action2}) {
            if (action != InToolActionID::NONE) {
                const auto &prefs = in_tool_key_sequeces_preferences.keys;
                if (prefs.count(action)) {
                    if (prefs.at(action).size()) {
                        auto seq = prefs.at(action).front();
                        auto kl = Gtk::manage(new Gtk::Label(key_sequence_to_string_short(seq)));
                        kl->set_valign(Gtk::ALIGN_BASELINE);
                        key_box->pack_start(*kl, false, false, 0);
                    }
                }
            }
        }
        key_box->show_all();
        key_box->get_style_context()->add_class("imp-key-box");

        box->pack_start(*key_box, false, false, 0);
    }
    const auto &as = s.size() ? s : in_tool_action_catalog.at(action1).name;

    auto la = Gtk::manage(new Gtk::Label(as));
    la->set_valign(Gtk::ALIGN_BASELINE);

    la->show();

    box->pack_start(*la, false, false, 0);

    main_window->tool_bar_append_action(*box);
}

void ImpBase::tool_bar_clear_actions()
{
    in_tool_action_label_infos.clear();
    main_window->tool_bar_clear_actions();
}


void ImpBase::add_tool_button(ToolID id, const std::string &label, bool left)
{
    auto button = Gtk::manage(new Gtk::Button(label));
    button->signal_clicked().connect([this, id] { tool_begin(id); });
    button->show();
    core->signal_tool_changed().connect([button](ToolID t) { button->set_sensitive(t == ToolID::NONE); });
    if (left)
        main_window->header->pack_start(*button);
    else
        main_window->header->pack_end(*button);
}

void ImpBase::add_tool_action(ToolID tid, const std::string &action)
{
    auto tool_action = main_window->add_action(action, [this, tid] { tool_begin(tid); });
}

void ImpBase::add_tool_action(ActionID aid, const std::string &action)
{
    auto tool_action = main_window->add_action(action, [this, aid] { trigger_action(aid); });
}

void ImpBase::set_action_sensitive(ActionToolID action, bool v)
{
    action_sensitivity[action] = v;
    s_signal_action_sensitive.emit();
}

bool ImpBase::get_action_sensitive(ActionToolID action) const
{
    if (core->tool_is_active())
        return action_catalog.at(action).flags & ActionCatalogItem::FLAGS_IN_TOOL;
    if (action_sensitivity.count(action))
        return action_sensitivity.at(action);
    else
        return true;
}

void ImpBase::update_action_sensitivity()
{
    set_action_sensitive(make_action(ActionID::SAVE), !read_only && core->get_needs_save());
    set_action_sensitive(make_action(ActionID::UNDO), core->can_undo());
    set_action_sensitive(make_action(ActionID::REDO), core->can_redo());
    auto sel = canvas->get_selection();
    set_action_sensitive(make_action(ActionID::COPY), sel.size() > 0);
    bool can_select_polygon = std::any_of(sel.begin(), sel.end(), [](const auto &x) {
        switch (x.type) {
        case ObjectType::POLYGON_ARC_CENTER:
        case ObjectType::POLYGON_EDGE:
        case ObjectType::POLYGON_VERTEX:
            return true;

        default:
            return false;
        }
    });
    set_action_sensitive(make_action(ActionID::SELECT_POLYGON), can_select_polygon);
}


Gtk::Button *ImpBase::create_action_button(ActionToolID action)
{
    auto &catitem = action_catalog.at(action);
    auto button = Gtk::manage(new Gtk::Button(catitem.name));
    signal_action_sensitive().connect([this, button, action] { button->set_sensitive(get_action_sensitive(action)); });
    button->signal_clicked().connect([this, action] { trigger_action(action); });
    button->set_sensitive(get_action_sensitive(action));
    return button;
}

bool ImpBase::trigger_action(const ActionToolID &action)
{
    if (core->tool_is_active() && !(action_catalog.at(action).flags & ActionCatalogItem::FLAGS_IN_TOOL)) {
        return false;
    }
    auto conn = action_connections.at(action);
    conn.cb(conn);
    return true;
}

bool ImpBase::trigger_action(ActionID aid)
{
    return trigger_action({aid, ToolID::NONE});
}

bool ImpBase::trigger_action(ToolID tid)
{
    return trigger_action({ActionID::TOOL, tid});
}

void ImpBase::handle_tool_action(const ActionConnection &conn)
{
    assert(conn.action_id == ActionID::TOOL);
    tool_begin(conn.tool_id);
}


ActionConnection &ImpBase::connect_action(ActionID action_id, std::function<void(const ActionConnection &)> cb)
{
    return connect_action(action_id, ToolID::NONE, cb);
}

ActionConnection &ImpBase::connect_action(ToolID tool_id, std::function<void(const ActionConnection &)> cb)
{
    return connect_action(ActionID::TOOL, tool_id, cb);
}

ActionConnection &ImpBase::connect_action(ToolID tool_id)
{
    return connect_action(tool_id, sigc::mem_fun(*this, &ImpBase::handle_tool_action));
}

ActionConnection &ImpBase::connect_action(ActionID action_id, ToolID tool_id,
                                          std::function<void(const ActionConnection &)> cb)
{
    const auto key = std::make_pair(action_id, tool_id);
    if (action_connections.count(key)) {
        throw std::runtime_error("duplicate action");
    }
    if (action_catalog.count(key) == 0) {
        throw std::runtime_error("invalid action");
    }
    auto &act = action_connections
                        .emplace(std::piecewise_construct, std::forward_as_tuple(key),
                                 std::forward_as_tuple(action_id, tool_id, cb))
                        .first->second;

    return act;
}

void ImpBase::handle_pan_action(const ActionConnection &c)
{
    Coordf d;
    switch (c.action_id) {
    case ActionID::PAN_DOWN:
        d.y = -1;
        break;

    case ActionID::PAN_UP:
        d.y = 1;
        break;

    case ActionID::PAN_LEFT:
        d.x = 1;
        break;

    case ActionID::PAN_RIGHT:
        d.x = -1;
        break;
    default:
        return;
    }
    auto [sc, offset] = canvas->get_scale_and_offset();
    offset += d * 50;
    canvas->set_scale_and_offset(sc, offset);
}

void ImpBase::handle_zoom_action(const ActionConnection &c)
{
    auto factor = canvas->zoom_base;
    if (c.action_id == ActionID::ZOOM_OUT)
        factor = 1 / factor;

    if (preferences.zoom.keyboard_zoom_to_cursor)
        canvas->zoom_to_cursor(factor);
    else
        canvas->zoom_to_center(factor);
}

void ImpBase::force_end_tool()
{
    if (!core->tool_is_active())
        return;

    for (auto i = 0; i < 5; i++) {
        ToolArgs args;
        args.coords = canvas->get_cursor_pos();
        args.work_layer = canvas->property_work_layer();
        args.type = ToolEventType::ACTION;
        args.action = InToolActionID::CANCEL;
        ToolResponse r = core->tool_update(args);
        tool_process(r);
        if (!core->tool_is_active())
            return;
    }
    Logger::get().log_critical("Tool didn't end", Logger::Domain::IMP, "end the tool and repeat the last action");
}

} // namespace horizon
