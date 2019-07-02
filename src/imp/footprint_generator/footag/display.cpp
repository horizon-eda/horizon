#include <regex>
#include "display.hpp"
#include "widgets/preview_canvas.hpp"
#include "widgets/spin_button_dim.hpp"
#include "board/board_layers.hpp"
#include "logger/logger.hpp"

namespace horizon {
static const auto PLUSMINUS = std::string("\u00B1");
FootagDisplay::FootagDisplay(CorePackage *c, enum footag_type type) : core(c), autofit("Auto-fit"), ppkg(UUID::random())
{
    ctx = footag_open(type);
    if (!ctx) {
        return;
    }

    params = footag_get_param(ctx);

    canvas_package = Gtk::manage(new PreviewCanvas(*core->m_pool, true));
    canvas_package->set_size_request(400, 500);
    canvas_package->signal_resize().connect([this](int width, int height) {
        if (autofit.get_active()) {
            display();
        }
    });
    canvas_package->set_hexpand(true);
    canvas_package->set_vexpand(true);

    auto vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
    {
        auto la = Gtk::manage(new Gtk::Label());
        auto ti = footag_get_typeinfo(type);
        la->set_markup((std::string) "<big><b>" + ti->brief + "</b></big>");
        la->set_margin_top(4);
        vbox->add(*la);
        if (ti->desc) {
            la = Gtk::manage(new Gtk::Label(ti->desc));
            la->set_halign(Gtk::ALIGN_START);
            vbox->add(*la);
        }
    }
    {
        auto grid = Gtk::manage(new Gtk::Grid());
        grid->set_orientation(Gtk::Orientation::ORIENTATION_VERTICAL);
        grid->set_row_spacing(2);
        grid->set_column_spacing(2);
        grid->set_border_width(4);

        int i = 0;

        for (struct footag_param *p = params; p->id != FOOTAG_PARAM_DONE; p++) {
            if (p->id == FOOTAG_PARAM_IGNORE) {
                continue;
            }
            if (p->id == FOOTAG_PARAM_TOPIC) {
                auto sep = Gtk::manage(new Gtk::Separator());
                grid->attach(*sep, 0, i, 4, 1);
                i++;
                auto la = Gtk::manage(new Gtk::Label());
                la->set_markup("<b>" + std::string(p->name) + "</b>");
                la->set_halign(Gtk::ALIGN_START);
                grid->attach(*la, 0, i, 1, 1);
                i++;
                continue;
            }

            {
                const bool use_abbr = true;
                std::string name;
                if (use_abbr && (std::string)p->abbr != "-") {
                    name = (std::string)p->name + " (<b>" + p->abbr + "</b>)";
                }
                else {
                    name = p->name;
                }
                auto la = Gtk::manage(new Gtk::Label());
                la->set_markup(name);
                la->set_halign(Gtk::ALIGN_START);
                grid->attach(*la, 0, i, 1, 1);
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
                grid->attach(*rbox, 1, i, 1, 1);
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
                grid->attach(*wi, 1, i, 1, 1);
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
                grid->attach(*wi, 1, i, 1, 1);
            }
            else if (p->item.type == FOOTAG_DATA_ENUM) {
                struct footag_enum *e = &p->item.data.e;
                /* if few options then radio buttons, else combo */
                if (e->num <= 4) {
                    auto rbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
                    rbox->get_style_context()->add_class("linked");

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
                        rbox->pack_start(*wi, false, false, 0);
                    }
                    grid->attach(*rbox, 1, i, 3, 1);
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
                    grid->attach(*wi, 1, i, 1, 1);
                }
            }
            else if (p->item.type == FOOTAG_DATA_BITMASK) {
                struct footag_bitmask *m = &p->item.data.m;
                auto rbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
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
                    rbox->pack_start(*wi, false, false, 0);
                }
                grid->attach(*rbox, 1, i, 3, 1);
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
                grid->attach(*wi0, 1, i, 1, 1);
                grid->attach(*pm, 2, i, 1, 1);
                grid->attach(*wi1, 3, i, 1, 1);
            }
            i++;
        }
        {
            auto sep = Gtk::manage(new Gtk::Separator());
            grid->attach(*sep, 0, i, 4, 1);
        }

        vbox->add(*grid);
    }

    if (true) {
        auto frame = Gtk::manage(new Gtk::Frame("Hint"));
        frame->add(hintbox);
        frame->set_border_width(2);
        // frame->set_hexpand(true);
        frame->set_vexpand(true);
        vbox->add(*frame);
    }
    else {
        vbox->add(hintbox);
    }

    add(*vbox);
    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::Orientation::ORIENTATION_VERTICAL));
        add(*sep);
    }

    {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
        box->add(autofit);
        autofit.set_active(true);
        autofit.signal_clicked().connect([this] {
            if (autofit.get_active()) {
                display();
            }
        });

        box->add(*canvas_package);

        if (true) {
            auto frame = Gtk::manage(new Gtk::Frame("Reference"));
            frame->add(refbox);
            frame->set_border_width(2);
            // frame->set_hexpand(true);
            frame->set_vexpand(true);
            box->add(*frame);
        }

        add(*box);
    }
    calc_and_display();
}

/* FIXME: should use well_known_padstack */
static const horizon::UUID STACKLOOKUP[] = {
        [FOOTAG_PADSTACK_SMD_RECT] = "3846f4bf-7acf-403a-bc36-771ec675eac9",
        [FOOTAG_PADSTACK_SMD_RRECT] = "8e762581-e1b1-4fb4-81d3-7f8a1cabb97f",
        [FOOTAG_PADSTACK_SMD_CIRC] = "244b0e3c-02eb-4627-9469-47c3a379f814",
        [FOOTAG_PADSTACK_SMD_OBLONG] = "00000000-0000-0000-0000-000000000000",
        [FOOTAG_PADSTACK_SMD_DSHAPE] = "d2aad97e-e7b2-40f1-9ffe-2bc7e6df4c56",
        [FOOTAG_PADSTACK_TH_ROUND] = "296cf69b-9d53-45e4-aaab-4aedf4087d3a",
        [FOOTAG_PADSTACK_TH_ROUND_RPAD] = "982142aa-2883-4fd0-9b0b-37eb5a37dd35",
        [FOOTAG_PADSTACK_NONE] = "00000000-0000-0000-0000-000000000000",
};
static_assert(sizeof(STACKLOOKUP) / sizeof(STACKLOOKUP[0]) == FOOTAG_PADSTACK_NUM, "sizes dont match");

static const Padstack *getpadstack(Pool &pool, enum footag_padstack stack)
{
    auto uu = STACKLOOKUP[stack];
    auto ps = pool.get_padstack(uu);
    if (!ps) {
        Logger::get().log_warning("Padstack '" + (std::string)uu + "' not found in pool", Logger::Domain::IMP);
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
        auto ps = getpadstack(*core->m_pool, p->stack);
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
        auto str = std::string((std::string) "<b>" + s->ref.doc + ", " + s->ref.where + "</b>\n" + s->ref.what);
        /* overkill */
        str = std::regex_replace(str, std::regex("&"), "&amp;");
        if (s->ref.doc) {
            refbox.showit(str);
        }
    }
}

void FootagDisplay::display(void)
{
    canvas_package->clear();
    canvas_package->load(ppkg, autofit.get_active());
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
        canvas_package->set_layer_display(la.first, LayerDisplay(visible, ld));
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
    core->commit();
    return true;
}

bool FootagDisplay::isopen(void)
{
    return ctx != NULL;
}

void FootagDisplay::help(const struct footag_param *p)
{
    const char *str = NULL;
    str = footag_hint(ctx, p);
    if (!str) {
        str = p->name;
    }
    hintbox.showit(str);
}

FootagDisplay::~FootagDisplay()
{
    if (ctx) {
        footag_close(ctx);
    }
}
} // namespace horizon
