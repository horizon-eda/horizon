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
#include "hud_util.hpp"
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
        sock_project.setsockopt(ZMQ_RCVTIMEO, 2000);
        sock_project.setsockopt(ZMQ_SNDTIMEO, 2000);
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
    if (dontask)
        return false;

    if (!core->get_needs_save()) {
        return false;
    }
    if (!read_only) {
        Gtk::MessageDialog md(*main_window, "Save changes before closing?", false /* use_markup */,
                              Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
        md.set_secondary_text("If you don't save, all your changes will be permanently lost.");
        md.add_button("Close without Saving", 1);
        md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
        md.add_button("Save", 2);
        switch (md.run()) {
        case 1:
            core->delete_autosave();
            return false; // close

        case 2:
            trigger_action(ActionID::SAVE);
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
            return false; // close

        default:
            return true; // keep window open
        }
    }
    return false;
}

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
    canvas->signal_key_press_event().connect(sigc::mem_fun(*this, &ImpBase::handle_key_press));
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
    canvas->signal_request_display_name().connect(
            [this](ObjectType ty, UUID uu) { return core->get_display_name(ty, uu); });

    {
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
        bind_widget<CanvasGL::SelectionQualifier>(qual_map, canvas->selection_qualifier,
                                                  [this](auto v) { this->update_selection_label(); });

        std::map<CanvasGL::SelectionTool, Gtk::RadioButton *> tool_map = {
                {CanvasGL::SelectionTool::BOX, selection_tool_box_button},
                {CanvasGL::SelectionTool::LASSO, selection_tool_lasso_button},
                {CanvasGL::SelectionTool::PAINT, selection_tool_paint_button},
        };
        bind_widget<CanvasGL::SelectionTool>(tool_map, canvas->selection_tool,
                                             [this, selection_qualifier_touch_box_button, selection_qualifier_box,
                                              selection_qualifier_auto_button,
                                              selection_qualifier_include_origin_button](auto v) {
                                                 this->update_selection_label();
                                                 auto is_paint = (v == CanvasGL::SelectionTool::PAINT);
                                                 if (is_paint) {
                                                     selection_qualifier_touch_box_button->set_active(true);
                                                 }
                                                 selection_qualifier_box->set_sensitive(!is_paint);

                                                 auto is_box = (v == CanvasGL::SelectionTool::BOX);
                                                 selection_qualifier_auto_button->set_sensitive(is_box);
                                                 if (is_box)
                                                     selection_qualifier_auto_button->set_active(true);

                                                 if (v == CanvasGL::SelectionTool::LASSO) {
                                                     selection_qualifier_include_origin_button->set_active(true);
                                                 }
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
        if (mode == CanvasGL::SelectionMode::HOVER) {
            main_window->selection_mode_label->set_text("Hover select");
        }
        else {
            main_window->selection_mode_label->set_text("Click select");
        }
    });
    canvas->set_selection_mode(CanvasGL::SelectionMode::HOVER);

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

    key_sequence_dialog = std::make_unique<KeySequenceDialog>(this->main_window);

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
    });
    connect_action(ActionID::REDO, [this](const auto &a) {
        core->redo();
        this->canvas_update_from_pp();
    });

    connect_action(ActionID::RELOAD_POOL, [this](const auto &a) {
        core->reload_pool();
        this->canvas_update_from_pp();
    });

    connect_action(ActionID::COPY,
                   [this](const auto &a) { clipboard->copy(canvas->get_selection(), canvas->get_cursor_pos()); });

    connect_action(ActionID::HELP, [this](const auto &a) { key_sequence_dialog->show(); });

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
        std::map<std::pair<ActionID, ToolID>, bool> can_begin;
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

    connect_action(ActionID::PREFERENCES, [this](const auto &a) {
        json j;
        j["op"] = "preferences";
        allow_set_foreground_window(mgr_pid);
        this->send_json(j);
    });

    for (const auto &it : action_catalog) {
        if ((it.first.first == ActionID::TOOL) && (it.second.availability & get_editor_type_for_action())) {
            connect_action(it.first.second);
        }
    }

    connect_action(ActionID::VIEW_TOP, [this](const auto &a) {
        canvas->set_flip_view(false);
        this->canvas_update_from_pp();
        this->update_view_hints();
    });

    connect_action(ActionID::VIEW_BOTTOM, [this](const auto &a) {
        canvas->set_flip_view(true);
        this->canvas_update_from_pp();
        this->update_view_hints();
    });

    connect_action(ActionID::FLIP_VIEW, [this](const auto &a) {
        canvas->set_flip_view(!canvas->get_flip_view());
        this->canvas_update_from_pp();
        this->update_view_hints();
    });

    connect_action(ActionID::SEARCH, [this](const auto &a) { this->set_search_mode(true); });
    connect_action(ActionID::SEARCH_NEXT, [this](const auto &a) {
        this->set_search_mode(true, false);
        this->search_go(1);
    });
    connect_action(ActionID::SEARCH_PREVIOUS, [this](const auto &a) {
        this->set_search_mode(true, false);
        this->search_go(-1);
    });
    connect_action(ActionID::SELECT_ALL, [this](const auto &a) {
        canvas->set_selection_mode(CanvasGL::SelectionMode::NORMAL);
        canvas->select_all();
    });

    canvas->signal_can_steal_focus().connect([this](bool &can_steal) {
        can_steal = !(main_window->search_entry->property_has_focus() || property_panel_has_focus());
    });

    main_window->search_entry->signal_changed().connect([this] {
        this->handle_search();
        this->search_go(0);
    });
    main_window->search_entry->signal_key_press_event().connect(
            [this](GdkEventKey *ev) {
                if (ev->keyval == GDK_KEY_Escape) {
                    set_search_mode(false);
                    return true;
                }
                else if (ev->keyval == GDK_KEY_Return && (ev->state & Gdk::SHIFT_MASK)) {
                    search_go(-1);
                    return true;
                }
                return false;
            },
            false);
    main_window->search_next_button->signal_clicked().connect([this] { trigger_action(ActionID::SEARCH_NEXT); });
    main_window->search_previous_button->signal_clicked().connect(
            [this] { trigger_action(ActionID::SEARCH_PREVIOUS); });
    main_window->search_entry->signal_activate().connect([this] { trigger_action(ActionID::SEARCH_NEXT); });

    for (const auto &it : object_descriptions) {
        if (core->can_search_for_object_type(it.first)) {
            auto b = Gtk::manage(new Gtk::CheckButton(it.second.name_pl));
            search_check_buttons.emplace(it.first, b);
            main_window->search_types_box->pack_start(*b, false, false, 0);
            b->set_active(true);
            b->signal_toggled().connect([this] {
                update_search_types_label();
                handle_search();
                search_go(0);
            });
            b->show();
        }
    }
    if (search_check_buttons.size() < 2) {
        main_window->search_expander->hide();
    }
    core->signal_tool_changed().connect([this](ToolID id) {
        if (id != ToolID::NONE) {
            set_search_mode(false);
        }
        else {
            handle_search();
        }
    });

    update_search_types_label();
    search_go(0);


    grid_spin_button = Gtk::manage(new SpinButtonDim());
    grid_spin_button->set_range(0.01_mm, 10_mm);
    grid_spacing_binding = Glib::Binding::bind_property(grid_spin_button->property_value(),
                                                        canvas->property_grid_spacing(), Glib::BINDING_BIDIRECTIONAL);
    grid_spin_button->set_value(1.25_mm);
    grid_spin_button->show_all();
    main_window->grid_box->pack_start(*grid_spin_button, true, true, 0);

    auto save_button = create_action_button(make_action(ActionID::SAVE));
    save_button->show();
    main_window->header->pack_start(*save_button);
    core->signal_needs_save().connect([this](bool v) {
        update_action_sensitivity();
        json j;
        j["op"] = "needs-save";
        j["pid"] = getpid();
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
        rules_window = RulesWindow::create(main_window, canvas, core->get_rules(), core);
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

    for (const auto &la : core->get_layer_provider()->get_layers()) {
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

    imp_interface = std::make_unique<ImpInterface>(this);

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

    {
        json j;
        j["op"] = "ready";
        j["pid"] = getpid();
        send_json(j);
    }

    app->run(*main_window);
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

void ImpBase::hud_update()
{
    if (core->tool_is_active())
        return;

    auto sel = canvas->get_selection();
    if (sel.size()) {
        std::string hud_text = get_hud_text(sel);
        trim(hud_text);
        hud_text += ImpBase::get_hud_text(sel);
        trim(hud_text);
        main_window->hud_update(hud_text);
    }
    else {
        main_window->hud_update("");
    }
}

std::string ImpBase::get_hud_text(std::set<SelectableRef> &sel)
{
    std::string s;
    if (sel_count_type(sel, ObjectType::LINE)) {
        auto n = sel_count_type(sel, ObjectType::LINE);
        s += "\n\n<b>" + std::to_string(n) + " " + object_descriptions.at(ObjectType::LINE).get_name_for_n(n)
             + "</b>\n";
        std::set<int> layers;
        int64_t length = 0;
        for (const auto &it : sel) {
            if (it.type == ObjectType::LINE) {
                const auto li = core->get_line(it.uuid);
                layers.insert(li->layer);
                length += sqrt((li->from->position - li->to->position).mag_sq());
            }
        }
        s += "Layers: ";
        for (auto layer : layers) {
            s += core->get_layer_provider()->get_layers().at(layer).name + " ";
        }
        s += "\nTotal length: " + dim_to_string(length, false);
        sel_erase_type(sel, ObjectType::LINE);
        return s;
    }

    // Display the length if a single edge of a polygon is given
    if (sel_count_type(sel, ObjectType::POLYGON_EDGE) == 1) {

        auto n = sel_count_type(sel, ObjectType::POLYGON_EDGE);
        s += "\n\n<b>" + std::to_string(n) + " " + object_descriptions.at(ObjectType::POLYGON_EDGE).get_name_for_n(n)
             + "</b>\n";
        int64_t length = 0;
        for (const auto &it : sel) {
            if (it.type == ObjectType::POLYGON_EDGE) {
                const auto li = core->get_polygon(it.uuid);
                const auto pair = li->get_vertices_for_edge(it.vertex);
                length += sqrt((li->vertices[pair.first].position - li->vertices[pair.second].position).mag_sq());
                s += "Layer: ";
                s += core->get_layer_provider()->get_layers().at(li->layer).name + " ";
                s += "\nLength: " + dim_to_string(length, false);
                sel_erase_type(sel, ObjectType::POLYGON_EDGE);
                return s;
            }
        }
    }
    trim(s);
    if (sel.size()) {
        s += "\n\n<b>Others:</b>\n";
        for (const auto &it : object_descriptions) {
            auto n = std::count_if(sel.begin(), sel.end(), [&it](const auto &a) { return a.type == it.first; });
            if (n) {
                s += std::to_string(n) + " " + it.second.get_name_for_n(n) + "\n";
            }
        }
        sel.clear();
    }
    return s;
}

std::string ImpBase::get_hud_text_for_net(const Net *net)
{
    if (!net)
        return "No net";

    std::string s = "Net: " + net->name + "\n";
    s += "Net class " + net->net_class->name + "\n";
    if (net->is_power)
        s += "is power net";

    trim(s);
    return s;
}

std::string ImpBase::get_hud_text_for_component(const Component *comp)
{
    const Part *part = comp->part;
    if (!part) {
        std::string s = "No part\n";
        s += "Entity: " + comp->entity->name;
        return s;
    }
    else {
        std::string s = "MPN: " + Glib::Markup::escape_text(part->get_MPN()) + "\n";
        s += "Manufacturer: " + Glib::Markup::escape_text(part->get_manufacturer()) + "\n";
        s += "Package: " + Glib::Markup::escape_text(part->package->name) + "\n";
        if (part->get_description().size())
            s += Glib::Markup::escape_text(part->get_description()) + "\n";
        if (part->get_datasheet().size())
            s += "<a href=\"" + Glib::Markup::escape_text(part->get_datasheet()) + "\" title=\""
                 + Glib::Markup::escape_text(Glib::Markup::escape_text(part->get_datasheet())) + "\">Datasheet</a>\n";

        const auto block = core->get_block();
        if (comp->group)
            s += "Group: " + Glib::Markup::escape_text(block->get_group_name(comp->group)) + "\n";
        if (comp->tag)
            s += "Tag: " + Glib::Markup::escape_text(block->get_tag_name(comp->tag)) + "\n";

        trim(s);
        return s;
    }
}

void ImpBase::edit_pool_item(ObjectType type, const UUID &uu)
{
    json j;
    j["op"] = "edit";
    j["type"] = object_type_lut.lookup_reverse(type);
    j["uuid"] = static_cast<std::string>(uu);
    send_json(j);
}

Gtk::Button *ImpBase::create_action_button(std::pair<ActionID, ToolID> action)
{
    auto &catitem = action_catalog.at(action);
    auto button = Gtk::manage(new Gtk::Button(catitem.name));
    signal_action_sensitive().connect([this, button, action] { button->set_sensitive(get_action_sensitive(action)); });
    button->signal_clicked().connect([this, action] { trigger_action(action); });
    button->set_sensitive(get_action_sensitive(action));
    return button;
}

bool ImpBase::trigger_action(const std::pair<ActionID, ToolID> &action)
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
            ToolID tool_id;
            if (ctrl)
                tool_id = ToolID::DUPLICATE;
            else
                tool_id = ToolID::MOVE;

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
    {
        const auto mod0 = static_cast<GdkModifierType>(0);
        action_connections.at(make_action(ToolID::MOVE_KEY_LEFT)).key_sequences = {{{GDK_KEY_Left, mod0}}};
        action_connections.at(make_action(ToolID::MOVE_KEY_RIGHT)).key_sequences = {{{GDK_KEY_Right, mod0}}};
        action_connections.at(make_action(ToolID::MOVE_KEY_UP)).key_sequences = {{{GDK_KEY_Up, mod0}}};
        action_connections.at(make_action(ToolID::MOVE_KEY_DOWN)).key_sequences = {{{GDK_KEY_Down, mod0}}};
    }
    {
        switch (canvas_prefs->appearance.grid_fine_modifier) {
        case Appearance::GridFineModifier::ALT:
            grid_fine_modifier = GDK_MOD1_MASK;
            break;
        case Appearance::GridFineModifier::CTRL:
            grid_fine_modifier = GDK_CONTROL_MASK;
            break;
        }
        action_connections.at(make_action(ToolID::MOVE_KEY_FINE_LEFT)).key_sequences = {
                {{GDK_KEY_Left, grid_fine_modifier}}};
        action_connections.at(make_action(ToolID::MOVE_KEY_FINE_RIGHT)).key_sequences = {
                {{GDK_KEY_Right, grid_fine_modifier}}};
        action_connections.at(make_action(ToolID::MOVE_KEY_FINE_UP)).key_sequences = {
                {{GDK_KEY_Up, grid_fine_modifier}}};
        action_connections.at(make_action(ToolID::MOVE_KEY_FINE_DOWN)).key_sequences = {
                {{GDK_KEY_Down, grid_fine_modifier}}};
    }

    key_sequence_dialog->clear();
    for (const auto &it : action_connections) {
        if (it.second.key_sequences.size()) {
            key_sequence_dialog->add_sequence(it.second.key_sequences, action_catalog.at(it.first).name);
            tool_popover->set_key_sequences(it.first, it.second.key_sequences);
        }
    }
    canvas->smooth_zoom = preferences.zoom.smooth_zoom_2d;
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

void ImpBase::set_action_sensitive(std::pair<ActionID, ToolID> action, bool v)
{
    action_sensitivity[action] = v;
    s_signal_action_sensitive.emit();
}

bool ImpBase::get_action_sensitive(std::pair<ActionID, ToolID> action) const
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
    auto layers = core->get_layer_provider()->get_layers();
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
    if (core->get_layer_provider()->get_layers().count(layer)) {
        canvas->property_work_layer() = layer;
    }
}

bool ImpBase::handle_key_press(GdkEventKey *key_event)
{
    if (core->tool_is_active()) {
        if (handle_action_key(key_event))
            return true;

        ToolArgs args;
        args.coords = canvas->get_cursor_pos();
        args.work_layer = canvas->property_work_layer();
        if (!core->tool_handles_esc() && key_event->keyval == GDK_KEY_Escape) {
            args.type = ToolEventType::CLICK;
            args.button = 3;
        }
        else {
            args.type = ToolEventType::KEY;
            args.key = key_event->keyval;
            if (key_event->state & grid_fine_modifier)
                args.mod = ToolArgs::MOD_FINE;
        }

        ToolResponse r = core->tool_update(args);
        tool_process(r);
        return true;
    }
    else {
        return handle_action_key(key_event);
    }
    return false;
}

bool ImpBase::handle_action_key(GdkEventKey *ev)
{
    if (ev->is_modifier)
        return false;
    if (ev->keyval == GDK_KEY_Escape) {
        if (!core->tool_is_active()) {
            canvas->set_selection_mode(CanvasGL::SelectionMode::HOVER);
            canvas->set_selection({});
            set_search_mode(false);
        }
        if (keys_current.size() == 0) {
            return false;
        }
        else {
            keys_current.clear();
            main_window->tool_hint_label->set_text(key_sequence_to_string(keys_current));
            return true;
        }
    }
    else {
        auto display = main_window->get_display()->gobj();
        auto hw_keycode = ev->hardware_keycode;
        auto state = static_cast<GdkModifierType>(ev->state);
        auto group = ev->group;
        guint keyval;
        GdkModifierType consumed_modifiers;
        if (gdk_keymap_translate_keyboard_state(gdk_keymap_get_for_display(display), hw_keycode, state, group, &keyval,
                                                NULL, NULL, &consumed_modifiers)) {
            auto mod = static_cast<GdkModifierType>((state & (~consumed_modifiers))
                                                    & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK));
            keys_current.emplace_back(keyval, mod);
        }
        std::set<ActionConnection *> connections_matched;
        auto selection = canvas->get_selection();
        update_action_sensitivity();
        for (auto &it : action_connections) {
            auto k = std::make_pair(it.second.action_id, it.second.tool_id);
            if (action_catalog.at(k).availability & get_editor_type_for_action()) {
                bool can_begin = false;
                if (it.second.action_id == ActionID::TOOL && !core->tool_is_active()) {
                    can_begin = core->tool_can_begin(it.second.tool_id, selection).first;
                }
                else if (it.second.tool_id == ToolID::NONE) {
                    can_begin = get_action_sensitive(k);
                }
                if (can_begin) {
                    for (const auto &it2 : it.second.key_sequences) {
                        auto minl = std::min(keys_current.size(), it2.size());
                        if (minl && std::equal(keys_current.begin(), keys_current.begin() + minl, it2.begin())) {
                            connections_matched.insert(&it.second);
                        }
                    }
                }
            }
        }
        if (connections_matched.size() == 0) {
            main_window->tool_hint_label->set_text("Unknown key sequence");
            keys_current.clear();
            return false;
        }
        else if (connections_matched.size() > 1) { // still ambigous
            main_window->tool_hint_label->set_text(key_sequence_to_string(keys_current) + "?");
            return true;
        }
        else if (connections_matched.size() == 1) {
            main_window->tool_hint_label->set_text(key_sequence_to_string(keys_current));
            keys_current.clear();
            auto conn = *connections_matched.begin();
            if (!trigger_action({conn->action_id, conn->tool_id})) {
                main_window->tool_hint_label->set_text(">");
                return false;
            }
            return true;
        }
        else {
            assert(false); // don't go here
        }
    }
    return false;
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
        args.type = ToolEventType::CLICK_RELEASE;
        args.coords = canvas->get_cursor_pos();
        args.button = button_event->button;
        args.target = canvas->get_current_target();
        args.work_layer = canvas->property_work_layer();
        ToolResponse r = core->tool_update(args);
        tool_process(r);
    }
    return false;
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
                        auto la_sub = Gtk::manage(new Gtk::MenuItem(it.second.name));
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
                        auto la_sub = Gtk::manage(new Gtk::MenuItem(it.second.name));
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
        args.type = ToolEventType::CLICK;
        args.coords = canvas->get_cursor_pos();
        args.button = button_event->button;
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
    else if (!core->tool_is_active() && button_event->button == 1) {
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
                    std::string text = object_descriptions.at(sr.type).name;
                    auto display_name = core->get_display_name(sr.type, sr.uuid);
                    if (display_name.size()) {
                        text += " " + display_name;
                    }
                    auto layers = core->get_layer_provider()->get_layers();
                    if (layers.count(sr.layer)) {
                        text += " (" + layers.at(sr.layer).name + ")";
                    }
                    auto la = Gtk::manage(new Gtk::MenuItem(text));
                    la->signal_select().connect([this, sr] { canvas->set_selection({sr}, false); });
                    la->signal_deselect().connect([this] { canvas->set_selection({}, false); });
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
        no_update = false;
        highlights.clear();
        update_highlights();
    }
    if (!no_update) {
        canvas->fast_draw = resp.fast_draw;
        canvas_update();
        canvas->fast_draw = false;
        canvas->set_selection(core->get_tool_selection());
    }
    if (resp.layer != 10000) {
        canvas->property_work_layer() = resp.layer;
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
        main_window->property_scrolled_window->set_visible(show_properties);
    }
}

void ImpBase::handle_tool_change(ToolID id)
{
    panels->set_sensitive(id == ToolID::NONE);
    canvas->set_selection_allowed(id == ToolID::NONE);
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

void ImpBase::set_monitor_items(const std::set<std::pair<ObjectType, UUID>> &items)
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

void ImpBase::handle_search()
{
    Core::SearchQuery q;
    q.set_query(main_window->search_entry->get_text());
    for (auto &it : search_check_buttons) {
        if (it.second->get_active()) {
            q.types.insert(it.first);
        }
    }
    auto min_c = canvas->screen2canvas({0, canvas->get_height()});
    auto max_c = canvas->screen2canvas({canvas->get_width(), 0});
    q.area_visible = {min_c, max_c};
    search_result_current = 0;
    search_results = core->search(q);
    update_search_markers();
}

void ImpBase::search_go(int dir)
{
    if (search_results.size() == 0) {
        main_window->search_status_label->set_text("No matches");
        return;
    }
    auto &dom = canvas->markers.get_domain(MarkerDomain::SEARCH);
    dom.at(search_result_current).size = MarkerRef::Size::SMALL;
    dom.at(search_result_current).color = get_canvas_preferences()->appearance.colors.at(ColorP::SEARCH);
    if (dir > 0) {
        search_result_current = (search_result_current + 1) % search_results.size();
    }
    else if (dir < 0) {
        if (search_result_current)
            search_result_current--;
        else
            search_result_current = search_results.size() - 1;
    }
    dom.at(search_result_current).size = MarkerRef::Size::DEFAULT;
    dom.at(search_result_current).color = get_canvas_preferences()->appearance.colors.at(ColorP::SEARCH_CURRENT);
    auto n_results = search_results.size();
    std::string status;
    if (n_results == 1) {
        status = "One match:";
    }
    else {
        status = "Match " + format_m_of_n(search_result_current + 1, search_results.size()) + ":";
    }
    auto &res = *std::next(search_results.begin(), search_result_current);
    status += " " + object_descriptions.at(res.type).name + " " + core->get_display_name(res.type, res.uuid, res.sheet);
    main_window->search_status_label->set_text(status);
    canvas->update_markers();
    search_center(res);
}

void ImpBase::search_center(const Core::SearchResult &res)
{
    auto c = res.location;
    auto min_c = canvas->screen2canvas({0, canvas->get_height()});
    auto max_c = canvas->screen2canvas({canvas->get_width(), 0});
    if (c.x > max_c.x || c.y > max_c.y || c.x < min_c.x || c.y < min_c.y) {
        canvas->center_and_zoom(c);
    }
}

void ImpBase::update_search_markers()
{
    auto &dom = canvas->markers.get_domain(MarkerDomain::SEARCH);
    dom.clear();
    for (const auto &it : search_results) {
        dom.emplace_back(it.location, get_canvas_preferences()->appearance.colors.at(ColorP::SEARCH), it.sheet);
        dom.back().size = MarkerRef::Size::SMALL;
    }
    canvas->update_markers();
}

void ImpBase::set_search_mode(bool enabled, bool focus)
{
    main_window->search_revealer->set_reveal_child(enabled);

    if (enabled) {
        if (focus)
            main_window->search_entry->grab_focus();
    }
    else {
        canvas->grab_focus();
    }

    canvas->markers.set_domain_visible(MarkerDomain::SEARCH, enabled);
}

void ImpBase::update_search_types_label()
{
    size_t n_enabled = 0;
    for (auto &it : search_check_buttons) {
        if (it.second->get_active()) {
            n_enabled++;
        }
    }
    std::string la;
    if (n_enabled == 0) {
        la = "No object types enabled";
    }
    else if (n_enabled == search_check_buttons.size()) {
        la = "All object types";
    }
    else if (n_enabled > (search_check_buttons.size() / 2)) {
        la = "All but ";
        for (auto &it : search_check_buttons) {
            if (!it.second->get_active()) {
                la += object_descriptions.at(it.first).name_pl + ", ";
            }
        }
        la.pop_back();
        la.pop_back();
    }

    else {
        for (auto &it : search_check_buttons) {
            if (it.second->get_active()) {
                la += object_descriptions.at(it.first).name_pl + ", ";
            }
        }
        la.pop_back();
        la.pop_back();
    }
    main_window->search_expander->set_label(la);
}

std::pair<ActionID, ToolID> ImpBase::get_doubleclick_action(ObjectType type, const UUID &uu)
{
    switch (type) {
    case ObjectType::TEXT:
        return make_action(ToolID::ENTER_DATUM);
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

} // namespace horizon
