#include "github_login_window.hpp"
#include "util/gtk_util.hpp"
#include "util/http_client.hpp"
#include "util/github_client.hpp"
#include "util/util.hpp"
#include <iostream>

namespace horizon {

GitHubLoginWindow *GitHubLoginWindow::create(const std::string &fn, const std::string &cid)
{
    GitHubLoginWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/pool-mgr/github_login_window.ui");
    x->get_widget_derived("window", w, fn, cid);
    return w;
}

GitHubLoginWindow::GitHubLoginWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                     const std::string &a_auth_filename, const std::string &cid)
    : Gtk::Window(cobject), auth_filename(a_auth_filename), client_id(cid)
{
    GET_WIDGET(stack);
    GET_WIDGET(code_label);
    GET_WIDGET(done_label);
    GET_WIDGET(error_label);
    GET_WIDGET(code_label);
    GET_WIDGET(copy_button);
    GET_WIDGET(browser_button);

    copy_button->signal_clicked().connect([this] { Gtk::Clipboard::get()->set_text(user_code); });
    browser_button->signal_clicked().connect(
            [this] { gtk_show_uri_on_window(gobj(), verification_uri.c_str(), GDK_CURRENT_TIME, nullptr); });

    dispatcher.connect([this] {
        if (user.size()) { // done
            stack->set_visible_child("done");
            done_label->set_markup("You've successfully authenticated as\n<b>" + user
                                   + "</b>.\n\nYou can now close this window.");
            {
                json j;
                j["token"] = token;
                save_json_to_file(auth_filename, j);
            }
        }
        else if (error_msg.size()) { // error
            stack->set_visible_child("error");
            error_label->set_text("Error while logging in:\n" + error_msg + "\n\nClose this window and try again.");
        }
        else if (user_code.size()) {
            stack->set_visible_child("auth");
            code_label->set_text(user_code);
        }
    });

    thread = std::thread(&GitHubLoginWindow::worker_thread, this);

    signal_hide().connect([this] {
        cancel = true;
        thread.join();
    });
}

std::map<std::string, std::string> decode_form(const std::string &s)
{
    std::map<std::string, std::string> r;
    std::string *value = nullptr;
    std::string key;

    for (const auto c : s) {
        if (value == nullptr) {
            if (c != '=') {
                key += c;
            }
            else {
                value = &r[key];
                key.clear();
            }
        }
        else {
            if (c != '&') {
                *value += c;
            }
            else {
                value = nullptr;
            }
        }
    }

    for (auto &[k, v] : r) {
        v = Glib::uri_unescape_string(v);
    }

    return r;
}

void GitHubLoginWindow::worker_thread()
{
    try {
        HTTP::Client client;
        std::string device_code;
        int interval = 10;
        {
            const auto resp_string_code =
                    client.post_form("https://github.com/login/device/code",
                                     {{"client_id", client_id}, {"scope", "public_repo,workflow"}});
            const auto resp_code = decode_form(resp_string_code);
            for (const auto &[k, v] : resp_code) {
                std::cout << k << " = " << v << std::endl;
            }

            user_code = resp_code.at("user_code");
            verification_uri = resp_code.at("verification_uri");
            dispatcher.emit();

            device_code = resp_code.at("device_code");
            interval = std::stoi(resp_code.at("interval"));
        }
        while (1) {
            if (cancel)
                return;

            const auto resp_string_token =
                    client.post_form("https://github.com/login/oauth/access_token",
                                     {{"client_id", client_id},
                                      {"device_code", device_code},
                                      {"grant_type", "urn:ietf:params:oauth:grant-type:device_code"}});

            const auto resp_token = decode_form(resp_string_token);
            for (const auto &[k, v] : resp_token) {
                std::cout << k << " = " << v << std::endl;
            }
            std::cout << std::endl;

            if (resp_token.count("error")) {
                const auto &error = resp_token.at("error");
                if (error == "authorization_pending") {
                    // it's okay
                }
                else if (error == "slow_down") {
                    interval = std::stoi(resp_token.at("interval"));
                }
                else if (error == "expired_token") {
                    error_msg = "Expired token";
                    break;
                }
                else if (error == "access_denied") {
                    error_msg = "Access denied";
                    break;
                }
                else {
                    error_msg = "unexpected error: " + error;
                    break;
                }
            }
            else {
                token = resp_token.at("access_token");
                break;
            }

            for (int i = 0; i < interval; i++) {
                Glib::usleep(1e6);
                if (cancel)
                    return;
            }
        }
        if (error_msg.size()) {
            dispatcher.emit();
            return;
        }
        if (!token.size()) {
            error_msg = "no token";
            dispatcher.emit();
            return;
        }

        GitHubClient gh_client;
        auto user_info = gh_client.login_token(token);
        user = user_info.at("login").get<std::string>();
    }
    catch (const std::exception &e) {
        error_msg = "exception " + std::string(e.what());
    }
    catch (...) {
        error_msg = "unknown exception";
    }
    dispatcher.emit();
}

} // namespace horizon
