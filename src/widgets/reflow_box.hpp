#include <gtkmm/container.h>
#include <glibmm/property.h>

namespace horizon {
class ReflowBox : public Gtk::Container {
public:
    ReflowBox();
    virtual ~ReflowBox();

    void append(Gtk::Widget &child);

    Glib::PropertyProxy<int> property_spacing()
    {
        return p_property_spacing.get_proxy();
    }
    Glib::PropertyProxy<int> property_row_spacing()
    {
        return p_property_row_spacing.get_proxy();
    }

protected:
    // Overrides:
    Gtk::SizeRequestMode get_request_mode_vfunc() const override;
    void get_preferred_width_vfunc(int &minimum_width, int &natural_width) const override;
    void get_preferred_height_for_width_vfunc(int width, int &minimum_height, int &natural_height) const override;
    void get_preferred_height_vfunc(int &minimum_height, int &natural_height) const override;
    void get_preferred_width_for_height_vfunc(int height, int &minimum_width, int &natural_width) const override;
    void on_size_allocate(Gtk::Allocation &allocation) override;

    void forall_vfunc(gboolean include_internals, GtkCallback callback, gpointer callback_data) override;

    void on_add(Gtk::Widget *child) override;
    void on_remove(Gtk::Widget *child) override;
    GType child_type_vfunc() const override;

    Glib::Property<int> p_property_spacing;
    Glib::Property<int> p_property_row_spacing;

    std::vector<Gtk::Widget *> m_children;
};

} // namespace horizon
