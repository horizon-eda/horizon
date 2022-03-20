#include <algorithm> // std::max
#include "reflow_box.hpp"

namespace horizon {
ReflowBox::ReflowBox()
    : Glib::ObjectBase(typeid(ReflowBox)), p_property_spacing(*this, "spacing", 0),
      p_property_row_spacing(*this, "row-spacing", 0)
{
    set_has_window(false);
    set_redraw_on_allocate(false);
    property_spacing().signal_changed().connect([this] { queue_resize(); });
    property_row_spacing().signal_changed().connect([this] { queue_resize(); });
}

ReflowBox::~ReflowBox() = default;

void ReflowBox::append(Gtk::Widget &child)
{
    m_children.push_back(&child);
    child.set_parent(*this);
}

Gtk::SizeRequestMode ReflowBox::get_request_mode_vfunc() const
{
    return Gtk::SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

// Discover the total amount of minimum space and natural space needed by
// this container and its children.
void ReflowBox::get_preferred_width_vfunc(int &minimum_width, int &natural_width) const
{
    const int border_width = get_border_width();
    minimum_width = 0;
    natural_width = 0;
    const int spacing = p_property_spacing.get_value();
    bool first_child = true;
    for (auto ch : m_children) {
        if (!ch->get_visible())
            continue;
        int min, nat;
        ch->get_preferred_width(min, nat);
        minimum_width = std::max(minimum_width, min);
        natural_width += nat;
        if (!first_child) {
            minimum_width += spacing;
            natural_width += spacing;
        }
        first_child = false;
    }
    minimum_width += 2 * border_width;
    natural_width += 2 * border_width;
}

void ReflowBox::get_preferred_height_for_width_vfunc(int width, int &minimum_height, int &natural_height) const
{
    const int border_width = get_border_width();
    const int spacing = p_property_spacing.get_value();
    const int row_spacing = p_property_row_spacing.get_value();

    int row_height = 0;
    int n_rows = 1;
    int w = 0;
    width = std::max(0, width - (2 * border_width));
    for (auto ch : m_children) {
        if (!ch->get_visible())
            continue;
        int minw, natw;
        ch->get_preferred_width(minw, natw);
        int minh, nath;
        ch->get_preferred_height(minh, nath);
        row_height = std::max(row_height, nath);
        if (w != 0) // not first child in row, add spacing
            w += spacing;
        w += natw;
        if (w > width) {
            n_rows++;
            w = 0;
        }
    }
    minimum_height = n_rows * row_height + (2 * border_width) + (n_rows - 1) * row_spacing;
    natural_height = minimum_height;
}

void ReflowBox::get_preferred_height_vfunc(int &minimum_height, int &natural_height) const
{
    const int border_width = get_border_width();

    minimum_height = 0;
    natural_height = 0;
    for (auto ch : m_children) {
        if (!ch->get_visible())
            continue;
        int min, nat;
        ch->get_preferred_height(min, nat);
        minimum_height = std::max(min, minimum_height);
        natural_height = std::max(nat, natural_height);
    }
    minimum_height += 2 * border_width;
    natural_height += 2 * border_width;
}

void ReflowBox::get_preferred_width_for_height_vfunc(int height, int &minimum_width, int &natural_width) const
{
    get_preferred_width_vfunc(minimum_width, natural_width);
}

void ReflowBox::on_size_allocate(Gtk::Allocation &allocation)
{
    // Do something with the space that we have actually been given:
    //(We will not be given heights or widths less than we have requested, though
    // we might get more.)

    // Use the offered allocation for this container:
    set_allocation(allocation);

    int row_height = 0;
    for (auto ch : m_children) {
        int minh, nath;
        ch->get_preferred_height(minh, nath);
        row_height = std::max(row_height, nath);
    }
    const int border_width = get_border_width();
    const int spacing = p_property_spacing.get_value();
    const int row_spacing = p_property_row_spacing.get_value();
    int w = 0;
    const int width = allocation.get_width() - (2 * border_width);
    int row = 0;
    for (auto ch : m_children) {
        if (!ch->get_visible())
            continue;
        int minw, natw;
        ch->get_preferred_width(minw, natw);
        Gtk::Allocation alloc;
        alloc.set_height(row_height);
        alloc.set_width(natw);
        if ((w + natw) > width) {
            row++;
            w = 0;
        }
        alloc.set_x(allocation.get_x() + w + border_width);
        alloc.set_y(allocation.get_y() + row * (row_height + row_spacing) + border_width);
        w += natw + spacing;
        ch->size_allocate(alloc);
    }
}

void ReflowBox::forall_vfunc(gboolean, GtkCallback callback, gpointer callback_data)
{
    for (auto ch : m_children) {
        callback(ch->gobj(), callback_data);
    }
}

void ReflowBox::on_add(Gtk::Widget *child)
{
    if (child)
        append(*child);
}

void ReflowBox::on_remove(Gtk::Widget *child)
{
    if (child) {
        const bool visible = child->get_visible();

        auto it = std::find(m_children.begin(), m_children.end(), child);

        if (it != m_children.end()) {
            m_children.erase(it);
            child->unparent();

            if (visible)
                queue_resize();
        }
    }
}

GType ReflowBox::child_type_vfunc() const
{
    return Gtk::Widget::get_type();
}
} // namespace horizon
