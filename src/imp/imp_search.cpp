#include "imp.hpp"
#include "util/util.hpp"
#include "core/tool_id.hpp"
#include "actions.hpp"

namespace horizon {

void ImpBase::init_search()
{
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

    main_window->search_entry->signal_changed().connect([this] {
        this->handle_search();
        this->search_go(0);
    });
    main_window->search_exact_cb->signal_toggled().connect([this] {
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

    if (has_searcher()) {
        auto &searcher = get_searcher();
        for (const auto &type : searcher.get_types()) {
            auto b = Gtk::manage(new Gtk::CheckButton(Searcher::get_type_info(type).name_pl));
            search_check_buttons.emplace(type, b);
            main_window->search_types_box->pack_start(*b, false, false, 0);
            b->set_active(true);
            b->signal_toggled().connect([this] {
                update_search_types_label();
                handle_search();
                search_go(0);
            });
            b->show();
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
    }
}

void ImpBase::handle_search()
{
    Searcher::SearchQuery q;
    q.set_query(main_window->search_entry->get_text());
    for (auto &it : search_check_buttons) {
        if (it.second->get_active()) {
            q.types.insert(it.first);
        }
    }
    q.exact = main_window->search_exact_cb->get_active();
    auto min_c = canvas->screen2canvas({0, canvas->get_height()});
    auto max_c = canvas->screen2canvas({canvas->get_width(), 0});
    q.area_visible = {min_c, max_c};
    search_result_current = 0;
    search_results = get_searcher().search(q);
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
    dom.at(search_result_current).color = get_canvas_preferences().appearance.colors.at(ColorP::SEARCH);
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
    dom.at(search_result_current).color = get_canvas_preferences().appearance.colors.at(ColorP::SEARCH_CURRENT);
    auto n_results = search_results.size();
    std::string status;
    if (n_results == 1) {
        status = "One match:";
    }
    else {
        status = "Match " + format_m_of_n(search_result_current + 1, search_results.size()) + ":";
    }
    auto &res = *std::next(search_results.begin(), search_result_current);
    status += " " + Searcher::get_type_info(res.type).name + " " + get_searcher().get_display_name(res);
    main_window->search_status_label->set_text(status);
    canvas->update_markers();
    search_center(res);
}

void ImpBase::search_center(const Searcher::SearchResult &res)
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
        UUIDVec sheet;
        if (it.sheet)
            sheet = uuid_vec_append(it.instance_path, it.sheet);
        dom.emplace_back(it.location, get_canvas_preferences().appearance.colors.at(ColorP::SEARCH), sheet);
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
                la += Searcher::get_type_info(it.first).name_pl + ", ";
            }
        }
        la.pop_back();
        la.pop_back();
    }

    else {
        for (auto &it : search_check_buttons) {
            if (it.second->get_active()) {
                la += Searcher::get_type_info(it.first).name_pl + ", ";
            }
        }
        la.pop_back();
        la.pop_back();
    }
    main_window->search_expander->set_label(la);
}
} // namespace horizon
