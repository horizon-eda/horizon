#include "imp.hpp"
#include "block/block.hpp"
#include "canvas/canvas_gl.hpp"
#include "export_gerber/gerber_export.hpp"
#include "logger/logger.hpp"
#include "pool/part.hpp"
#include "pool/project_pool.hpp"
#include "property_panels/property_panels.hpp"
#include "rules/rules_window.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "util/geom_util.hpp"
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
#include "actions.hpp"
#include "preferences/preferences_util.hpp"
#include "widgets/action_button.hpp"
#include "in_tool_action_catalog.hpp"
#include "util/zmq_helper.hpp"
#include "pool/pool_parametric.hpp"
#include "action_icon.hpp"
#include "grids_window.hpp"
#include "widgets/msd_tuning_window.hpp"
#include "pool/pool_manager.hpp"
#include "core/tools/tool_data_pool_updated.hpp"

#ifdef G_OS_WIN32
#include <winsock2.h>
#endif
#include "util/win32_undef.hpp"

namespace horizon {

class PoolWithParametric : public ProjectPool {
public:
    PoolWithParametric(const std::string &bp, bool cache) : ProjectPool(bp, cache), parametric(bp)
    {
    }

    PoolParametric *get_parametric() override
    {
        return &parametric;
    }

private:
    PoolParametric parametric;
};

static std::unique_ptr<Pool> make_pool(const PoolParams &p)
{
    if (PoolInfo(p.base_path).uuid == PoolInfo::project_pool_uuid) {
        return std::make_unique<PoolWithParametric>(p.base_path, false);
    }
    else {
        return std::make_unique<Pool>(p.base_path);
    }
}


static std::unique_ptr<Pool> make_pool_caching(const PoolParams &p)
{
    if (PoolInfo(p.base_path).uuid == PoolInfo::project_pool_uuid) {
        return std::make_unique<PoolWithParametric>(p.base_path, true);
    }
    else {
        return nullptr;
    }
}


ImpBase::ImpBase(const PoolParams &params)
    : pool(make_pool(params)), real_pool_caching(make_pool_caching(params)),
      pool_caching(real_pool_caching ? real_pool_caching.get() : pool.get()), core(nullptr),
      sock_broadcast_rx(zctx, ZMQ_SUB), sock_project(zctx, ZMQ_REQ), drag_tool(ToolID::NONE)
{
    auto ep_broadcast = Glib::getenv("HORIZON_EP_BROADCAST");
    if (ep_broadcast.size()) {
        sock_broadcast_rx.connect(ep_broadcast);
        zmq_helper::subscribe_int(sock_broadcast_rx, 0);
        zmq_helper::subscribe_int(sock_broadcast_rx, getpid());
        auto chan = zmq_helper::io_channel_from_socket(sock_broadcast_rx);

        {
            auto pid_p = Glib::getenv("HORIZON_MGR_PID");
            if (pid_p.size())
                mgr_pid = std::stoi(pid_p);
        }

        {
            auto ipc_cookie_p = Glib::getenv("HORIZON_IPC_COOKIE");
            if (ipc_cookie_p.size())
                ipc_cookie = ipc_cookie_p;
        }

        Glib::signal_io().connect(
                [this](Glib::IOCondition cond) {
                    while (zmq_helper::can_recv(sock_broadcast_rx)) {
                        zmq::message_t msg;
                        if (zmq_helper::recv(sock_broadcast_rx, msg)) {
                            if (msg.size() < (UUID::size + sizeof(int) + 1))
                                throw std::runtime_error("received short message");

                            int prefix;
                            auto m = reinterpret_cast<const char *>(msg.data());
                            memcpy(&prefix, m, sizeof(int));
                            if (memcmp(m + sizeof(int), ipc_cookie.get_bytes(), UUID::size) != 0)
                                throw std::runtime_error("IPC cookie didn't match");

                            auto data = m + sizeof(int) + UUID::size;
                            json j = json::parse(data);
                            if (prefix == 0 || prefix == getpid()) {
                                handle_broadcast(j);
                            }
                        }
                    }
                    return true;
                },
                chan, Glib::IO_IN | Glib::IO_HUP);
    }
    auto ep_project = Glib::getenv("HORIZON_EP_MGR");
    if (ep_project.size()) {
        sock_project.connect(ep_project);
        zmq_helper::set_timeouts(sock_project, 5000);
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
    const std::string op = j.value("op", "unknown");
    zmq::message_t msg(UUID::size + s.size() + 1);
    auto m = (char *)msg.data();
    memcpy(m, ipc_cookie.get_bytes(), UUID::size);
    memcpy(m + UUID::size, s.c_str(), s.size());
    m[msg.size() - 1] = 0;
    try {
        if (zmq_helper::send(sock_project, msg) == false) {
            sockets_broken = true;
            sockets_connected = false;
            show_sockets_broken_dialog("send timeout, op=" + op);
            return nullptr;
        }
    }
    catch (zmq::error_t &e) {
        sockets_broken = true;
        sockets_connected = false;
        show_sockets_broken_dialog(e.what() + std::string(", op=") + op);
        return nullptr;
    }

    zmq::message_t rx;
    try {
        if (zmq_helper::recv(sock_project, rx) == false) {
            sockets_broken = true;
            sockets_connected = false;
            show_sockets_broken_dialog("receive timeout, op=" + op);
            return nullptr;
        }
    }
    catch (zmq::error_t &e) {
        sockets_broken = true;
        sockets_connected = false;
        show_sockets_broken_dialog(e.what() + std::string(", op=") + op);
        return nullptr;
    }
    char *rxdata = ((char *)rx.data());
    return json::parse(rxdata);
}

bool ImpBase::handle_close(const GdkEventAny *ev)
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
            if (force_end_tool()) {
                trigger_action(ActionID::SAVE);
                if (core->get_needs_save())
                    return true; // saving didn't work, keep open
                core->delete_autosave();
                return false; // close
            }
            else {
                return true;
            }


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
    g_simple_action_set_state(bottom_view_action->gobj(), g_variant_new_boolean(canvas->get_flip_view()));
}

static std::string append_if_not_empty(const std::string &prefix, const std::string &suffix)
{
    if (suffix.size())
        return prefix + " " + suffix;
    else
        return prefix;
}

void ImpBase::run(int argc, char *argv[])
{
    auto app = Gtk::Application::create(argc, argv, "org.horizon_eda.HorizonEDA.imp", Gio::APPLICATION_NON_UNIQUE);

    main_window = MainWindow::create();
    canvas = main_window->canvas;
    clipboard = ClipboardBase::create(*core);
    clipboard_handler = std::make_unique<ClipboardHandler>(*clipboard);

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
        set_action_sensitive(ActionID::CLICK_SELECT, mode == CanvasGL::SelectionMode::HOVER);
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

    if (core->has_object_type(ObjectType::PICTURE)) {
        show_pictures_action = main_window->add_action_bool("show_pictures", canvas->show_pictures);
        show_pictures_action->signal_change_state().connect([this](const Glib::VariantBase &v) {
            auto b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(v).get();
            if (b != canvas->show_pictures) {
                trigger_action(ActionID::TOGGLE_PICTURES);
            }
        });
        connect_action(ActionID::TOGGLE_PICTURES, [this](const auto &a) {
            canvas->show_pictures = !canvas->show_pictures;
            canvas->queue_draw();
            g_simple_action_set_state(show_pictures_action->gobj(), g_variant_new_boolean(canvas->show_pictures));
            update_view_hints();
        });
    }

    connect_action(ActionID::SELECTION_FILTER, [this](const auto &a) { selection_filter_dialog->present(); });
    connect_action(ActionID::SAVE, [this](const auto &a) {
        if (!read_only) {
            if (core->get_filename().empty()) {
                if (!set_filename())
                    return;
            }
            core->save();
            save_button->set_label("Save");
            json j;
            this->get_save_meta(j);
            if (!j.is_null()) {
                save_json_to_file(core->get_filename() + meta_suffix, j);
            }

            if (uses_dynamic_version()) {
                saved_version = get_required_version();
                check_version();
            }
            else {
                main_window->set_version_info("");
            }
        }
    });
    connect_action_with_source(ActionID::UNDO, [this](const auto &a, auto src) {
        if (src == ActionSource::KEY && preferences.undo_redo.show_hints)
            main_window->set_undo_redo_hint("Undid " + core->get_undo_comment());
        core->undo();
        this->canvas_update_from_pp();
        this->update_property_panels();
    });
    connect_action_with_source(ActionID::REDO, [this](const auto &a, auto src) {
        if (src == ActionSource::KEY && preferences.undo_redo.show_hints)
            main_window->set_undo_redo_hint("Redid " + core->get_redo_comment());
        core->redo();
        this->canvas_update_from_pp();
        this->update_property_panels();
    });

    connect_action(ActionID::RELOAD_POOL, [this](const auto &a) {
        core->reload_pool();
        this->canvas_update_from_pp();
    });

    connect_action(ActionID::COPY, [this](const auto &a) {
        clipboard_handler->copy(canvas->get_selection(), canvas->get_cursor_pos());
    });

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
            if (it.first.is_tool()) {
                bool r = core->tool_can_begin(it.first.tool, sel).first;
                can_begin[it.first] = r;
            }
            else {
                can_begin[it.first] = this->get_action_sensitive(it.first.action);
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

    connect_action(ActionID::SELECT_POLYGON, sigc::mem_fun(*this, &ImpBase::handle_select_polygon));

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

    if (auto grid_settings = core->get_grid_settings()) {
        if (!m_meta.is_null()) {
            if (m_meta.count("grid_settings")) {
                const auto &s = m_meta.at("grid_settings");
                auto &cur = grid_settings->current;
                cur.spacing_square = s.at("spacing_square").get<uint64_t>();
                cur.origin.x = s.at("origin_x").get<int64_t>();
                cur.origin.y = s.at("origin_y").get<int64_t>();
                cur.spacing_rect.x = s.at("spacing_x").get<int64_t>();
                cur.spacing_rect.y = s.at("spacing_y").get<int64_t>();
                if (s.at("mode").get<std::string>() == "rect") {
                    cur.mode = GridSettings::Grid::Mode::RECTANGULAR;
                }
                else {
                    cur.mode = GridSettings::Grid::Mode::SQUARE;
                }
            }
            else {
                grid_settings->current.spacing_square = m_meta.value("grid_spacing", 1.25_mm);
            }
        }
    }

    grid_controller.emplace(*main_window, *canvas, core->get_grid_settings());
    connect_action(ActionID::SET_GRID_ORIGIN, [this](const auto &c) {
        auto co = canvas->get_cursor_pos();
        grid_controller->set_origin(co);
    });

    if (auto grid_settings = core->get_grid_settings()) {
        grids_window = GridsWindow::create(main_window, *grid_controller, *grid_settings);
        connect_action(ActionID::GRIDS_WINDOW, [this](const auto &c) {
            grids_window->set_select_mode(false);
            grids_window->present();
        });
        connect_action(ActionID::SELECT_GRID, [this](const auto &c) {
            grids_window->set_select_mode(true);
            grids_window->present();
        });
        main_window->grid_window_button->signal_clicked().connect([this] { trigger_action(ActionID::GRIDS_WINDOW); });
        grids_window->signal_changed().connect([this] {
            core->set_needs_save();
            set_action_sensitive(ActionID::SELECT_GRID, grids_window->has_grids());
        });
        set_action_sensitive(ActionID::SELECT_GRID, grids_window->has_grids());
    }

    save_button = create_action_button(ActionID::SAVE);
    if (temp_mode)
        save_button->set_label("Save as…");
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
    set_action_sensitive(ActionID::SAVE, false);

    {
        auto undo_redo_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
        undo_redo_box->get_style_context()->add_class("linked");

        undo_button = create_action_button(ActionID::UNDO);
        gtk_button_set_label(undo_button->gobj(), NULL);
        undo_button->set_tooltip_text("Undo");
        undo_button->set_image_from_icon_name("edit-undo-symbolic", Gtk::ICON_SIZE_BUTTON);
        undo_redo_box->pack_start(*undo_button, false, false, 0);

        redo_button = create_action_button(ActionID::REDO);
        gtk_button_set_label(redo_button->gobj(), NULL);
        redo_button->set_tooltip_text("Redo");
        redo_button->set_image_from_icon_name("edit-redo-symbolic", Gtk::ICON_SIZE_BUTTON);
        undo_redo_box->pack_start(*redo_button, false, false, 0);

        undo_redo_box->show_all();
        main_window->header->pack_start(*undo_redo_box);
    }

    core->signal_can_undo_redo().connect([this] {
        undo_button->set_tooltip_text(append_if_not_empty("Undo", core->get_undo_comment()));
        redo_button->set_tooltip_text(append_if_not_empty("Redo", core->get_redo_comment()));
        update_action_sensitivity();
    });
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
        rules_window = RulesWindow::create(main_window, *canvas, *core, is_layered());
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
            auto button = create_action_button(ActionID::RULES);
            button->set_label("Rules…");
            main_window->header->pack_start(*button);
            button->show();
        }
    }

    msd_tuning_window = new MSDTuningWindow();
    msd_tuning_window->set_transient_for(*main_window);
    msd_tuning_window->signal_changed().connect(
            [this] { canvas->set_msd_params(msd_tuning_window->get_msd_params()); });
    connect_action(ActionID::MSD_TUNING_WINDOW, [this](const auto &conn) { msd_tuning_window->present(); });

    tool_popover = Gtk::manage(new ToolPopover(canvas, get_editor_type_for_action()));
    tool_popover->set_position(Gtk::POS_BOTTOM);
    tool_popover->signal_action_activated().connect([this](ActionToolID action_id) { trigger_action(action_id); });


    log_window = new LogWindow();
    log_window->set_transient_for(*main_window);
    log_dispatcher.set_handler([this](const auto &it) { log_window->get_view()->push_log(it); });
    Logger::get().set_log_handler([this](const Logger::Item &it) { log_dispatcher.log(it); });
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

    auto view_options_popover = Gtk::manage(new Gtk::PopoverMenu);
    main_window->view_options_button->set_popover(*view_options_popover);
    {
        Gdk::Rectangle rect;
        rect.set_width(24);
        main_window->view_options_button->get_popover()->set_pointing_to(rect);
    }

    view_options_menu = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    view_options_menu->set_border_width(9);
    view_options_popover->add(*view_options_menu);
    view_options_menu->show();
    view_options_popover->child_property_submenu(*view_options_menu) = "main";

    view_options_menu_append_action("Distraction free mode", "win.distraction_free");
    add_tool_action(ActionID::SELECTION_FILTER, "selection_filter");
    view_options_menu_append_action("Selection filter", "win.selection_filter");
    if (core->has_object_type(ObjectType::PICTURE))
        view_options_menu_append_action("Pictures", "win.show_pictures");

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
        handle_drag();
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

    if (core->get_top_block()) {
        core->signal_rebuilt().connect(sigc::mem_fun(*this, &ImpBase::set_window_title_from_block));
        set_window_title_from_block();
    }
    update_view_hints();

    reset_tool_hint_label();

    saved_version = core->get_version().get_file();
    {
        const auto &version = core->get_version();
        if (version.get_app() < version.get_file()) {
            set_read_only(true);
        }
    }
    check_version();
    if (uses_dynamic_version())
        core->signal_modified().connect(sigc::mem_fun(*this, &ImpBase::check_version));

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
    j["time"] = gtk_get_current_event_time();
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

    if (preferences.mouse.drag_polygon_edges && sel.size() == 1 && sel.begin()->type == ObjectType::POLYGON_EDGE)
        return ToolID::DRAG_POLYGON_EDGE;

    if (preferences.mouse.drag_to_move)
        return ToolID::MOVE;
    else
        return ToolID::NONE;
}

void ImpBase::handle_drag()
{
    if (drag_tool == ToolID::NONE)
        return;
    if (selection_for_drag_move.size() == 0)
        return;
    auto pos = canvas->get_cursor_pos_win();
    auto delta = pos - cursor_pos_drag_begin;
    if (delta.mag() > 10) {
        clear_highlights();
        update_highlights();
        ToolArgs args;
        args.coords = cursor_pos_grid_drag_begin;
        args.selection = selection_for_drag_move;

        ToolResponse r = core->tool_begin(drag_tool, args, imp_interface.get(), true);
        tool_process(r);

        selection_for_drag_move.clear();
        drag_tool = ToolID::NONE;
    }
}

void ImpBase::apply_preferences()
{
    const auto &canvas_prefs = get_canvas_preferences();
    canvas->set_appearance(canvas_prefs.appearance);
    if (core->tool_is_active()) {
        canvas->set_cursor_size(canvas_prefs.appearance.cursor_size_tool);
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
    main_window->tool_bar_set_vertical(preferences.tool_bar.vertical_layout);
    core->set_history_max(preferences.undo_redo.max_depth);
    core->set_history_never_forgets(preferences.undo_redo.never_forgets);
    preferences_apply_appearance(preferences);
}

void ImpBase::canvas_update_from_pp()
{
    if (core->tool_is_active()) {
        canvas_update();
        canvas->set_selection(core->get_tool_selection());
        return;
    }

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
    clear_highlights();
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

bool ImpBase::handle_click_release(const GdkEventButton *button_event)
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
                if (it.first.is_tool()) {
                    auto r = core->tool_can_begin(it.first.tool, {sel});
                    if (r.first && r.second) {
                        auto la_sub = create_context_menu_item(it.first);
                        ToolID tool_id = it.first.tool;
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
                    if (get_action_sensitive(it.first.action)
                        && (it.second.flags & ActionCatalogItem::FLAGS_SPECIFIC)) {
                        auto la_sub = create_context_menu_item(it.first);
                        ActionID action_id = it.first.action;
                        std::set<SelectableRef> sr(sel);
                        la_sub->signal_activate().connect([this, action_id, sr] {
                            canvas->set_selection(sr, false);
                            fix_cursor_pos();
                            trigger_action(action_id);
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

void ImpBase::reset_tool_hint_label()
{
    const auto act = ActionID::POPOVER;
    if (action_connections.count(act)) {
        if (action_connections.at(act).key_sequences.size()) {
            const auto keys = key_sequence_to_string(action_connections.at(act).key_sequences.front());
            main_window->tool_hint_label->set_text("> " + keys + " for menu");
            return;
        }
    }
    main_window->tool_hint_label->set_text(">");
}

bool ImpBase::handle_click(const GdkEventButton *button_event)
{
    if (button_event->button > 3) {
        handle_extra_button(button_event);
        return false;
    }

    if (button_event->button != 2)
        set_search_mode(false);

    main_window->key_hint_set_visible(false);

    bool need_menu = false;
    if (core->tool_is_active() && button_event->button != 2 && !(button_event->state & Gdk::SHIFT_MASK)
        && button_event->type != GDK_3BUTTON_PRESS) {
        const bool is_doubleclick = button_event->type == GDK_2BUTTON_PRESS;
        const bool tool_supports_doubleclick = core->get_tool_actions().count(InToolActionID::LMB_DOUBLE);
        if (tool_supports_doubleclick || !is_doubleclick) {
            ToolArgs args;
            args.type = ToolEventType::ACTION;
            if (button_event->button == 1) {
                if (is_doubleclick)
                    args.action = InToolActionID::LMB_DOUBLE;
                else
                    args.action = InToolActionID::LMB;
            }
            else {
                args.action = InToolActionID::RMB;
            }
            args.coords = canvas->get_cursor_pos();
            args.target = canvas->get_current_target();
            args.work_layer = canvas->property_work_layer();
            ToolResponse r = core->tool_update(args);
            tool_process(r);
        }
    }
    else if (!core->tool_is_active() && button_event->type == GDK_2BUTTON_PRESS) {
        auto sel = canvas->get_selection();
        if (sel.size() == 1) {
            auto a = get_doubleclick_action(sel.begin()->type, sel.begin()->uuid);
            if (a.is_valid()) {
                selection_for_drag_move.clear();
                trigger_action(a);
            }
        }
    }
    else if (!core->tool_is_active() && button_event->button == 1 && !(button_event->state & Gdk::SHIFT_MASK)) {
        handle_maybe_drag(button_event->state & Gdk::CONTROL_MASK);
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
            text += " (" + layers.at(sr.layer.end()).name + +" - " + layers.at(sr.layer.start()).name + ")";
        else
            text += " (" + layers.at(sr.layer.start()).name + ")";
    }
    return text;
}


void ImpBase::handle_maybe_drag(bool ctrl)
{
    auto c = canvas->screen2canvas(canvas->get_cursor_pos_win());
    auto sel_at_cursor = canvas->get_selection_at(Coordi(c.x, c.y));
    auto sel_from_canvas = canvas->get_selection();
    std::set<SelectableRef> isect;
    std::set_intersection(sel_from_canvas.begin(), sel_from_canvas.end(), sel_at_cursor.begin(), sel_at_cursor.end(),
                          std::inserter(isect, isect.begin()));
    if (isect.size()) {
        drag_tool = get_tool_for_drag_move(ctrl, sel_from_canvas);
        if (drag_tool != ToolID::NONE) {
            canvas->inhibit_drag_selection();
            cursor_pos_drag_begin = canvas->get_cursor_pos_win();
            cursor_pos_grid_drag_begin = canvas->get_cursor_pos();
            selection_for_drag_move = sel_from_canvas;
        }
    }
}

void ImpBase::tool_process_one()
{
    if (!core->tool_is_active()) {
        imp_interface->dialogs.close_nonmodal();
        reset_tool_hint_label();
        canvas->set_cursor_external(false);
        canvas->snap_filter.clear();
        no_update = false;
        clear_highlights();
        update_highlights();
    }
    if (!no_update) {
        canvas_update();
        if (core->tool_is_active()) {
            canvas->set_selection(core->get_tool_selection());
        }
        else {
            canvas->set_selection(core->get_tool_selection(),
                                  canvas->get_selection_mode() == CanvasGL::SelectionMode::NORMAL);
        }
    }
}

void ImpBase::tool_process(ToolResponse &resp)
{
    tool_process_one();
    while (auto args = core->get_pending_tool_args()) {
        auto r = core->tool_update(*args);
        if (r.next_tool != ToolID::NONE) {
            Logger::log_critical("shouldn't have received next tool in deferred tool_update", Logger::Domain::CORE);
            return;
        }
        tool_process_one();
    }
    if (resp.next_tool != ToolID::NONE) {
        clear_highlights();
        update_highlights();
        ToolArgs args;
        args.coords = canvas->get_cursor_pos();
        args.keep_selection = true;
        args.data = std::move(resp.data);
        ToolResponse r = core->tool_begin(resp.next_tool, args, imp_interface.get());
        tool_process(r);
    }
}

void ImpBase::clear_highlights()
{
    highlights.clear();
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
        clear_highlights();
        update_highlights();
        update_property_panels();
    }
    if (sockets_connected) {
        auto selection = canvas->get_selection();
        if (selection != last_canvas_selection) {
            handle_selection_cross_probe();
        }
        last_canvas_selection = selection;
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
        canvas->set_cursor_size(get_canvas_preferences().appearance.cursor_size_tool);
    }
    else {
        canvas->set_cursor_size(get_canvas_preferences().appearance.cursor_size);
    }
    main_window->tool_bar_set_visible(id != ToolID::NONE);
    tool_bar_clear_actions();
    main_window->action_bar_revealer->set_reveal_child(preferences.action_bar.show_in_tool || id == ToolID::NONE);
    update_cursor(id);
}

void ImpBase::handle_warning_selected(const Coordi &pos)
{
    canvas->center_and_zoom(pos);
}

bool ImpBase::handle_broadcast(const json &j)
{
    const auto op = j.at("op").get<std::string>();
    guint32 timestamp = j.value("time", 0);
    if (op == "present") {
        main_window->present(timestamp);
        return true;
    }
    else if (op == "save") {
        if (force_end_tool())
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
    else if (op == "reload-pools") {
        PoolManager::get().reload();
        return true;
    }
    else if (op == "pool-updated") {
        const auto path = j.at("path").get<std::string>();
        if (core->tool_is_active() && path == pool->get_base_path()) {
            tool_update_data(std::make_unique<ToolDataPoolUpdated>());
        }
        return false; // so that derived imps can handle this as well
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
    file_monitor_delay_connection.disconnect();
    file_monitor_delay_connection = Glib::signal_timeout().connect_seconds(
            [this, filenames] {
                set_monitor_files(filenames);
                return false;
            },
            1);
}

void ImpBase::handle_file_changed(const Glib::RefPtr<Gio::File> &file1, const Glib::RefPtr<Gio::File> &file2,
                                  Gio::FileMonitorEvent ev)
{
    main_window->show_nonmodal(
            "Pool has changed", "Reload pool", [this] { trigger_action(ActionID::RELOAD_POOL); }, "");
}

void ImpBase::set_read_only(bool v)
{
    read_only = v;
}

void ImpBase::tool_update_data(std::unique_ptr<ToolData> data)
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
        return ToolID::EDIT_TEXT;

    case ObjectType::POLYGON_ARC_CENTER:
    case ObjectType::POLYGON_VERTEX:
    case ObjectType::POLYGON_EDGE:
        return ActionID::SELECT_POLYGON;

    default:
        return {};
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
        main_window->set_title(s + " – " + object_descriptions.at(get_editor_type()).name);
    }
    else {
        main_window->set_title("Untitled " + object_descriptions.at(get_editor_type()).name);
    }
}


void ImpBase::set_window_title_from_block()
{
    std::string title;
    if (core->get_top_block()->project_meta.count("project_title"))
        title = core->get_top_block()->project_meta.at("project_title");

    set_window_title(title);
}

std::vector<std::string> ImpBase::get_view_hints()
{
    std::vector<std::string> r;
    if (distraction_free)
        r.emplace_back("distraction free mode");

    if (selection_filter_dialog->get_filtered())
        r.emplace_back("selection filtered");

    if (!canvas->show_pictures)
        r.emplace_back("no pictures");
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

void ImpBase::check_version()
{
    const auto dyn = uses_dynamic_version();
    if (dyn && !read_only) {
        if (get_required_version() > saved_version) {
            const auto &t = object_descriptions.at(core->get_object_type()).name;
            main_window->set_version_info(
                    "Saving this " + t + " will update it from version " + std::to_string(saved_version) + " to "
                    + std::to_string(get_required_version()) + " . " + FileVersion::learn_more_markup);
        }
        else {
            main_window->set_version_info("");
        }
    }
    else {
        main_window->set_version_info(core->get_version().get_message(core->get_object_type()));
    }
}

void ImpBase::view_options_menu_append_action(const std::string &label, const std::string &action)
{
    auto bu = Gtk::manage(new Gtk::ModelButton);
    bu->set_label(label);
    bu->get_child()->set_halign(Gtk::ALIGN_START);
    gtk_actionable_set_action_name(GTK_ACTIONABLE(bu->gobj()), action.c_str());
    view_options_menu->pack_start(*bu, false, false, 0);
    bu->show();
}

void ImpBase::show_in_pool_manager(ObjectType type, const UUID &uu, ShowInPoolManagerPool p)
{
    json j;
    j["op"] = "show-in-pool-mgr";
    j["type"] = object_type_lut.lookup_reverse(type);
    const auto pool_uuids = pool->get_pool_uuids(type, uu);
    UUID pool_uuid;
    if ((p == ShowInPoolManagerPool::LAST) && pool_uuids.last)
        pool_uuid = pool_uuids.last;
    else
        pool_uuid = pool_uuids.pool;
    j["pool_uuid"] = (std::string)pool_uuid;
    j["uuid"] = (std::string)uu;
    send_json(j);
}

static const char *get_cursor_icon(ToolID id)
{
    switch (id) {
    case ToolID::PLACE_TEXT:
    case ToolID::PLACE_REFDES_AND_VALUE:
    case ToolID::PLACE_SHAPE:
    case ToolID::PLACE_SHAPE_OBROUND:
    case ToolID::PLACE_SHAPE_RECTANGLE:
    case ToolID::PLACE_HOLE:
    case ToolID::PLACE_HOLE_SLOT:
    case ToolID::PLACE_BOARD_HOLE:
    case ToolID::PLACE_PAD:
    case ToolID::PLACE_POWER_SYMBOL:
    case ToolID::PLACE_NET_LABEL:
    case ToolID::PLACE_BUS_LABEL:
    case ToolID::PLACE_BUS_RIPPER:
    case ToolID::PLACE_VIA:
    case ToolID::PLACE_DOT:
        return nullptr;

    default:
        return get_action_icon(id);
    }
}

void ImpBase::update_cursor(ToolID tool_id)
{
    auto win = canvas->get_window();
    if (tool_id == ToolID::NONE) {
        win->set_cursor();
        return;
    }
    static const int icon_size = 16;
    static const int border_width = 1;
    auto surf = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, icon_size + border_width * 2,
                                            icon_size + border_width * 2);
    auto cr = Cairo::Context::create(surf);
    auto theme = Gtk::IconTheme::get_default();
    const auto icon_name = get_cursor_icon(tool_id);
    if (!icon_name) {
        win->set_cursor();
        return;
    }
    auto icon_info = theme->lookup_icon(icon_name, icon_size);
    if (!icon_info) {
        win->set_cursor();
        return;
    }
    bool was_symbolic = false;
    auto pb_black = icon_info.load_symbolic(Gdk::RGBA("#000000"), Gdk::RGBA(), Gdk::RGBA(), Gdk::RGBA(), was_symbolic);
    Gdk::Cairo::set_source_pixbuf(cr, pb_black, border_width, border_width);
    auto pat = cr->get_source();
    for (int x : {-1, 1, 0}) {
        for (int y : {-1, 1, 0}) {
            cr->save();
            cr->translate(x * border_width, y * border_width);
            cr->set_source_rgb(1, 1, 1);
            cr->mask(pat);
            cr->restore();
        }
    }
    Gdk::Cairo::set_source_pixbuf(cr, pb_black, border_width, border_width);
    cr->paint();
    win->set_cursor(Gdk::Cursor::create(win->get_display(), surf, 0, 0));
}

bool ImpBase::set_filename()
{
    throw std::runtime_error("set_filename not implemented");
    return false;
}

void ImpBase::set_suggested_filename(const std::string &s)
{
    suggested_filename = s;
}

} // namespace horizon
