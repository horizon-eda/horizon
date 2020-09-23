#include "imp.hpp"
#include "block/block.hpp"
#include "canvas/canvas_gl.hpp"
#include "export_gerber/gerber_export.hpp"
#include "logger/logger.hpp"
#include "pool/part.hpp"
#include "pool/pool_cached.hpp"
#include "property_panels/property_panels.hpp"
#include "rules/rules_window.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "widgets/log_view.hpp"
#include "widgets/log_window.hpp"
#include "widgets/spin_button_dim.hpp"
#include "action_catalog.hpp"
#include "tool_popover.hpp"
#include "util/selection_util.hpp"
#include "util/str_util.hpp"
#include "preferences/preferences_provider.hpp"
#include "parameter_window.hpp"
#include "widgets/about_dialog.hpp"
#include <glibmm/main.h>
#include <glibmm/markup.h>
#include <gtkmm.h>
#include <iomanip>
#include <functional>
#include "nlohmann/json.hpp"
#include "core/tool_id.hpp"
#include "imp/action.hpp"
#include "preferences/preferences_util.hpp"
#include "widgets/action_button.hpp"
#include "in_tool_action_catalog.hpp"

namespace horizon {

std::unique_ptr<Pool> make_pool(const PoolParams &p)
{
    if (p.cache_path.size() && Glib::file_test(p.cache_path, Glib::FILE_TEST_IS_DIR)) {
        return std::make_unique<PoolCached>(p.base_path, p.cache_path);
    }
    else {
        return std::make_unique<Pool>(p.base_path);
    }
}

ImpBase::ImpBase(const PoolParams &params)
    : pool(make_pool(params)), core(nullptr), sock_broadcast_rx(zctx, ZMQ_SUB), sock_project(zctx, ZMQ_REQ)
{
    auto ep_broadcast = Glib::getenv("HORIZON_EP_BROADCAST");
    if (ep_broadcast.size()) {
        sock_broadcast_rx.connect(ep_broadcast);
        {
            unsigned int prefix = 0;
            sock_broadcast_rx.setsockopt(ZMQ_SUBSCRIBE, &prefix, 4);
            prefix = getpid();
            sock_broadcast_rx.setsockopt(ZMQ_SUBSCRIBE, &prefix, 4);
        }
        Glib::RefPtr<Glib::IOChannel> chan;
#ifdef G_OS_WIN32
        SOCKET fd = sock_broadcast_rx.getsockopt<SOCKET>(ZMQ_FD);
        chan = Glib::IOChannel::create_from_win32_socket(fd);
#else
        int fd = sock_broadcast_rx.getsockopt<int>(ZMQ_FD);
        chan = Glib::IOChannel::create_from_fd(fd);
#endif

        {
            auto pid_p = Glib::getenv("HORIZON_MGR_PID");
            if (pid_p.size())
                mgr_pid = std::stoi(pid_p);
        }

        Glib::signal_io().connect(
                [this](Glib::IOCondition cond) {
                    while (sock_broadcast_rx.getsockopt<int>(ZMQ_EVENTS) & ZMQ_POLLIN) {
                        zmq::message_t msg;
                        sock_broadcast_rx.recv(&msg);
                        int prefix;
                        memcpy(&prefix, msg.data(), 4);
                        char *data = ((char *)msg.data()) + 4;
                        json j = json::parse(data);
                        if (prefix == 0 || prefix == getpid()) {
                            handle_broadcast(j);
                        }
                    }
                    return true;
                },
                chan, Glib::IO_IN | Glib::IO_HUP);
    }
    auto ep_project = Glib::getenv("HORIZON_EP_MGR");
    if (ep_project.size()) {
        sock_project.connect(ep_project);
        sock_project.setsockopt(ZMQ_RCVTIMEO, 5000);
        sock_project.setsockopt(ZMQ_SNDTIMEO, 5000);
    }
    sockets_connected = ep_project.size() && ep_broadcast.size();
}


void ImpBase::show_sockets_broken_dialog(const std::string &msg)
{
    Gtk::MessageDialog md(*main_window, "Lost connection to project/pool manager", false /* use_markup */,
                          Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
    md.set_secondary_text("Save your work an reopen the current document\n" + msg);
    md.run();
}

json ImpBase::send_json(const json &j)
{
    if (sockets_broken) {
        show_sockets_broken_dialog();
        return nullptr;
    }
    if (!sockets_connected)
        return nullptr;

    std::string s = j.dump();
    zmq::message_t msg(s.size() + 1);
    memcpy(((uint8_t *)msg.data()), s.c_str(), s.size());
    auto m = (char *)msg.data();
    m[msg.size() - 1] = 0;
    try {
        if (sock_project.send(msg) == false) {
            sockets_broken = true;
            sockets_connected = false;
            show_sockets_broken_dialog("send timeout");
            return nullptr;
        }
    }
    catch (zmq::error_t &e) {
        sockets_broken = true;
        sockets_connected = false;
        show_sockets_broken_dialog(e.what());
        return nullptr;
    }

    zmq::message_t rx;
    try {
        if (sock_project.recv(&rx) == false) {
            sockets_broken = true;
            sockets_connected = false;
            show_sockets_broken_dialog("receive timeout");
            return nullptr;
        }
    }
    catch (zmq::error_t &e) {
        sockets_broken = true;
        sockets_connected = false;
        show_sockets_broken_dialog(e.what());
        return nullptr;
    }
    char *rxdata = ((char *)rx.data());
    return json::parse(rxdata);
}

bool ImpBase::handle_close(GdkEventAny *ev)
{
    bool dontask = false;
    Glib::getenv("HORIZON_NOEXITCONFIRM", dontask);
    if (dontask) {
        core->delete_autosave();
        return false;
    }

    if (!core->get_needs_save()) {
        core->delete_autosave();
        return false;
    }
    if (!read_only) {
        Gtk::MessageDialog md(*main_window, "Save changes before closing?", false /* use_markup */,
                              Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
        md.set_secondary_text("If you don't save, all your changes will be permanently lost.");
        md.add_button("Close without Saving", 1);
        md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
        md.add_button("Save", 2);
        md.set_default_response(Gtk::RESPONSE_CANCEL);
        switch (md.run()) {
        case 1:
            core->delete_autosave();
            return false; // close

        case 2:
            force_end_tool();
            trigger_action(ActionID::SAVE);
            core->delete_autosave();
            return false; // close

        default:
            return true; // keep window open
        }
    }
    else {
        Gtk::MessageDialog md(*main_window, "Document is read only", false /* use_markup */, Gtk::MESSAGE_QUESTION,
                              Gtk::BUTTONS_NONE);
        md.set_secondary_text("You won't be able to save your changes");
        md.add_button("Close without Saving", 1);
        md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
        switch (md.run()) {
        case 1:
            core->delete_autosave();
            return false; // close

        default:
            return true; // keep window open
        }
    }

    core->delete_autosave();
    return false;
}

#undef GET_WIDGET
#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        main_window->builder->get_widget(#name, name);                                                                 \
    } while (0)

void ImpBase::update_selection_label()
{
    std::string la = "Selection (";
    switch (canvas->selection_tool) {
    case CanvasGL::SelectionTool::BOX:
        la += "Box";
        break;

    case CanvasGL::SelectionTool::LASSO:
        la += "Lasso";
        break;

    case CanvasGL::SelectionTool::PAINT:
        la += "Paint";
        break;
    }
    la += ", ";
    switch (canvas->selection_qualifier) {
    case CanvasGL::SelectionQualifier::INCLUDE_BOX:
        la += "include box";
        break;
    case CanvasGL::SelectionQualifier::INCLUDE_ORIGIN:
        la += "include origin";
        break;
    case CanvasGL::SelectionQualifier::TOUCH_BOX:
        la += "touch box";
        break;
    case CanvasGL::SelectionQualifier::AUTO:
        la += "auto";
        break;
    }
    la += ")";
    main_window->selection_label->set_text(la);
}

std::string ImpBase::get_tool_settings_filename(ToolID id)
{
    auto type_str = object_type_lut.lookup_reverse(get_editor_type());
    auto settings_dir = Glib::build_filename(get_config_dir(), "tool_settings", type_str);
    if (!Glib::file_test(settings_dir, Glib::FILE_TEST_IS_DIR))
        Gio::File::create_for_path(settings_dir)->make_directory_with_parents();
    return Glib::build_filename(settings_dir, tool_lut.lookup_reverse(id) + ".json");
}

bool ImpBase::property_panel_has_focus()
{
    auto focus_widget = main_window->get_focus();
    bool property_has_focus = false;
    if (focus_widget && (dynamic_cast<Gtk::Entry *>(focus_widget) || dynamic_cast<Gtk::TextView *>(focus_widget)))
        property_has_focus = focus_widget->is_ancestor(*main_window->property_viewport);
    return property_has_focus;
}

void ImpBase::set_flip_view(bool flip)
{
    canvas->set_flip_view(flip);
    canvas_update_from_pp();
    update_view_hints();
    apply_arrow_keys();
    g_simple_action_set_state(bottom_view_action->gobj(), g_variant_new_boolean(canvas->get_flip_view()));
}

void ImpBase::run(int argc, char *argv[])
{
    auto app = Gtk::Application::create(argc, argv, "org.horizon_eda.HorizonEDA.imp", Gio::APPLICATION_NON_UNIQUE);

    main_window = MainWindow::create();
    canvas = main_window->canvas;
    {
        Documents docs(core);
        clipboard.reset(new ClipboardManager(docs));
    }

    canvas->signal_selection_changed().connect(sigc::mem_fun(*this, &ImpBase::handle_selection_changed));
    canvas->signal_cursor_moved().connect(sigc::mem_fun(*this, &ImpBase::handle_cursor_move));
    canvas->signal_button_press_event().connect(sigc::mem_fun(*this, &ImpBase::handle_click));
    canvas->signal_button_press_event().connect(
            [this](GdkEventButton *ev) {
                if (property_panel_has_focus()
                    && (ev->button == 1 || ev->button == 3)) { // eat event so that things don't get deselected
                    canvas->grab_focus();
                    return true;
                }
                else {
                    return false;
                }
            },
            false);
    canvas->signal_button_release_event().connect(sigc::mem_fun(*this, &ImpBase::handle_click_release));
    canvas->signal_request_display_name().connect(sigc::mem_fun(*this, &ImpBase::get_complete_display_name));

    init_key();

    {
        selection_qualifiers = {
                {CanvasGL::SelectionTool::BOX, CanvasGL::SelectionQualifier::INCLUDE_ORIGIN},
                {CanvasGL::SelectionTool::LASSO, CanvasGL::SelectionQualifier::INCLUDE_ORIGIN},
                {CanvasGL::SelectionTool::PAINT, CanvasGL::SelectionQualifier::TOUCH_BOX},
        };

        Gtk::RadioButton *selection_tool_box_button, *selection_tool_lasso_button, *selection_tool_paint_button;
        Gtk::RadioButton *selection_qualifier_include_origin_button, *selection_qualifier_touch_box_button,
                *selection_qualifier_include_box_button, *selection_qualifier_auto_button;
        GET_WIDGET(selection_tool_box_button);
        GET_WIDGET(selection_tool_lasso_button);
        GET_WIDGET(selection_tool_paint_button);
        GET_WIDGET(selection_qualifier_include_origin_button);
        GET_WIDGET(selection_qualifier_touch_box_button);
        GET_WIDGET(selection_qualifier_include_box_button);
        GET_WIDGET(selection_qualifier_auto_button);

        Gtk::Box *selection_qualifier_box;
        GET_WIDGET(selection_qualifier_box);

        std::map<CanvasGL::SelectionQualifier, Gtk::RadioButton *> qual_map = {
                {CanvasGL::SelectionQualifier::INCLUDE_BOX, selection_qualifier_include_box_button},
                {CanvasGL::SelectionQualifier::TOUCH_BOX, selection_qualifier_touch_box_button},
                {CanvasGL::SelectionQualifier::INCLUDE_ORIGIN, selection_qualifier_include_origin_button},
                {CanvasGL::SelectionQualifier::AUTO, selection_qualifier_auto_button}};
        bind_widget<CanvasGL::SelectionQualifier>(qual_map, canvas->selection_qualifier, [this](auto v) {
            selection_qualifiers.at(canvas->selection_tool) = v;
            this->update_selection_label();
        });

        std::map<CanvasGL::SelectionTool, Gtk::RadioButton *> tool_map = {
                {CanvasGL::SelectionTool::BOX, selection_tool_box_button},
                {CanvasGL::SelectionTool::LASSO, selection_tool_lasso_button},
                {CanvasGL::SelectionTool::PAINT, selection_tool_paint_button},
        };
        bind_widget<CanvasGL::SelectionTool>(
                tool_map, canvas->selection_tool, [this, selection_qualifier_box, qual_map](auto v) {
                    this->update_selection_label();
                    selection_qualifier_box->set_sensitive(v != CanvasGL::SelectionTool::PAINT);
                    qual_map.at(CanvasGL::SelectionQualifier::AUTO)->set_sensitive(v != CanvasGL::SelectionTool::LASSO);

                    auto qual = selection_qualifiers.at(v);
                    qual_map.at(qual)->set_active(true);
                });

        connect_action(ActionID::SELECTION_TOOL_BOX,
                       [selection_tool_box_button](const auto &a) { selection_tool_box_button->set_active(true); });

        connect_action(ActionID::SELECTION_TOOL_LASSO,
                       [selection_tool_lasso_button](const auto &a) { selection_tool_lasso_button->set_active(true); });

        connect_action(ActionID::SELECTION_TOOL_PAINT,
                       [selection_tool_paint_button](const auto &a) { selection_tool_paint_button->set_active(true); });

        connect_action(ActionID::SELECTION_QUALIFIER_AUTO, [this, selection_qualifier_auto_button](const auto &a) {
            if (canvas->selection_tool == CanvasGL::SelectionTool::BOX)
                selection_qualifier_auto_button->set_active(true);
        });

        connect_action(ActionID::SELECTION_QUALIFIER_INCLUDE_BOX,
                       [this, selection_qualifier_include_box_button](const auto &a) {
                           if (canvas->selection_tool != CanvasGL::SelectionTool::PAINT)
                               selection_qualifier_include_box_button->set_active(true);
                       });

        connect_action(ActionID::SELECTION_QUALIFIER_INCLUDE_ORIGIN,
                       [this, selection_qualifier_include_origin_button](const auto &a) {
                           if (canvas->selection_tool != CanvasGL::SelectionTool::PAINT)
                               selection_qualifier_include_origin_button->set_active(true);
                       });

        connect_action(ActionID::SELECTION_QUALIFIER_TOUCH_BOX, [selection_qualifier_touch_box_button](const auto &a) {
            selection_qualifier_touch_box_button->set_active(true);
        });


        update_selection_label();
    }

    canvas->signal_selection_mode_changed().connect([this](auto mode) {
        set_action_sensitive(make_action(ActionID::CLICK_SELECT), mode == CanvasGL::SelectionMode::HOVER);
        if (mode == CanvasGL::SelectionMode::HOVER) {
            main_window->selection_mode_label->set_text("Hover select");
        }
        else {
            main_window->selection_mode_label->set_text("Click select");
        }
    });
    canvas->set_selection_mode(CanvasGL::SelectionMode::HOVER);

    connect_action(ActionID::CLICK_SELECT, [this](const auto &c) {
        canvas->set_selection({});
        canvas->set_selection_mode(CanvasGL::SelectionMode::NORMAL);
    });

    panels = Gtk::manage(new PropertyPanels(core));
    panels->show_all();
    panels->property_margin() = 10;
    main_window->property_viewport->add(*panels);
    panels->signal_update().connect([this] {
        canvas_update();
        canvas->set_selection(panels->get_selection(), false);
    });
    panels->signal_activate().connect([this] { canvas->grab_focus(); });
    panels->signal_throttled().connect(
            [this](bool thr) { main_window->property_throttled_revealer->set_reveal_child(thr); });

    warnings_box = Gtk::manage(new WarningsBox());
    warnings_box->signal_selected().connect(sigc::mem_fun(*this, &ImpBase::handle_warning_selected));
    main_window->left_panel->pack_end(*warnings_box, false, false, 0);

    selection_filter_dialog =
            std::make_unique<SelectionFilterDialog>(this->main_window, canvas->selection_filter, *this);
    selection_filter_dialog->signal_changed().connect(sigc::mem_fun(*this, &ImpBase::update_view_hints));

    distraction_free_action = main_window->add_action_bool("distraction_free", distraction_free);
    distraction_free_action->signal_change_state().connect([this](const Glib::VariantBase &v) {
        auto b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
        if (b != distraction_free) {
            trigger_action(ActionID::DISTRACTION_FREE);
        }
    });

    connect_action(ActionID::DISTRACTION_FREE, [this](const auto &a) {
        distraction_free = !distraction_free;
        g_simple_action_set_state(distraction_free_action->gobj(), g_variant_new_boolean(distraction_free));
        if (distraction_free) {
            left_panel_width = main_window->left_panel->get_allocated_width()
                               + main_window->left_panel->get_margin_start()
                               + main_window->left_panel->get_margin_end();
        }

        auto [scale, offset] = canvas->get_scale_and_offset();
        if (distraction_free)
            offset.x += left_panel_width;
        else
            offset.x -= left_panel_width;
        canvas->set_scale_and_offset(scale, offset);

        main_window->left_panel->set_visible(!distraction_free);
        bool show_properties = panels->get_selection().size() > 0;
        main_window->property_scrolled_window->set_visible(show_properties && !distraction_free);
        this->update_view_hints();
    });
    connect_action(ActionID::SELECTION_FILTER, [this](const auto &a) { selection_filter_dialog->present(); });
    connect_action(ActionID::SAVE, [this](const auto &a) {
        if (!read_only) {
            core->save();
            json j;
            this->get_save_meta(j);
            if (!j.is_null()) {
                save_json_to_file(core->get_filename() + meta_suffix, j);
            }
        }
    });
    connect_action(ActionID::UNDO, [this](const auto &a) {
        core->undo();
        this->canvas_update_from_pp();
        this->update_property_panels();
    });
    connect_action(ActionID::REDO, [this](const auto &a) {
        core->redo();
        this->canvas_update_from_pp();
        this->update_property_panels();
    });

    connect_action(ActionID::RELOAD_POOL, [this](const auto &a) {
        core->reload_pool();
        this->canvas_update_from_pp();
    });

    connect_action(ActionID::COPY,
                   [this](const auto &a) { clipboard->copy(canvas->get_selection(), canvas->get_cursor_pos()); });

    connect_action(ActionID::VIEW_ALL, [this](const auto &a) {
        auto bbox = canvas->get_bbox();
        canvas->zoom_to_bbox(bbox.first, bbox.second);
    });

    connect_action(ActionID::POPOVER, [this](const auto &a) {
        Gdk::Rectangle rect;
        auto c = canvas->get_cursor_pos_win();
        rect.set_x(c.x);
        rect.set_y(c.y);
        tool_popover->set_pointing_to(rect);

        this->update_action_sensitivity();
        std::map<ActionToolID, bool> can_begin;
        auto sel = canvas->get_selection();
        for (const auto &it : action_catalog) {
            if (it.first.first == ActionID::TOOL) {
                bool r = core->tool_can_begin(it.first.second, sel).first;
                can_begin[it.first] = r;
            }
            else {
                can_begin[it.first] = this->get_action_sensitive(it.first);
            }
        }
        tool_popover->set_can_begin(can_begin);

#if GTK_CHECK_VERSION(3, 22, 0)
        tool_popover->popup();
#else
        tool_popover->show();
#endif
    });

    connect_action(ActionID::PREFERENCES, [this](const auto &a) { show_preferences(); });

    init_action();

    connect_action(ActionID::VIEW_TOP, [this](const auto &a) { set_flip_view(false); });

    connect_action(ActionID::VIEW_BOTTOM, [this](const auto &a) { set_flip_view(true); });

    connect_action(ActionID::FLIP_VIEW, [this](const auto &a) { set_flip_view(!canvas->get_flip_view()); });

    bottom_view_action = main_window->add_action_bool("bottom_view", false);
    bottom_view_action->signal_change_state().connect([this](const Glib::VariantBase &v) {
        auto b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
        if (b) {
            trigger_action(ActionID::VIEW_BOTTOM);
        }
        else {
            trigger_action(ActionID::VIEW_TOP);
        }
    });


    canvas->signal_can_steal_focus().connect([this](bool &can_steal) {
        can_steal = !(main_window->search_entry->property_has_focus() || property_panel_has_focus());
    });

    init_search();

    grid_controller.emplace(*main_window, *canvas);
    connect_action(ActionID::SET_GRID_ORIGIN, [this](const auto &c) {
        auto co = canvas->get_cursor_pos();
        grid_controller->set_origin(co);
    });

    auto save_button = create_action_button(make_action(ActionID::SAVE));
    save_button->show();
    main_window->header->pack_start(*save_button);
    core->signal_needs_save().connect([this](bool v) {
        update_action_sensitivity();
        json j;
        j["op"] = "needs-save";
        j["pid"] = getpid();
        j["filename"] = core->get_filename();
        j["needs_save"] = core->get_needs_save();
        send_json(j);
    });
    set_action_sensitive(make_action(ActionID::SAVE), false);

    {
        auto undo_redo_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
        undo_redo_box->get_style_context()->add_class("linked");

        auto undo_button = create_action_button(make_action(ActionID::UNDO));
        gtk_button_set_label(undo_button->gobj(), NULL);
        undo_button->set_tooltip_text("Undo");
        undo_button->set_image_from_icon_name("edit-undo-symbolic", Gtk::ICON_SIZE_BUTTON);
        undo_redo_box->pack_start(*undo_button, false, false, 0);

        auto redo_button = create_action_button(make_action(ActionID::REDO));
        gtk_button_set_label(redo_button->gobj(), NULL);
        redo_button->set_tooltip_text("Redo");
        redo_button->set_image_from_icon_name("edit-redo-symbolic", Gtk::ICON_SIZE_BUTTON);
        undo_redo_box->pack_start(*redo_button, false, false, 0);

        undo_redo_box->show_all();
        main_window->header->pack_start(*undo_redo_box);
    }

    core->signal_can_undo_redo().connect([this] { update_action_sensitivity(); });
    canvas->signal_selection_changed().connect([this] {
        if (!core->tool_is_active()) {
            update_action_sensitivity();
        }
    });
    core->signal_can_undo_redo().emit();

    core->signal_load_tool_settings().connect([this](ToolID id) {
        json j;
        auto fn = get_tool_settings_filename(id);
        if (Glib::file_test(fn, Glib::FILE_TEST_IS_REGULAR)) {
            j = load_json_from_file(fn);
        }
        return j;
    });
    core->signal_save_tool_settings().connect(
            [this](ToolID id, json j) { save_json_to_file(get_tool_settings_filename(id), j); });

    if (core->get_rules()) {
        rules_window = RulesWindow::create(main_window, *canvas, *core);
        rules_window->signal_canvas_update().connect(sigc::mem_fun(*this, &ImpBase::canvas_update_from_pp));
        rules_window->signal_changed().connect([this] { core->set_needs_save(); });
        core->signal_tool_changed().connect([this](ToolID id) { rules_window->set_enabled(id == ToolID::NONE); });

        connect_action(ActionID::RULES, [this](const auto &conn) { rules_window->present(); });
        connect_action(ActionID::RULES_RUN_CHECKS, [this](const auto &conn) {
            rules_window->run_checks();
            rules_window->present();
        });
        connect_action(ActionID::RULES_APPLY, [this](const auto &conn) { rules_window->apply_rules(); });

        {
            auto button = create_action_button(make_action(ActionID::RULES));
            button->set_label("Rules...");
            main_window->header->pack_start(*button);
            button->show();
        }
    }

    tool_popover = Gtk::manage(new ToolPopover(canvas, get_editor_type_for_action()));
    tool_popover->set_position(Gtk::POS_BOTTOM);
    tool_popover->signal_action_activated().connect(
            [this](ActionID action_id, ToolID tool_id) { trigger_action(std::make_pair(action_id, tool_id)); });


    log_window = new LogWindow(main_window);
    Logger::get().set_log_handler([this](const Logger::Item &it) { log_window->get_view()->push_log(it); });

    main_window->add_action("view_log", [this] { log_window->present(); });

    main_window->signal_delete_event().connect(sigc::mem_fun(*this, &ImpBase::handle_close));

    for (const auto &la : core->get_layer_provider().get_layers()) {
        canvas->set_layer_display(la.first, LayerDisplay(true, LayerDisplay::Mode::FILL));
    }


    preferences.load();

    preferences_monitor = Gio::File::create_for_path(Preferences::get_preferences_filename())->monitor();

    preferences_monitor->signal_changed().connect([this](const Glib::RefPtr<Gio::File> &file,
                                                         const Glib::RefPtr<Gio::File> &file_other,
                                                         Gio::FileMonitorEvent ev) {
        if (ev == Gio::FILE_MONITOR_EVENT_CHANGES_DONE_HINT)
            preferences.load();
    });
    PreferencesProvider::get().set_prefs(preferences);

    add_hamburger_menu();

    view_options_menu = Gio::Menu::create();
    main_window->view_options_button->set_menu_model(view_options_menu);
    {
        Gdk::Rectangle rect;
        rect.set_width(24);
        main_window->view_options_button->get_popover()->set_pointing_to(rect);
    }

    view_options_menu->append("Distraction free mode", "win.distraction_free");
    add_tool_action(ActionID::SELECTION_FILTER, "selection_filter");
    view_options_menu->append("Selection filter", "win.selection_filter");

    imp_interface = std::make_unique<ImpInterface>(this);

    construct();

    {
        auto refBuilder = Gtk::Builder::create();
        refBuilder->add_from_resource("/org/horizon-eda/horizon/imp/app_menu.ui");

        auto object = refBuilder->get_object("appmenu");
        auto app_menu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(object);
        hamburger_menu->append_section(app_menu);
    }

    if (sockets_connected)
        main_window->add_action("preferences", [this] { trigger_action(ActionID::PREFERENCES); });

    main_window->add_action("about", [this] {
        AboutDialog dia;
        dia.set_transient_for(*main_window);
        dia.run();
    });

    main_window->add_action("help", [this] { trigger_action(ActionID::HELP); });

    preferences.signal_changed().connect(sigc::mem_fun(*this, &ImpBase::apply_preferences));

    apply_preferences();

    canvas->property_work_layer().signal_changed().connect([this] {
        if (core->tool_is_active()) {
            ToolArgs args;
            args.type = ToolEventType::LAYER_CHANGE;
            args.coords = canvas->get_cursor_pos();
            args.work_layer = canvas->property_work_layer();
            ToolResponse r = core->tool_update(args);
            tool_process(r);
        }
        selection_filter_dialog->set_work_layer(canvas->property_work_layer());
    });

    canvas->signal_grid_mul_changed().connect([this](unsigned int mul) {
        std::string s = "×";
        std::string n = std::to_string(mul);
        for (int i = 0; i < (3 - (int)n.size()); i++) {
            s += " ";
        }
        main_window->grid_mul_label->set_text(s + n);
    });

    context_menu = Gtk::manage(new Gtk::Menu());

    core->signal_tool_changed().connect([this](ToolID id) { s_signal_action_sensitive.emit(); });

    canvas->signal_hover_selection_changed().connect(sigc::mem_fun(*this, &ImpBase::hud_update));
    canvas->signal_selection_changed().connect(sigc::mem_fun(*this, &ImpBase::hud_update));

    canvas_update();

    initial_view_all_conn = canvas->signal_size_allocate().connect([this](const Gtk::Allocation &alloc) {
        auto bbox = canvas->get_bbox();
        canvas->zoom_to_bbox(bbox.first, bbox.second);
        initial_view_all_conn.disconnect();
    });

    handle_cursor_move(Coordi()); // fixes label

    Gtk::IconTheme::get_default()->add_resource_path("/org/horizon-eda/horizon/icons");

    Gtk::Window::set_default_icon_name("horizon-eda");

    Glib::set_prgname("horizon-eda"); // Fixes icons on wayland

    app->signal_startup().connect([this, app] {
        app->add_window(*main_window);
        app->add_action("quit", [this] { main_window->close(); });
    });

    auto cssp = Gtk::CssProvider::create();
    cssp->load_from_resource("/org/horizon-eda/horizon/global.css");
    Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), cssp, 700);

    canvas->signal_motion_notify_event().connect([this](GdkEventMotion *ev) {
        if (selection_for_drag_move.size()) {
            handle_drag(ev->state & Gdk::CONTROL_MASK);
        }
        return false;
    });

    canvas->signal_button_release_event().connect([this](GdkEventButton *ev) {
        selection_for_drag_move.clear();
        return false;
    });

    handle_selection_changed();

    core->signal_rebuilt().connect([this] { update_monitor(); });

    Glib::signal_timeout().connect_seconds(sigc::track_obj(
                                                   [this] {
                                                       if (core->tool_is_active()) {
                                                           queue_autosave = true;
                                                       }
                                                       else {
                                                           if (needs_autosave) {
                                                               core->autosave();
                                                               needs_autosave = false;
                                                           }
                                                       }
                                                       return true;
                                                   },
                                                   *main_window),
                                           60);
    core->signal_rebuilt().connect([this] {
        if (queue_autosave) {
            queue_autosave = false;
            core->autosave();
            needs_autosave = false;
        }
    });
    core->signal_needs_save().connect([this](bool v) {
        if (!v)
            needs_autosave = false;
    });
    core->signal_modified().connect([this] { needs_autosave = true; });

    if (core->get_block()) {
        core->signal_rebuilt().connect(sigc::mem_fun(*this, &ImpBase::set_window_title_from_block));
        set_window_title_from_block();
    }
    update_view_hints();

    connect_action(ActionID::SELECT_POLYGON, sigc::mem_fun(*this, &ImpBase::handle_select_polygon));

    {
        json j;
        j["op"] = "ready";
        j["pid"] = getpid();
        send_json(j);
    }

    app->run(*main_window);
}

void ImpBase::show_preferences(std::optional<std::string> page)
{
    json j;
    j["op"] = "preferences";
    if (page) {
        j["page"] = *page;
    }
    allow_set_foreground_window(mgr_pid);
    this->send_json(j);
}

void ImpBase::parameter_window_add_polygon_expand(ParameterWindow *parameter_window)
{
    auto button = Gtk::manage(new Gtk::Button("Polygon expand"));
    parameter_window->add_button(button);
    button->signal_clicked().connect([this, parameter_window] {
        auto sel = canvas->get_selection();
        if (sel.size() == 1) {
            auto &s = *sel.begin();
            if (s.type == ObjectType::POLYGON_EDGE || s.type == ObjectType::POLYGON_VERTEX) {
                auto poly = core->get_polygon(s.uuid);
                if (!poly->has_arcs()) {
                    std::stringstream ss;
                    ss.imbue(std::locale::classic());
                    ss << "expand-polygon [ " << poly->parameter_class << " ";
                    for (const auto &it : poly->vertices) {
                        ss << it.position.x << " " << it.position.y << " ";
                    }
                    ss << "]\n";
                    parameter_window->insert_text(ss.str());
                }
            }
        }
    });
}


void ImpBase::edit_pool_item(ObjectType type, const UUID &uu)
{
    json j;
    j["op"] = "edit";
    j["type"] = object_type_lut.lookup_reverse(type);
    j["uuid"] = static_cast<std::string>(uu);
    send_json(j);
}

ToolID ImpBase::get_tool_for_drag_move(bool ctrl, const std::set<SelectableRef> &sel) const
{
    if (ctrl)
        return ToolID::DUPLICATE;
    else
        return ToolID::MOVE;
}

void ImpBase::handle_drag(bool ctrl)
{
    auto pos = canvas->get_cursor_pos_win();
    auto delta = pos - cursor_pos_drag_begin;
    if (delta.mag_sq() > (50 * 50)) {
        {
            highlights.clear();
            update_highlights();
            ToolArgs args;
            args.coords = cursor_pos_grid_drag_begin;
            args.selection = selection_for_drag_move;
            ToolID tool_id = get_tool_for_drag_move(ctrl, selection_for_drag_move);

            ToolResponse r = core->tool_begin(tool_id, args, imp_interface.get(), true);
            tool_process(r);
        }
        selection_for_drag_move.clear();
    }
}

void ImpBase::apply_preferences()
{
    auto canvas_prefs = get_canvas_preferences();
    canvas->set_appearance(canvas_prefs->appearance);
    if (core->tool_is_active()) {
        canvas->set_cursor_size(canvas_prefs->appearance.cursor_size_tool);
    }
    canvas->show_all_junctions_in_schematic = preferences.schematic.show_all_junctions;

    auto av = get_editor_type_for_action();
    for (auto &it : action_connections) {
        auto act = action_catalog.at(it.first);
        if (!(act.flags & ActionCatalogItem::FLAGS_NO_PREFERENCES) && preferences.key_sequences.keys.count(it.first)) {
            auto pref = preferences.key_sequences.keys.at(it.first);
            std::vector<KeySequence> *seqs = nullptr;
            if (pref.count(av) && pref.at(av).size()) {
                seqs = &pref.at(av);
            }
            else if (pref.count(ActionCatalogItem::AVAILABLE_EVERYWHERE)
                     && pref.at(ActionCatalogItem::AVAILABLE_EVERYWHERE).size()) {
                seqs = &pref.at(ActionCatalogItem::AVAILABLE_EVERYWHERE);
            }
            if (seqs) {
                it.second.key_sequences = *seqs;
            }
            else {
                it.second.key_sequences.clear();
            }
        }
    }
    in_tool_key_sequeces_preferences = preferences.in_tool_key_sequences;
    in_tool_key_sequeces_preferences.keys.erase(InToolActionID::LMB);
    in_tool_key_sequeces_preferences.keys.erase(InToolActionID::RMB);
    in_tool_key_sequeces_preferences.keys.erase(InToolActionID::LMB_RELEASE);
    apply_arrow_keys();

    {
        const auto mod0 = static_cast<GdkModifierType>(0);

        in_tool_key_sequeces_preferences.keys[InToolActionID::CANCEL] = {{{GDK_KEY_Escape, mod0}}};
        in_tool_key_sequeces_preferences.keys[InToolActionID::COMMIT] = {{{GDK_KEY_Return, mod0}},
                                                                         {{GDK_KEY_KP_Enter, mod0}}};
    }

    key_sequence_dialog->clear();
    for (const auto &it : action_connections) {
        if (it.second.key_sequences.size()) {
            key_sequence_dialog->add_sequence(it.second.key_sequences, action_catalog.at(it.first).name);
            tool_popover->set_key_sequences(it.first, it.second.key_sequences);
        }
    }
    preferences_apply_to_canvas(canvas, preferences);
    for (auto it : action_buttons) {
        it->update_key_sequences();
        it->set_keep_primary_action(!preferences.action_bar.remember);
    }
    main_window->set_use_action_bar(preferences.action_bar.enable);
}

void ImpBase::canvas_update_from_pp()
{
    if (core->tool_is_active())
        return;

    auto sel = canvas->get_selection();
    canvas_update();
    canvas->set_selection(sel, false);
    update_highlights();
}

void ImpBase::tool_begin(ToolID id, bool override_selection, const std::set<SelectableRef> &sel,
                         std::unique_ptr<ToolData> data)
{
    if (core->tool_is_active()) {
        Logger::log_critical("can't begin tool while tool is active", Logger::Domain::IMP);
        return;
    }
    highlights.clear();
    update_highlights();
    ToolArgs args;
    args.data = std::move(data);
    args.coords = canvas->get_cursor_pos();

    if (override_selection)
        args.selection = sel;
    else
        args.selection = canvas->get_selection();

    args.work_layer = canvas->property_work_layer();
    ToolResponse r = core->tool_begin(id, args, imp_interface.get());
    tool_process(r);
}

void ImpBase::add_hamburger_menu()
{
    auto hamburger_button = Gtk::manage(new Gtk::MenuButton);
    hamburger_button->set_image_from_icon_name("open-menu-symbolic", Gtk::ICON_SIZE_BUTTON);
    core->signal_tool_changed().connect(
            [hamburger_button](ToolID t) { hamburger_button->set_sensitive(t == ToolID::NONE); });

    hamburger_menu = Gio::Menu::create();
    hamburger_button->set_menu_model(hamburger_menu);
    hamburger_button->show();
    main_window->header->pack_end(*hamburger_button);
}

void ImpBase::layer_up_down(bool up)
{
    int wl = canvas->property_work_layer();
    auto layers = core->get_layer_provider().get_layers();
    std::vector<int> layer_indexes;
    layer_indexes.reserve(layers.size());
    std::transform(layers.begin(), layers.end(), std::back_inserter(layer_indexes),
                   [](const auto &x) { return x.first; });

    int idx = std::find(layer_indexes.begin(), layer_indexes.end(), wl) - layer_indexes.begin();
    if (up) {
        idx++;
    }
    else {
        idx--;
    }
    if (idx >= 0 && idx < (int)layers.size()) {
        canvas->property_work_layer() = layer_indexes.at(idx);
    }
}

void ImpBase::goto_layer(int layer)
{
    if (core->get_layer_provider().get_layers().count(layer)) {
        canvas->property_work_layer() = layer;
    }
}

void ImpBase::handle_cursor_move(const Coordi &pos)
{
    if (core->tool_is_active()) {
        ToolArgs args;
        args.type = ToolEventType::MOVE;
        args.coords = pos;
        args.work_layer = canvas->property_work_layer();
        ToolResponse r = core->tool_update(args);
        tool_process(r);
    }
    main_window->cursor_label->set_text(coord_to_string(pos));
}

void ImpBase::fix_cursor_pos()
{
    auto ev = gtk_get_current_event();
    auto dev = gdk_event_get_device(ev);
    auto win = canvas->get_window()->gobj();
    gdouble x, y;
    gdk_window_get_device_position_double(win, dev, &x, &y, nullptr);
    if (!canvas->get_has_window()) {
        auto alloc = canvas->get_allocation();
        x -= alloc.get_x();
        y -= alloc.get_y();
    }
    canvas->update_cursor_pos(x, y);
}

bool ImpBase::handle_click_release(GdkEventButton *button_event)
{
    if (core->tool_is_active() && button_event->button != 2 && !(button_event->state & Gdk::SHIFT_MASK)) {
        ToolArgs args;
        args.type = ToolEventType::ACTION;
        args.action = InToolActionID::LMB_RELEASE;
        args.coords = canvas->get_cursor_pos();
        args.target = canvas->get_current_target();
        args.work_layer = canvas->property_work_layer();
        ToolResponse r = core->tool_update(args);
        tool_process(r);
    }
    return false;
}

Gtk::MenuItem *ImpBase::create_context_menu_item(ActionToolID act)
{
    auto it = Gtk::manage(new Gtk::MenuItem);
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));

    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_xalign(0);
        la->set_label(action_catalog.at(act).name);
        box->pack_start(*la, false, false, 0);
    }

    {
        auto la = Gtk::manage(new Gtk::Label);
        la->set_xalign(1);
        la->get_style_context()->add_class("dim-label");
        la->set_label(key_sequences_to_string(action_connections.at(act).key_sequences));
        la->set_margin_end(4);
        box->pack_start(*la, true, true, 0);
    }

    box->show_all();
    it->add(*box);
    return it;
}

void ImpBase::create_context_menu(Gtk::Menu *parent, const std::set<SelectableRef> &sel)
{
    update_action_sensitivity();
    Gtk::SeparatorMenuItem *sep = nullptr;
    for (const auto &it_gr : action_group_catalog) {
        std::vector<Gtk::MenuItem *> menu_items;
        for (const auto &it : action_catalog) {
            if (it.second.group == it_gr.first && (it.second.availability & get_editor_type_for_action())
                && !(it.second.flags & ActionCatalogItem::FLAGS_NO_MENU)) {
                if (it.first.first == ActionID::TOOL) {
                    auto r = core->tool_can_begin(it.first.second, {sel});
                    if (r.first && r.second) {
                        auto la_sub = create_context_menu_item(it.first);
                        ToolID tool_id = it.first.second;
                        std::set<SelectableRef> sr(sel);
                        la_sub->signal_activate().connect([this, tool_id, sr] {
                            canvas->set_selection(sr, false);
                            fix_cursor_pos();
                            tool_begin(tool_id);
                        });
                        la_sub->show();
                        menu_items.push_back(la_sub);
                    }
                }
                else {
                    if (get_action_sensitive(it.first) && (it.second.flags & ActionCatalogItem::FLAGS_SPECIFIC)) {
                        auto la_sub = create_context_menu_item(it.first);
                        ActionID action_id = it.first.first;
                        std::set<SelectableRef> sr(sel);
                        la_sub->signal_activate().connect([this, action_id, sr] {
                            canvas->set_selection(sr, false);
                            fix_cursor_pos();
                            trigger_action(make_action(action_id));
                        });
                        la_sub->show();
                        menu_items.push_back(la_sub);
                    }
                }
            }
        }
        if (menu_items.size() > 6) {
            auto submenu = Gtk::manage(new Gtk::Menu);
            submenu->show();
            for (auto it : menu_items) {
                submenu->append(*it);
            }
            auto la_sub = Gtk::manage(new Gtk::MenuItem(it_gr.second));
            la_sub->show();
            la_sub->set_submenu(*submenu);
            parent->append(*la_sub);
        }
        else {
            for (auto it : menu_items) {
                parent->append(*it);
            }
        }
        if (menu_items.size()) {
            sep = Gtk::manage(new Gtk::SeparatorMenuItem);
            sep->show();
            parent->append(*sep);
        }
    }
    if (sep)
        delete sep;
}

bool ImpBase::handle_click(GdkEventButton *button_event)
{
    if (button_event->button != 2)
        set_search_mode(false);

    bool need_menu = false;
    if (core->tool_is_active() && button_event->button != 2 && !(button_event->state & Gdk::SHIFT_MASK)
        && button_event->type != GDK_2BUTTON_PRESS && button_event->type != GDK_3BUTTON_PRESS) {
        ToolArgs args;
        args.type = ToolEventType::ACTION;
        if (button_event->button == 1)
            args.action = InToolActionID::LMB;
        else
            args.action = InToolActionID::RMB;
        args.coords = canvas->get_cursor_pos();
        args.target = canvas->get_current_target();
        args.work_layer = canvas->property_work_layer();
        ToolResponse r = core->tool_update(args);
        tool_process(r);
    }
    else if (!core->tool_is_active() && button_event->type == GDK_2BUTTON_PRESS) {
        auto sel = canvas->get_selection();
        if (sel.size() == 1) {
            auto a = get_doubleclick_action(sel.begin()->type, sel.begin()->uuid);
            if (a.first != ActionID::NONE) {
                selection_for_drag_move.clear();
                trigger_action(a);
            }
        }
    }
    else if (!core->tool_is_active() && button_event->button == 1 && !(button_event->state & Gdk::SHIFT_MASK)) {
        handle_maybe_drag();
    }
    else if (!core->tool_is_active() && button_event->button == 3) {
        for (const auto it : context_menu->get_children()) {
            delete it;
        }
        std::set<SelectableRef> sel_for_menu;
        if (canvas->get_selection_mode() == CanvasGL::SelectionMode::HOVER) {
            sel_for_menu = canvas->get_selection();
        }
        else {
            auto c = canvas->screen2canvas(Coordf(button_event->x, button_event->y));
            auto sel = canvas->get_selection_at(Coordi(c.x, c.y));
            auto sel_from_canvas = canvas->get_selection();
            std::set<SelectableRef> isect;
            std::set_intersection(sel.begin(), sel.end(), sel_from_canvas.begin(), sel_from_canvas.end(),
                                  std::inserter(isect, isect.begin()));
            if (isect.size()) { // was in selection
                sel_for_menu = sel_from_canvas;
            }
            else if (sel.size() == 1) { // there's exactly one item
                canvas->set_selection(sel, false);
                sel_for_menu = sel;
            }
            else if (sel.size() > 1) { // multiple items: do our own menu
                canvas->set_selection({}, false);
                for (const auto &sr : sel) {
                    std::string text = get_complete_display_name(sr);
                    auto la = Gtk::manage(new Gtk::MenuItem(text));

                    la->signal_select().connect([this, sr] {
                        canvas->set_selection({sr}, false);
#ifdef G_OS_WIN32 // work around a bug(?) in intel(?) GPU drivers on windows
                        Glib::signal_idle().connect_once([this] { canvas->queue_draw(); });
#endif
                    });

                    la->signal_deselect().connect([this] {
                        canvas->set_selection({}, false);
#ifdef G_OS_WIN32 // work around a bug(?) in intel(?) GPU drivers on windows
                        Glib::signal_idle().connect_once([this] { canvas->queue_draw(); });
#endif
                    });

                    auto submenu = Gtk::manage(new Gtk::Menu);

                    create_context_menu(submenu, {sr});
                    la->set_submenu(*submenu);
                    la->show();
                    context_menu->append(*la);
                }
                need_menu = true;
                sel_for_menu.clear();
            }
        }

        if (sel_for_menu.size()) {
            create_context_menu(context_menu, sel_for_menu);
            need_menu = true;
        }
    }

    if (need_menu) {
#if GTK_CHECK_VERSION(3, 22, 0)
        context_menu->popup_at_pointer((GdkEvent *)button_event);
#else
        context_menu->popup(0, gtk_get_current_event_time());
#endif
    }
    return false;
}

std::string ImpBase::get_complete_display_name(const SelectableRef &sr)
{
    std::string text = object_descriptions.at(sr.type).name;
    auto display_name = core->get_display_name(sr.type, sr.uuid);
    if (display_name.size()) {
        text += " " + display_name;
    }
    auto layers = core->get_layer_provider().get_layers();
    if (layers.count(sr.layer.start()) && layers.count(sr.layer.end())) {
        if (sr.layer.is_multilayer())
            text += " (" + layers.at(sr.layer.start()).name + +" - " + layers.at(sr.layer.end()).name + ")";
        else
            text += " (" + layers.at(sr.layer.start()).name + ")";
    }
    return text;
}


void ImpBase::handle_maybe_drag()
{
    auto c = canvas->screen2canvas(canvas->get_cursor_pos_win());
    auto sel_at_cursor = canvas->get_selection_at(Coordi(c.x, c.y));
    auto sel_from_canvas = canvas->get_selection();
    std::set<SelectableRef> isect;
    std::set_intersection(sel_from_canvas.begin(), sel_from_canvas.end(), sel_at_cursor.begin(), sel_at_cursor.end(),
                          std::inserter(isect, isect.begin()));
    if (isect.size()) {
        canvas->inhibit_drag_selection();
        cursor_pos_drag_begin = canvas->get_cursor_pos_win();
        cursor_pos_grid_drag_begin = canvas->get_cursor_pos();
        selection_for_drag_move = sel_from_canvas;
    }
}

void ImpBase::tool_process(ToolResponse &resp)
{
    if (!core->tool_is_active()) {
        imp_interface->dialogs.close_nonmodal();
        main_window->tool_hint_label->set_text(">");
        canvas->set_cursor_external(false);
        canvas->snap_filter.clear();
        no_update = false;
        highlights.clear();
        update_highlights();
    }
    if (!no_update) {
        canvas_update();
        if (core->tool_is_active()) {
            canvas->set_selection(core->get_tool_selection());
        }
        else {
            if (canvas->get_selection_mode() == CanvasGL::SelectionMode::NORMAL) {
                canvas->set_selection(core->get_tool_selection());
            }
            else {
                canvas->set_selection({});
            }
        }
    }
    if (resp.next_tool != ToolID::NONE) {
        highlights.clear();
        update_highlights();
        ToolArgs args;
        args.coords = canvas->get_cursor_pos();
        args.keep_selection = true;
        args.data = std::move(resp.data);
        ToolResponse r = core->tool_begin(resp.next_tool, args, imp_interface.get());
        tool_process(r);
    }
}

void ImpBase::expand_selection_for_property_panel(std::set<SelectableRef> &sel_extra,
                                                  const std::set<SelectableRef> &sel)
{
}

void ImpBase::handle_selection_changed(void)
{
    // std::cout << "Selection changed\n";
    // std::cout << "---" << std::endl;
    if (!core->tool_is_active()) {
        highlights.clear();
        update_highlights();
        update_property_panels();
    }
}

void ImpBase::update_property_panels()
{
    auto sel = canvas->get_selection();
    decltype(sel) sel_extra;
    for (const auto &it : sel) {
        switch (it.type) {
        case ObjectType::POLYGON_EDGE:
        case ObjectType::POLYGON_VERTEX: {
            sel_extra.emplace(it.uuid, ObjectType::POLYGON);
            auto poly = core->get_polygon(it.uuid);
            if (poly->usage && poly->usage->get_type() == PolygonUsage::Type::PLANE) {
                sel_extra.emplace(poly->usage->get_uuid(), ObjectType::PLANE);
            }
            if (poly->usage && poly->usage->get_type() == PolygonUsage::Type::KEEPOUT) {
                sel_extra.emplace(poly->usage->get_uuid(), ObjectType::KEEPOUT);
            }
        } break;
        default:;
        }
    }
    expand_selection_for_property_panel(sel_extra, sel);

    sel.insert(sel_extra.begin(), sel_extra.end());
    panels->update_objects(sel);
    bool show_properties = panels->get_selection().size() > 0;
    main_window->property_scrolled_window->set_visible(show_properties && !distraction_free);
}

void ImpBase::handle_tool_change(ToolID id)
{
    panels->set_sensitive(id == ToolID::NONE);
    canvas->set_selection_allowed(id == ToolID::NONE);
    main_window->tool_bar_set_use_actions(core->get_tool_actions().size());
    if (id != ToolID::NONE) {
        main_window->tool_bar_set_tool_name(action_catalog.at({ActionID::TOOL, id}).name);
        main_window->tool_bar_set_tool_tip("");
        main_window->hud_hide();
        canvas->set_cursor_size(get_canvas_preferences()->appearance.cursor_size_tool);
    }
    else {
        canvas->set_cursor_size(get_canvas_preferences()->appearance.cursor_size);
    }
    main_window->tool_bar_set_visible(id != ToolID::NONE);
    tool_bar_clear_actions();
    main_window->action_bar_revealer->set_reveal_child(id == ToolID::NONE);
}

void ImpBase::handle_warning_selected(const Coordi &pos)
{
    canvas->center_and_zoom(pos);
}

bool ImpBase::handle_broadcast(const json &j)
{
    const std::string op = j.at("op");
    guint32 timestamp = j.value("time", 0);
    if (op == "present") {
        main_window->present(timestamp);
        return true;
    }
    else if (op == "save") {
        force_end_tool();
        trigger_action(ActionID::SAVE);
        return true;
    }
    else if (op == "close") {
        core->delete_autosave();
        delete main_window;
        return true;
    }
    else if (op == "preferences") {
        const auto &prefs = j.at("preferences");
        preferences.load_from_json(prefs);
        preferences.signal_changed().emit();
        return true;
    }
    return false;
}

void ImpBase::set_monitor_files(const std::set<std::string> &files)
{
    for (const auto &filename : files) {
        if (file_monitors.count(filename) == 0) {
            auto mon = Gio::File::create_for_path(filename)->monitor_file();
            mon->signal_changed().connect(sigc::mem_fun(*this, &ImpBase::handle_file_changed));
            file_monitors[filename] = mon;
        }
    }
    map_erase_if(file_monitors, [files](auto &it) { return files.count(it.first) == 0; });
}

void ImpBase::set_monitor_items(const ItemSet &items)
{
    std::set<std::string> filenames;
    std::transform(items.begin(), items.end(), std::inserter(filenames, filenames.begin()),
                   [this](const auto &it) { return pool->get_filename(it.first, it.second); });
    set_monitor_files(filenames);
}

void ImpBase::handle_file_changed(const Glib::RefPtr<Gio::File> &file1, const Glib::RefPtr<Gio::File> &file2,
                                  Gio::FileMonitorEvent ev)
{
    main_window->show_nonmodal(
            "Pool has changed", "Reload pool", [this] { trigger_action(ActionID::RELOAD_POOL); },
            "This will clear the undo/redo history");
}

void ImpBase::set_read_only(bool v)
{
    read_only = v;
}

void ImpBase::tool_update_data(std::unique_ptr<ToolData> &data)
{

    if (core->tool_is_active()) {
        ToolArgs args;
        args.type = ToolEventType::DATA;
        args.data = std::move(data);
        args.coords = canvas->get_cursor_pos();
        ToolResponse r = core->tool_update(args);
        tool_process(r);
    }
}


ActionToolID ImpBase::get_doubleclick_action(ObjectType type, const UUID &uu)
{
    switch (type) {
    case ObjectType::TEXT:
        return make_action(ToolID::ENTER_DATUM);
        break;
    case ObjectType::POLYGON_ARC_CENTER:
    case ObjectType::POLYGON_VERTEX:
    case ObjectType::POLYGON_EDGE:
        return make_action(ActionID::SELECT_POLYGON);
        break;
    default:
        return {ActionID::NONE, ToolID::NONE};
    }
}

std::map<ObjectType, ImpBase::SelectionFilterInfo> ImpBase::get_selection_filter_info() const
{
    std::map<ObjectType, ImpBase::SelectionFilterInfo> r;
    for (const auto &it : object_descriptions) {
        ObjectType type = it.first;
        if (type == ObjectType::POLYGON_ARC_CENTER || type == ObjectType::POLYGON_EDGE
            || type == ObjectType::POLYGON_VERTEX)
            type = ObjectType::POLYGON;
        if (core->has_object_type(type)) {
            r[type];
        }
    }
    return r;
}

const std::string ImpBase::meta_suffix = ".imp_meta";

void ImpBase::load_meta()
{
    std::string meta_filename = core->get_filename() + meta_suffix;
    if (Glib::file_test(meta_filename, Glib::FILE_TEST_IS_REGULAR)) {
        m_meta = load_json_from_file(meta_filename);
    }
    else {
        m_meta = core->get_meta();
    }
}

void ImpBase::set_window_title(const std::string &s)
{
    if (s.size()) {
        main_window->set_title(s + " - " + object_descriptions.at(get_editor_type()).name);
    }
    else {
        main_window->set_title("Untitled " + object_descriptions.at(get_editor_type()).name);
    }
}


void ImpBase::set_window_title_from_block()
{
    std::string title;
    if (core->get_block()->project_meta.count("project_title"))
        title = core->get_block()->project_meta.at("project_title");

    set_window_title(title);
}

std::vector<std::string> ImpBase::get_view_hints()
{
    std::vector<std::string> r;
    if (distraction_free)
        r.emplace_back("distraction free mode");

    if (canvas->get_flip_view())
        r.emplace_back("bottom view");

    if (selection_filter_dialog->get_filtered())
        r.emplace_back("selection filtered");
    return r;
}

void ImpBase::update_view_hints()
{
    auto hints = get_view_hints();
    main_window->set_view_hints_label(hints);
}

static bool is_poly(ObjectType type)
{
    switch (type) {
    case ObjectType::POLYGON_ARC_CENTER:
    case ObjectType::POLYGON_EDGE:
    case ObjectType::POLYGON_VERTEX:
        return true;
    default:
        return false;
    }
}

void ImpBase::handle_select_polygon(const ActionConnection &a)
{
    auto sel = canvas->get_selection();
    auto new_sel = sel;
    for (const auto &it : sel) {
        if (is_poly(it.type)) {
            auto poly = core->get_polygon(it.uuid);
            unsigned int v = 0;
            for (const auto &it_v : poly->vertices) {
                new_sel.emplace(it.uuid, ObjectType::POLYGON_VERTEX, v);
                new_sel.emplace(it.uuid, ObjectType::POLYGON_EDGE, v);
                if (it_v.type == Polygon::Vertex::Type::ARC)
                    new_sel.emplace(it.uuid, ObjectType::POLYGON_ARC_CENTER, v);
                v++;
            }
        }
    }
    canvas->set_selection(new_sel, false);
    canvas->set_selection_mode(CanvasGL::SelectionMode::NORMAL);
}

ObjectType ImpBase::get_editor_type() const
{
    return core->get_object_type();
}

} // namespace horizon
