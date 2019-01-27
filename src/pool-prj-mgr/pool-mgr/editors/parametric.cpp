#include "parametric.hpp"
#include <cmath>
#include <iomanip>
#include "util/gtk_util.hpp"

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
        auto attributes_list = pango_attr_list_new();
        auto attribute_font_features = pango_attr_font_features_new("tnum 1");
        pango_attr_list_insert(attributes_list, attribute_font_features);
        gtk_entry_set_attributes(GTK_ENTRY(gobj()), attributes_list);
        pango_attr_list_unref(attributes_list);

        signal_activate().connect(sigc::mem_fun(this, &ParamQuantityEditor::parse));
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
        value = std::stod(v);
        set_text(format_value(value));
    }

private:
    double value = 0;

    void parse()
    {
        auto v = parse_input(get_text());
        if (!std::isnan(v)) {
            value = v;
        }
        set_text(format_value(value));
    }

    std::string format_value(double v)
    {
        return column.format(v);
    }

    double parse_input(const std::string &inps)
    {
        const auto regex = Glib::Regex::create(
                "([+-]?)(?:(?:(\\d+)[\\.,]?(\\d*))|(?:[\\.,](\\d+)))(?:[eE]([-+]?)([0-9]+)|\\s*([a-zA-Zµ])|)");
        Glib::ustring inp(inps);
        Glib::MatchInfo ma;
        if (regex->match(inp, ma)) {
            auto sign = ma.fetch(1);
            auto intv = ma.fetch(2);
            auto fracv = ma.fetch(3);
            auto fracv2 = ma.fetch(4);
            auto exp_sign = ma.fetch(5);
            auto exp = ma.fetch(6);
            auto prefix = ma.fetch(7);

            double v = 0;
            if (intv.size()) {
                v = std::stoi(intv);
                if (fracv.size()) {
                    v += std::stoi(fracv) * std::pow(10, (int)fracv.size() * -1);
                }
            }
            else {
                v = std::stoi(fracv2) * std::pow(10, (int)fracv2.size() * -1);
            }

            if (exp.size()) {
                int iexp = std::stoi(exp);
                if (exp_sign == "-") {
                    iexp *= -1;
                }
                v *= std::pow(10, iexp);
            }
            else if (prefix.size()) {
                int prefix_exp = 0;
                if (prefix == "p")
                    prefix_exp = -12;
                else if (prefix == "n" || prefix == "N")
                    prefix_exp = -9;
                else if (prefix == "u" || prefix == "U" || prefix == "µ")
                    prefix_exp = -6;
                else if (prefix == "m")
                    prefix_exp = -3;
                else if (prefix == "k" || prefix == "K")
                    prefix_exp = 3;
                else if (prefix == "M")
                    prefix_exp = 6;
                else if (prefix == "G" || prefix == "g")
                    prefix_exp = 9;
                else if (prefix == "T" || prefix == "t")
                    prefix_exp = 12;
                v *= std::pow(10, prefix_exp);
            }
            if (sign == "-")
                v *= -1;

            return v;
        }

        return NAN;
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
            w->show();
            grid_attach_label_and_widget(this, col.display_name, w, top);
            editors[col.name] = e;
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
