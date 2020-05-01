#include <regex>
#include "display.hpp"
#include "widgets/preview_canvas.hpp"
#include "widgets/spin_button_dim.hpp"
#include "board/board_layers.hpp"
#include "logger/logger.hpp"
#include "util/str_util.hpp"
#include "document/idocument_package.hpp"
#include "pool/pool.hpp"
#include "canvas/canvas_gl.hpp"

namespace horizon {
static const auto PLUSMINUS = std::string("\u00B1");

FootagDisplay *FootagDisplay::create(IDocumentPackage *c, enum footag_type type)
{
    FootagDisplay *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/footprint_generator/footag/footag.ui");
    x->get_widget_derived("box", w, c, type);
    w->reference();
    return w;
}

FootagDisplay::FootagDisplay(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IDocumentPackage *c,
                             enum footag_type type)
    : Gtk::Box(cobject), core(c), ppkg(UUID::random())
{
    ctx = footag_open(type);
    if (!ctx) {
        return;
    }

    params = footag_get_param(ctx);

    canvas_package = Gtk::manage(new PreviewCanvas(*core->get_pool(), true));
    canvas_package->set_size_request(400, 500);
    canvas_package->signal_size_allocate().connect([this](auto &alloc) {
        if (autofit->get_active() && !(alloc == old_alloc)) {
            this->display();
            old_alloc = alloc;
        }
    });
    canvas_package->set_hexpand(true);
    canvas_package->set_vexpand(true);

    x->get_widget("reference_title", reference_title);
    x->get_widget("reference_label", reference_label);
    x->get_widget("autofit", autofit);
    x->get_widget("hint_label", hint_label);
    autofit->signal_clicked().connect([this] {
        if (autofit->get_active()) {
            display();
        }
    });

    Gtk::Box *canvas_box;
    x->get_widget("canvas_box", canvas_box);
    canvas_package->show();
    canvas_box->pack_start(*canvas_package, true, true, 0);

    {
        Gtk::Label *la;
        x->get_widget("package_label", la);
        auto ti = footag_get_typeinfo(type);
        la->set_label(ti->brief);

        Gtk::Label *la_desc;
        x->get_widget("package_description", la_desc);
        if (ti->desc) {
            la_desc->set_label(ti->desc);
        }
        else {
            la_desc->hide();
            la_desc->set_no_show_all(true);
        }
    }

    Gtk::Box *content_box;
    x->get_widget("content_box", content_box);

    Gtk::Box *current_box = nullptr;
    Gtk::Grid *grid = nullptr;

    auto sg_name = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    auto sg_abbr = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    auto sg_switch = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

    unsigned int i = 0;
    for (struct footag_param *p = params; p->id != FOOTAG_PARAM_DONE; p++) {
        if (p->id == FOOTAG_PARAM_IGNORE) {
            continue;
        }
        if (p->id == FOOTAG_PARAM_TOPIC) {
            current_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5));
            {
                auto la = Gtk::manage(new Gtk::Label());
                la->set_markup("<b>" + std::string(p->name) + "</b>");
                la->set_halign(Gtk::ALIGN_START);
                current_box->pack_start(*la, false, false, 0);
                la->show();

                grid = Gtk::manage(new Gtk::Grid);
                grid->set_row_spacing(5);
                grid->set_column_spacing(5);
                grid->set_margin_start(5);
                current_box->pack_start(*grid, false, false, 0);
                grid->show();
                i = 0;
            }
            content_box->pack_start(*current_box, false, false, 0);
            continue;
        }

        {
            const bool use_abbr = true;
            auto la = Gtk::manage(new Gtk::Label());
            la->set_label(p->name);
            la->set_xalign(1);
            la->get_style_context()->add_class("dim-label");
            sg_name->add_widget(*la);
            grid->attach(*la, 0, i, 1, 1);

            auto la_abbr = Gtk::manage(new Gtk::Label());
            if (use_abbr && (std::string)p->abbr != "-") {
                la_abbr->set_markup("<b>" + std::string(p->abbr) + "</b>");
            }
            la_abbr->set_xalign(0);
            la_abbr->get_style_context()->add_class("dim-label");
            sg_abbr->add_widget(*la_abbr);
            grid->attach(*la_abbr, 1, i, 1, 1);
        }

        if (p->item.type == FOOTAG_DATA_BOOL) {
            auto bb = &p->item.data.b;
            auto wi = Gtk::manage(new Gtk::Switch());
            wi->property_active().signal_changed().connect([this, bb, wi] {
                *bb = wi->get_active();
                calc_and_display();
            });
            wi->set_active(*bb);
            wi->signal_grab_focus().connect([this, p] {
                /* NOTE: this does not work */
                help(p);
            });
            auto rbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
            rbox->add(*wi);
            grid->attach(*rbox, 2, i, 1, 1);
        }
        else if (p->item.type == FOOTAG_DATA_INTEGER) {
            struct footag_integer *ii = &p->item.data.i;
            auto wi = Gtk::manage(new Gtk::SpinButton());
            wi->set_numeric(true);
            wi->set_range(ii->min, ii->max);
            wi->set_increments(ii->step, ii->step);
            wi->set_snap_to_ticks(true);
            wi->set_value(ii->val);
            wi->signal_value_changed().connect([this, ii, wi] {
                ii->val = wi->get_value_as_int();
                calc_and_display();
            });
            wi->signal_grab_focus().connect([this, p] { help(p); });
            grid->attach(*wi, 2, i, 1, 1);
        }
        else if (p->item.type == FOOTAG_DATA_LENGTH) {
            auto wi = Gtk::manage(new SpinButtonDim());
            wi->set_range(0, 200_mm);
            wi->set_increments(0.05_mm, 0.1_mm);
            wi->set_value(p->item.data.l * 1_mm);
            wi->signal_value_changed().connect([this, p, wi] {
                p->item.data.l = wi->get_value() / 1_mm;
                calc_and_display();
            });
            wi->signal_grab_focus().connect([this, p] { help(p); });
            grid->attach(*wi, 2, i, 1, 1);
        }
        else if (p->item.type == FOOTAG_DATA_ENUM) {
            struct footag_enum *e = &p->item.data.e;
            /* if few options then radio buttons, else combo */
            if (e->num <= 4) {
                auto rbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
                rbox->get_style_context()->add_class("linked");
                rbox->set_halign(Gtk::ALIGN_START);
                auto first = Gtk::manage(new Gtk::RadioButton(e->strs[0]));
                Gtk::RadioButton *wi = nullptr;
                for (unsigned int j = 0; j < e->num; j++) {
                    if (j == 0) {
                        wi = first;
                    }
                    else {
                        wi = Gtk::manage(new Gtk::RadioButton(e->strs[j]));
                        wi->join_group(*first);
                    }
                    wi->set_mode(false);
                    wi->set_active(e->val == j);
                    wi->signal_toggled().connect([this, wi, e, j] {
                        if (wi->get_active()) {
                            e->val = j;
                            calc_and_display();
                        }
                    });
                    wi->signal_grab_focus().connect([this, p] { help(p); });
                    rbox->pack_start(*wi, true, true, 0);
                }
                grid->attach(*rbox, 2, i, 3, 1);
                sg_switch->add_widget(*rbox);
            }
            else {
                auto wi = Gtk::manage(new Gtk::ComboBoxText());
                for (unsigned int j = 0; j < e->num; j++) {
                    wi->append(e->strs[j]);
                }
                wi->set_active(e->val);
                wi->signal_changed().connect([this, e, wi] {
                    e->val = wi->get_active_row_number();
                    calc_and_display();
                });
                wi->signal_grab_focus().connect([this, p] {
                    /* NOTE: this does not work */
                    help(p);
                });
                grid->attach(*wi, 2, i, 1, 1);
                sg_switch->add_widget(*wi);
            }
        }
        else if (p->item.type == FOOTAG_DATA_BITMASK) {
            struct footag_bitmask *m = &p->item.data.m;
            auto rbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
            rbox->get_style_context()->add_class("linked");
            rbox->set_halign(Gtk::ALIGN_START);
            for (unsigned int j = 0; j < m->num; j++) {
                auto wi = Gtk::manage(new Gtk::ToggleButton(m->strs[j]));
                unsigned long themask = 1 << j;
                wi->set_active(m->val & themask);
                wi->signal_toggled().connect([this, wi, m, themask] {
                    if (wi->get_active()) {
                        m->val |= themask;
                    }
                    else {
                        m->val &= ~themask;
                    }
                    calc_and_display();
                });
                wi->signal_grab_focus().connect([this, p] { help(p); });
                rbox->pack_start(*wi, true, true, 0);
            }
            grid->attach(*rbox, 2, i, 3, 1);
            sg_switch->add_widget(*rbox);
        }
        else if (p->item.type == FOOTAG_DATA_TOL) {
            struct footol *t = &p->item.data.t;

            auto wi0 = Gtk::manage(new SpinButtonDim());
            auto wi1 = Gtk::manage(new SpinButtonDim());
            wi0->set_range(0, 200_mm);
            wi1->set_range(0, 200_mm);
            wi0->set_increments(0.05_mm, 0.1_mm);
            wi1->set_increments(0.05_mm, 0.1_mm);
            wi0->set_value(t->nom * 1.0_mm);
            wi1->set_value(t->max * 0.5_mm - t->min * 0.5_mm);

            /* button selects "nominal, plusminus" or "min, max" */
            auto pm = Gtk::manage(new Gtk::Button(PLUSMINUS));
            pm->signal_clicked().connect([t, pm, wi0, wi1] {
                if (pm->get_label() == "to") {
                    int64_t v0 = t->nom * 1.0_mm;
                    int64_t v1 = t->max * 0.5_mm - t->min * 0.5_mm;
                    pm->set_label(PLUSMINUS);
                    wi0->set_value(v0);
                    wi1->set_value(v1);
                }
                else {
                    pm->set_label("to");
                    wi0->set_value(t->min * 1_mm);
                    wi1->set_value(t->max * 1_mm);
                }
            });

            wi0->signal_value_changed().connect([this, t, pm, wi0, wi1] {
                if (pm->get_label() == "to") {
                    int64_t min = wi0->get_value_as_int();
                    int64_t max = t->max * 1_mm;
                    int64_t newmax = max;
                    if (max < min) {
                        newmax = min;
                    }
                    footol_minmax(t, 1.0 * min / 1_mm, 1.0 * newmax / 1_mm);
                    if (max != newmax) {
                        wi1->set_value(newmax);
                    }
                }
                else {
                    int64_t nom = wi0->get_value_as_int();
                    int64_t diff = t->max * 0.5_mm - t->min * 0.5_mm;
                    footol_pm(t, 1.0 * nom / 1_mm, 1.0 * diff / 1_mm);
                }
                calc_and_display();
            });

            wi1->signal_value_changed().connect([this, t, pm, wi0, wi1] {
                if (pm->get_label() == "to") {
                    int64_t min = t->min * 1_mm;
                    int64_t max = wi1->get_value_as_int();
                    int64_t newmin = min;
                    if (max < min) {
                        newmin = max;
                    }
                    footol_minmax(t, 1.0 * newmin / 1_mm, 1.0 * max / 1_mm);
                    if (min != newmin) {
                        wi0->set_value(newmin);
                    }
                }
                else {
                    int64_t nom = t->nom * 1_mm;
                    int64_t diff = wi1->get_value_as_int();
                    footol_pm(t, 1.0 * nom / 1_mm, 1.0 * diff / 1_mm);
                }
                calc_and_display();
            });
            wi0->signal_grab_focus().connect([this, p] { help(p); });
            wi1->signal_grab_focus().connect([this, p] { help(p); });
            pm->signal_grab_focus().connect([this, p] { help(p); });
            grid->attach(*wi0, 2, i, 1, 1);
            grid->attach(*pm, 3, i, 1, 1);
            grid->attach(*wi1, 4, i, 1, 1);
        }

        i++;
    }

    calc_and_display();
}

static const char *padstack_names[] = {
        [FOOTAG_PADSTACK_SMD_RECT] = "smd rectangular",     [FOOTAG_PADSTACK_SMD_RRECT] = "smd rectangular rounded",
        [FOOTAG_PADSTACK_SMD_CIRC] = "smd circular",        [FOOTAG_PADSTACK_SMD_OBLONG] = "",
        [FOOTAG_PADSTACK_SMD_DSHAPE] = "smd half-obround",  [FOOTAG_PADSTACK_TH_ROUND] = "th circular",
        [FOOTAG_PADSTACK_TH_ROUND_RPAD] = "th rectangular", [FOOTAG_PADSTACK_NONE] = "",
};

static_assert(sizeof(padstack_names) / sizeof(padstack_names[0]) == FOOTAG_PADSTACK_NUM, "sizes dont match");

static const Padstack *getpadstack(Pool &pool, enum footag_padstack stack)
{
    const std::string ps_name = padstack_names[stack];
    auto ps = pool.get_well_known_padstack(ps_name);
    if (!ps) {
        Logger::get().log_warning("Well-known padstack '" + ps_name + "' not found in pool", Logger::Domain::IMP);
    }
    return ps;
}

static void make_rlimit_rect(Polygon &poly, const struct footag_rlimit *r, enum BoardLayers::Layer layer)
{
    poly.vertices.clear();
    poly.append_vertex(Coordi(r->minx * 1_mm, -r->maxy * 1_mm));
    poly.append_vertex(Coordi(r->maxx * 1_mm, -r->maxy * 1_mm));
    poly.append_vertex(Coordi(r->maxx * 1_mm, -r->miny * 1_mm));
    poly.append_vertex(Coordi(r->minx * 1_mm, -r->miny * 1_mm));
    poly.layer = layer;
}

void FootagDisplay::calc(Package *pkg, const struct footag_spec *s)
{
    for (int i = 0; i < s->npads; i++) {
        auto uu = UUID::random();
        auto p = &s->pads[i];
        if (p->stack == FOOTAG_PADSTACK_NONE) {
            continue;
        }
        auto ps = getpadstack(*core->get_pool(), p->stack);
        if (!ps) {
            continue;
        }
        auto &pad = pkg->pads.emplace(uu, Pad(uu, ps)).first->second;
        pad.placement.shift.x = p->x * 1_mm;
        pad.placement.shift.y = p->y * 1_mm;
        pad.name = p->name;
        pad.placement.set_angle(p->angle);
        if (p->stack == FOOTAG_PADSTACK_SMD_CIRC || p->stack == FOOTAG_PADSTACK_TH_ROUND) {
            pad.parameter_set[ParameterID::PAD_DIAMETER] = p->diam * 1_mm;
        }
        else {
            pad.parameter_set[ParameterID::PAD_HEIGHT] = p->h * 1_mm;
            pad.parameter_set[ParameterID::PAD_WIDTH] = p->w * 1_mm;
            if (p->stack == FOOTAG_PADSTACK_SMD_RRECT) {
                pad.parameter_set[ParameterID::CORNER_RADIUS] = p->param * 1_mm;
            }
        }
        if (p->stack == FOOTAG_PADSTACK_TH_ROUND || p->stack == FOOTAG_PADSTACK_TH_ROUND_RPAD) {
            pad.parameter_set[ParameterID::HOLE_DIAMETER] = p->holediam * 1_mm;
        }
    }

    {
        auto uu = UUID::random();
        auto &poly = pkg->polygons.emplace(uu, uu).first->second;
        make_rlimit_rect(poly, &s->body, BoardLayers::TOP_PACKAGE);
    }

    {
        auto uu = UUID::random();
        auto &poly = pkg->polygons.emplace(uu, uu).first->second;
        make_rlimit_rect(poly, &s->courtyard, BoardLayers::TOP_COURTYARD);
        poly.parameter_class = "courtyard";
    }
}

void FootagDisplay::calc_and_display(void)
{
    ppkg = Package(UUID::random());
    auto s = footag_get_spec(ctx);
    if (!s) {
        display();
        return;
    }

    calc(&ppkg, s);

    display();
    if (s->ref.doc) {
        reference_title->set_label(std::string("as per ") + s->ref.doc + ", " + s->ref.where);
        reference_label->set_label(s->ref.what);
    }
}

void FootagDisplay::display(void)
{
    canvas_package->load(ppkg, autofit->get_active());
    for (const auto &la : ppkg.get_layers()) {
        auto ld = LayerDisplay::Mode::FILL_ONLY;
        auto visible = false;
        if (la.second.copper) {
            visible = true;
        }
        if (la.first == BoardLayers::TOP_PACKAGE) {
            visible = true;
        }
        if (la.first == BoardLayers::TOP_COURTYARD) {
            visible = true;
            ld = LayerDisplay::Mode::OUTLINE;
        }
        canvas_package->get_canvas().set_layer_display(la.first, LayerDisplay(visible, ld));
    }
}

bool FootagDisplay::generate(void)
{
    auto pkg = core->get_package();
    auto s = footag_get_spec(ctx);
    if (!s) {
        return false;
    }

    calc(pkg, s);
    return true;
}

bool FootagDisplay::isopen(void)
{
    return ctx != NULL;
}

void FootagDisplay::help(const struct footag_param *p)
{

    std::string str;
    str = footag_hint(ctx, p);
    if (!str.size()) {
        str = "<big>" + std::string(p->name) + "</big>";
    }
    trim(str);
    hint_label->set_markup(str);
}

FootagDisplay::~FootagDisplay()
{
    if (ctx) {
        footag_close(ctx);
    }
}
} // namespace horizon
