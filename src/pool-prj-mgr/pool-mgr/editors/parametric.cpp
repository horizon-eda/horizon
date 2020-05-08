#include "parametric.hpp"
#include <cmath>
#include <iomanip>
#include "util/gtk_util.hpp"
#include "util/util.hpp"

namespace horizon {


class ParametricParamEditor {
public:
    ParametricParamEditor(const PoolParametric::Column &c) : column(c)
    {
    }
    virtual std::string get_value() = 0;
    virtual void set_value(const std::string &value) = 0;
    virtual ~ParametricParamEditor()
    {
    }

protected:
    const PoolParametric::Column &column;
};

class ParamQuantityEditor : public Gtk::Entry, public ParametricParamEditor {
public:
    ParamQuantityEditor(const PoolParametric::Column &c) : Gtk::Entry(), ParametricParamEditor(c)
    {
        entry_set_tnum(*this);

        signal_activate().connect(sigc::mem_fun(*this, &ParamQuantityEditor::parse));
        signal_focus_out_event().connect([this](GdkEventFocus *ev) {
            parse();
            return false;
        });
    }
    std::string get_value() override
    {
        std::ostringstream ss;
        ss << value;
        return ss.str();
    }
    void set_value(const std::string &v) override
    {
        std::istringstream istr(v);
        istr.imbue(std::locale("C"));
        istr >> value;
        set_text(format_value(value));
    }

private:
    double value = 0;

    void parse()
    {
        auto v = parse_si(get_text());
        if (!std::isnan(v)) {
            value = v;
        }
        set_text(format_value(value));
    }

    std::string format_value(double v)
    {
        return column.format(v);
    }
};


class ParamEnumEditor : public Gtk::ComboBoxText, public ParametricParamEditor {
public:
    ParamEnumEditor(const PoolParametric::Column &c) : Gtk::ComboBoxText(), ParametricParamEditor(c)
    {
        for (const auto &it : column.enum_items)
            append(it);
    }
    std::string get_value() override
    {
        return get_active_text();
    }
    void set_value(const std::string &v) override
    {
        set_active_text(v);
    }
};

class NullableParamEditor : public Gtk::Box, public ParametricParamEditor {
public:
    NullableParamEditor(Gtk::Widget *w, ParametricParamEditor *e, const PoolParametric::Column &c)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0), ParametricParamEditor(c), widget(w), editor(e)
    {
        get_style_context()->add_class("linked");
        pack_start(*widget, true, true, 0);
        widget->show();

        nullbutton = Gtk::manage(new Gtk::ToggleButton("N/A"));
        pack_start(*nullbutton, false, false, 0);
        nullbutton->show();
        nullbutton->set_sensitive(!column.required);

        if (!column.required)
            nullbutton->signal_toggled().connect([this] { widget->set_sensitive(!nullbutton->get_active()); });
    }

    std::string get_value() override
    {
        if (!column.required && nullbutton->get_active())
            return "";
        else
            return editor->get_value();
    }
    void set_value(const std::string &value) override
    {
        if (value.size() || column.required)
            editor->set_value(value);

        nullbutton->set_active(!column.required && (value.size() == 0));
    }

private:
    Gtk::Widget *widget;
    Gtk::ToggleButton *nullbutton = nullptr;
    ParametricParamEditor *editor;
};


ParametricEditor::ParametricEditor(PoolParametric *p, const std::string &t)
    : Gtk::Grid(), pool(p), table(pool->get_tables().at(t))
{
    set_row_spacing(10);
    set_column_spacing(10);
    int top = 0;
    for (auto &col : table.columns) {
        Gtk::Widget *w = nullptr;
        ParametricParamEditor *e = nullptr;
        switch (col.type) {
        case PoolParametric::Column::Type::QUANTITY: {
            auto x = Gtk::manage(new ParamQuantityEditor(col));
            x->signal_changed().connect([this] { s_signal_changed.emit(); });
            w = x;
            e = x;
        } break;
        case PoolParametric::Column::Type::ENUM: {
            auto x = Gtk::manage(new ParamEnumEditor(col));
            x->signal_changed().connect([this] { s_signal_changed.emit(); });
            w = x;
            e = x;
        } break;
        case PoolParametric::Column::Type::STRING:;
        }
        if (w && e) {
            auto ne = Gtk::manage(new NullableParamEditor(w, e, col));
            ne->show();
            grid_attach_label_and_widget(this, col.display_name, ne, top);
            editors[col.name] = ne;
        }
    }
}

void ParametricEditor::update(const std::map<std::string, std::string> &params)
{
    for (auto &it : params) {
        if (editors.count(it.first))
            editors.at(it.first)->set_value(it.second);
    }
}

std::map<std::string, std::string> ParametricEditor::get_values()
{
    std::map<std::string, std::string> r;
    r.emplace("table", table.name);
    for (auto &it : editors) {
        r.emplace(it.first, it.second->get_value());
    }
    return r;
}

}; // namespace horizon
