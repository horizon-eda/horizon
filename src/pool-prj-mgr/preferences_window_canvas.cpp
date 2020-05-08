#include "preferences_window_canvas.hpp"
#include "common/lut.hpp"
#include "common/common.hpp"
#include "canvas/color_palette.hpp"
#include "canvas/appearance.hpp"
#include "preferences/preferences.hpp"
#include "board/board_layers.hpp"
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "nlohmann/json.hpp"
#include <set>
#include <iostream>
#include <iomanip>

namespace horizon {

#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        x->get_widget(#name, name);                                                                                    \
    } while (0)

static const std::map<ColorP, std::string> color_names = {
        {ColorP::JUNCTION, "Junction"},
        {ColorP::FRAG_ORPHAN, "Orphan fragment"},
        {ColorP::AIRWIRE_ROUTER, "Router airwire"},
        {ColorP::TEXT_OVERLAY, "Text overlay"},
        {ColorP::HOLE, "Hole"},
        {ColorP::DIMENSION, "Dimension"},
        {ColorP::ERROR, "Error"},
        {ColorP::NET, "Net"},
        {ColorP::BUS, "Bus"},
        {ColorP::FRAME, "Frame"},
        {ColorP::AIRWIRE, "Airwire"},
        {ColorP::PIN, "Pin"},
        {ColorP::PIN_HIDDEN, "Hidden Pin"},
        {ColorP::DIFFPAIR, "Diff. pair"},
        {ColorP::NOPOPULATE_X, "Do not pop. X-out"},
        {ColorP::BACKGROUND, "Background"},
        {ColorP::GRID, "Grid"},
        {ColorP::CURSOR_NORMAL, "Cursor"},
        {ColorP::CURSOR_TARGET, "Cursor on target"},
        {ColorP::ORIGIN, "Origin"},
        {ColorP::MARKER_BORDER, "Marker border"},
        {ColorP::SELECTION_BOX, "Selection box"},
        {ColorP::SELECTION_LINE, "Selection line"},
        {ColorP::SELECTABLE_OUTER, "Selectable outer"},
        {ColorP::SELECTABLE_INNER, "Selectable inner"},
        {ColorP::SELECTABLE_PRELIGHT, "Selectable prelight"},
        {ColorP::SELECTABLE_ALWAYS, "Selectable always"},
        {ColorP::SEARCH, "Search markers"},
        {ColorP::SEARCH_CURRENT, "Current search marker"},
        {ColorP::SHADOW, "Highlight shadow"},
};

static const std::set<ColorP> colors_non_layer = {ColorP::NET,         ColorP::BUS,        ColorP::FRAME,
                                                  ColorP::PIN,         ColorP::PIN_HIDDEN, ColorP::DIFFPAIR,
                                                  ColorP::NOPOPULATE_X};

static const std::set<ColorP> colors_layer = {ColorP::FRAG_ORPHAN, ColorP::AIRWIRE_ROUTER, ColorP::TEXT_OVERLAY,
                                              ColorP::HOLE,        ColorP::DIMENSION,      ColorP::AIRWIRE,
                                              ColorP::SHADOW};

class ColorEditor : public Gtk::Box {
public:
    ColorEditor();
    void construct();
    virtual Color get_color() = 0;
    virtual void set_color(const Color &c) = 0;

protected:
    virtual std::string get_color_name() = 0;
    Gtk::DrawingArea *colorbox = nullptr;
};

ColorEditor::ColorEditor() : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5)
{
}

void ColorEditor::construct()
{
    auto la = Gtk::manage(new Gtk::Label(get_color_name()));
    la->set_xalign(0);
    Gdk::RGBA rgba;
    auto co = get_color();
    rgba.set_red(co.r);
    rgba.set_green(co.g);
    rgba.set_blue(co.b);
    rgba.set_alpha(1);
    colorbox = Gtk::manage(new Gtk::DrawingArea);
    colorbox->set_size_request(20, -1);
    colorbox->show();
    colorbox->signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context> &cr) -> bool {
        auto c = get_color();
        cr->set_source_rgb(c.r, c.g, c.b);
        cr->paint();
        return true;
    });
    pack_start(*colorbox, false, false, 0);
    pack_start(*la, false, false, 0);
    la->show();
    colorbox->show();
    set_margin_start(5);
    set_margin_end(5);
}

class ColorEditorPalette : public ColorEditor {
public:
    ColorEditorPalette(Appearance &a, Preferences *p, ColorP c);

private:
    Appearance &apperance;
    Preferences *prefs;
    ColorP color;

    Color get_color() override;
    void set_color(const Color &c) override;
    std::string get_color_name() override;
};

ColorEditorPalette::ColorEditorPalette(Appearance &a, Preferences *p, ColorP c) : apperance(a), prefs(p), color(c)
{
}

Color ColorEditorPalette::get_color()
{
    return apperance.colors.at(color);
}

void ColorEditorPalette::set_color(const Color &c)
{
    apperance.colors.at(color) = c;
    queue_draw();
    prefs->signal_changed().emit();
}

std::string ColorEditorPalette::get_color_name()
{
    return color_names.at(color);
}

class ColorEditorLayer : public ColorEditor {
public:
    ColorEditorLayer(Appearance &a, Preferences *p, int layer, const std::string &na = "");

private:
    Appearance &apperance;
    Preferences *prefs;
    int layer;
    std::string name;

    Color get_color() override;
    void set_color(const Color &c) override;
    std::string get_color_name() override;
};

ColorEditorLayer::ColorEditorLayer(Appearance &a, Preferences *p, int l, const std::string &na)
    : apperance(a), prefs(p), layer(l), name(na)
{
}

Color ColorEditorLayer::get_color()
{
    return apperance.layer_colors.at(layer);
}

void ColorEditorLayer::set_color(const Color &c)
{
    apperance.layer_colors.at(layer) = c;
    queue_draw();
    prefs->signal_changed().emit();
}

std::string ColorEditorLayer::get_color_name()
{
    if (name.size())
        return name;
    else
        return BoardLayers::get_layer_name(layer);
}

static const std::vector<std::pair<std::string, Appearance::CursorSize>> cursor_size_labels = {
        {"Default", Appearance::CursorSize::DEFAULT},
        {"Large", Appearance::CursorSize::LARGE},
        {"Full", Appearance::CursorSize::FULL},
};

static void make_cursor_size_editor(Gtk::Box *box, Appearance::CursorSize &cursor_size, std::function<void(void)> cb)
{
    std::map<Appearance::CursorSize, Gtk::RadioButton *> widgets;

    box->get_style_context()->add_class("linked");
    Gtk::RadioButton *last_rb = nullptr;
    for (const auto &it : cursor_size_labels) {
        auto rb = Gtk::manage(new Gtk::RadioButton(it.first));
        widgets.emplace(it.second, rb);
        rb->set_mode(false);
        rb->show();
        if (last_rb)
            rb->join_group(*last_rb);
        box->pack_start(*rb, true, true, 0);
        last_rb = rb;
    }
    bind_widget<Appearance::CursorSize>(widgets, cursor_size, [cb](auto _) { cb(); });
}

CanvasPreferencesEditor::CanvasPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                                 Preferences *prefs, CanvasPreferences *canvas_prefs, bool layered)
    : Gtk::Box(cobject), preferences(prefs), canvas_preferences(canvas_prefs), is_layered(layered)
{

    Gtk::RadioButton *canvas_grid_style_cross, *canvas_grid_style_dot, *canvas_grid_style_grid,
            *canvas_grid_fine_mod_alt, *canvas_grid_fine_mod_ctrl;
    GET_WIDGET(canvas_grid_style_cross);
    GET_WIDGET(canvas_grid_style_dot);
    GET_WIDGET(canvas_grid_style_grid);
    GET_WIDGET(canvas_grid_style_grid);
    GET_WIDGET(canvas_grid_fine_mod_alt);
    GET_WIDGET(canvas_grid_fine_mod_ctrl);

    Gtk::Scale *canvas_grid_opacity, *canvas_highlight_dim, *canvas_highlight_lighten;
    GET_WIDGET(canvas_grid_opacity);
    GET_WIDGET(canvas_highlight_dim);
    GET_WIDGET(canvas_highlight_lighten);

    Gtk::Label *canvas_label;
    GET_WIDGET(canvas_label);

    GET_WIDGET(canvas_colors_fb);

    Gtk::Menu *color_preset_menu;
    GET_WIDGET(color_preset_menu);

    color_presets = json_from_resource("/org/horizon-eda/horizon/preferences/color_presets.json");

    {

        {
            auto it = Gtk::manage(new Gtk::MenuItem("Export..."));
            it->signal_activate().connect(sigc::mem_fun(*this, &CanvasPreferencesEditor::handle_export));
            it->show();
            color_preset_menu->append(*it);
        }
        {
            auto it = Gtk::manage(new Gtk::MenuItem("Import..."));
            it->signal_activate().connect(sigc::mem_fun(*this, &CanvasPreferencesEditor::handle_import));
            it->show();
            color_preset_menu->append(*it);
        }
        {
            auto it = Gtk::manage(new Gtk::MenuItem("Default"));
            it->signal_activate().connect(sigc::mem_fun(*this, &CanvasPreferencesEditor::handle_default));
            it->show();
            color_preset_menu->append(*it);
        }
        unsigned int idx = 0;
        for (auto it = color_presets.cbegin(); it != color_presets.cend(); ++it) {
            const auto &v = it.value();
            if (v.value("layered", false) == is_layered) {
                auto item = Gtk::manage(new Gtk::MenuItem(v.at("name").get<std::string>()));
                item->signal_activate().connect([this, idx] { handle_load_preset(idx); });
                item->show();
                color_preset_menu->append(*item);
            }
            idx++;
        }
    }

    color_chooser = Glib::wrap(GTK_COLOR_CHOOSER(gtk_builder_get_object(x->gobj(), "canvas_color_chooser")), true);
    color_chooser_conn = color_chooser->property_rgba().signal_changed().connect([this] {
        auto sel = canvas_colors_fb->get_selected_children();
        if (sel.size() != 1)
            return;
        auto it = dynamic_cast<ColorEditor *>(sel.front()->get_child());
        Color c;
        auto rgba = color_chooser->get_rgba();
        c.r = rgba.get_red();
        c.g = rgba.get_green();
        c.b = rgba.get_blue();
        it->set_color(c);
    });

    canvas_colors_fb->signal_selected_children_changed().connect(
            sigc::mem_fun(*this, &CanvasPreferencesEditor::update_color_chooser));

    if (canvas_preferences == &preferences->canvas_layer) {
        canvas_label->set_text("Affects Padstack, Package and Board");
    }
    else {
        canvas_label->set_text("Affects Symbol and Schematic");
    }

    std::map<Appearance::GridStyle, Gtk::RadioButton *> grid_style_widgets = {
            {Appearance::GridStyle::CROSS, canvas_grid_style_cross},
            {Appearance::GridStyle::DOT, canvas_grid_style_dot},
            {Appearance::GridStyle::GRID, canvas_grid_style_grid},
    };

    std::map<Appearance::GridFineModifier, Gtk::RadioButton *> grid_fine_mod_widgets = {
            {Appearance::GridFineModifier::ALT, canvas_grid_fine_mod_alt},
            {Appearance::GridFineModifier::CTRL, canvas_grid_fine_mod_ctrl},
    };

    auto &appearance = canvas_preferences->appearance;

    bind_widget(grid_style_widgets, appearance.grid_style);
    bind_widget(grid_fine_mod_widgets, appearance.grid_fine_modifier);
    bind_widget(canvas_grid_opacity, appearance.grid_opacity);
    bind_widget(canvas_highlight_dim, appearance.highlight_dim);
    bind_widget(canvas_highlight_lighten, appearance.highlight_lighten);

    for (auto &it : grid_style_widgets) {
        it.second->signal_toggled().connect([this] { preferences->signal_changed().emit(); });
    }
    for (auto &it : grid_fine_mod_widgets) {
        it.second->signal_toggled().connect([this] { preferences->signal_changed().emit(); });
    }
    canvas_grid_opacity->signal_value_changed().connect([this] { preferences->signal_changed().emit(); });
    canvas_highlight_dim->signal_value_changed().connect([this] { preferences->signal_changed().emit(); });
    canvas_highlight_lighten->signal_value_changed().connect([this] { preferences->signal_changed().emit(); });

    Gtk::Box *cursor_size_tool_box, *cursor_size_box;
    GET_WIDGET(cursor_size_box);
    GET_WIDGET(cursor_size_tool_box);
    make_cursor_size_editor(cursor_size_box, appearance.cursor_size, [this] { preferences->signal_changed().emit(); });
    make_cursor_size_editor(cursor_size_tool_box, appearance.cursor_size_tool,
                            [this] { preferences->signal_changed().emit(); });

    {
        Gtk::SpinButton *min_line_width_sp;
        GET_WIDGET(min_line_width_sp);

        min_line_width_sp->signal_output().connect([min_line_width_sp] {
            auto v = min_line_width_sp->get_value();
            std::ostringstream oss;
            oss.imbue(get_locale());
            oss << std::fixed << std::setprecision(1) << v << " px";
            min_line_width_sp->set_text(oss.str());
            return true;
        });
        entry_set_tnum(*min_line_width_sp);
        min_line_width_sp->set_value(appearance.min_line_width);
        min_line_width_sp->signal_changed().connect([this, min_line_width_sp, &appearance] {
            appearance.min_line_width = min_line_width_sp->get_value();
            preferences->signal_changed().emit();
        });
    }

    {
        Gtk::ComboBoxText *msaa_combo;
        GET_WIDGET(msaa_combo);
        msaa_combo->append("0", "Off");
        for (int i = 1; i < 5; i *= 2) {
            msaa_combo->append(std::to_string(i), std::to_string(i) + "× MSAA");
        }
        msaa_combo->set_active_id(std::to_string(appearance.msaa));
        msaa_combo->signal_changed().connect([this, msaa_combo, &appearance] {
            int msaa = std::stoi(msaa_combo->get_active_id());
            appearance.msaa = msaa;
            preferences->signal_changed().emit();
        });
    }

    std::vector<ColorEditor *> ws;
    for (const auto &it : color_names) {
        bool add = !colors_layer.count(it.first) && !colors_non_layer.count(it.first); // in both
        if (layered) {
            add = add || colors_layer.count(it.first);
        }
        else {
            add = add || colors_non_layer.count(it.first);
        }
        if (add)
            ws.push_back(Gtk::manage(new ColorEditorPalette(appearance, prefs, it.first)));
    }

    if (layered) {
        for (const auto la : BoardLayers::get_layers()) {
            ws.push_back(Gtk::manage(new ColorEditorLayer(appearance, prefs, la)));
        }
    }
    else {
        ws.push_back(Gtk::manage(new ColorEditorLayer(appearance, prefs, 0, "Default layer")));
    }
    ws.push_back(Gtk::manage(new ColorEditorLayer(appearance, prefs, 10000, "Non-layer")));
    for (auto w : ws) {
        w->construct();
        w->show();
        canvas_colors_fb->add(*w);
    }
    canvas_colors_fb->select_child(*canvas_colors_fb->get_child_at_index(0));
}

void CanvasPreferencesEditor::handle_export()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    std::string filename;
    {

        GtkFileChooserNative *native = gtk_file_chooser_native_new("Save color scheme", GTK_WINDOW(top->gobj()),
                                                                   GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");
        auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
        chooser->set_do_overwrite_confirmation(true);
        chooser->set_current_name("colors.json");

        if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
            filename = chooser->get_filename();
            if (!endswith(filename, ".json"))
                filename += ".json";
        }
    }
    if (filename.size()) {
        while (1) {
            std::string error_str;
            try {
                auto j = canvas_preferences->serialize_colors();
                j["layered"] = is_layered;
                save_json_to_file(filename, j);
                break;
            }
            catch (const std::exception &e) {
                error_str = e.what();
            }
            catch (...) {
                error_str = "unknown error";
            }
            if (error_str.size()) {
                Gtk::MessageDialog dia(*top, "Export error", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_NONE);
                dia.set_secondary_text(error_str);
                dia.add_button("Cancel", Gtk::RESPONSE_CANCEL);
                dia.add_button("Retry", 1);
                if (dia.run() != 1)
                    break;
            }
        }
    }
}

void CanvasPreferencesEditor::handle_import()
{
    auto top = dynamic_cast<Gtk::Window *>(get_ancestor(GTK_TYPE_WINDOW));
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Open color scheme", GTK_WINDOW(top->gobj()),
                                                               GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
    auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Horizon color schemes");
    filter->add_pattern("*.json");
    chooser->add_filter(filter);

    if (gtk_native_dialog_run(GTK_NATIVE_DIALOG(native)) == GTK_RESPONSE_ACCEPT) {
        auto filename = chooser->get_filename();
        std::string error_str;
        try {
            auto j = load_json_from_file(filename);
            if (j.value("layered", false) != is_layered) {
                if (is_layered)
                    throw std::runtime_error("Can't load non-layer file into layer settings");
                else
                    throw std::runtime_error("Can't load layer file into non-layer settings");
            }
            load_colors(j);
        }
        catch (const std::exception &e) {
            error_str = e.what();
        }
        catch (...) {
            error_str = "unknown error";
        }
        if (error_str.size()) {
            Gtk::MessageDialog dia(*top, "Import error", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
            dia.set_secondary_text(error_str);
            dia.run();
        }
    }
}

void CanvasPreferencesEditor::handle_load_preset(unsigned int idx)
{
    load_colors(color_presets.at(idx));
}

void CanvasPreferencesEditor::handle_default()
{
    CanvasPreferences def;
    if (!is_layered)
        def.appearance.layer_colors[0] = {1, 1, 0};
    canvas_preferences->appearance.colors = def.appearance.colors;
    canvas_preferences->appearance.layer_colors = def.appearance.layer_colors;
    preferences->signal_changed().emit();
    update_color_chooser();
    queue_draw();
    canvas_colors_fb->queue_draw();
}

void CanvasPreferencesEditor::load_colors(const json &j)
{
    canvas_preferences->load_colors_from_json(j);
    preferences->signal_changed().emit();
    update_color_chooser();
    queue_draw();
    canvas_colors_fb->queue_draw();
}

void CanvasPreferencesEditor::update_color_chooser()
{
    auto sel = canvas_colors_fb->get_selected_children();
    if (sel.size() != 1)
        return;
    auto it = dynamic_cast<ColorEditor *>(sel.front()->get_child());
    color_chooser_conn.block();
    Gdk::RGBA rgba;
    color_chooser->set_rgba(rgba); // fixes things...
    auto c = it->get_color();
    rgba.set_rgba(c.r, c.g, c.b, 1);
    color_chooser->set_rgba(rgba);
    color_chooser_conn.unblock();
}

CanvasPreferencesEditor *CanvasPreferencesEditor::create(Preferences *prefs, CanvasPreferences *canvas_prefs,
                                                         bool layered)
{
    CanvasPreferencesEditor *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    std::vector<Glib::ustring> widgets = {"canvas_box",  "adjustment1", "adjustment2",      "adjustment3",
                                          "adjustment4", "adjustment7", "color_preset_menu"};
    x->add_from_resource("/org/horizon-eda/horizon/pool-prj-mgr/preferences.ui", widgets);
    x->get_widget_derived("canvas_box", w, prefs, canvas_prefs, layered);
    w->reference();
    return w;
}
} // namespace horizon
