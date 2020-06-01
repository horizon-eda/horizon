#pragma once
#include <gtkmm.h>
#include "core/core.hpp"
#include "common/object_descr.hpp"
#include "core/core_properties.hpp"
#include "widgets/generic_combo_box.hpp"

namespace horizon {
class PropertyEditor : public Gtk::Box {
public:
    PropertyEditor(ObjectType t, ObjectProperty::ID prop, class PropertyPanel *p);
    virtual void construct();
    void set_can_apply_all(bool v);

    virtual void reload(){};
    virtual PropertyValue &get_value()
    {
        return dummy;
    }
    virtual PropertyMeta &get_meta()
    {
        return meta;
    }

    typedef sigc::signal<void> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return s_signal_changed;
    }

    type_signal_changed signal_apply_all()
    {
        return s_signal_apply_all;
    }

    type_signal_changed signal_activate()
    {
        return s_signal_activate;
    }

    bool get_apply_all();

    virtual ~PropertyEditor()
    {
    }

    class PropertyPanel *parent;
    const ObjectProperty::ID property_id;

protected:
    const ObjectType type;

    const ObjectProperty &property;
    Gtk::ToggleButton *apply_all_button = nullptr;

    virtual Gtk::Widget *create_editor();

    type_signal_changed s_signal_changed;
    type_signal_changed s_signal_apply_all;
    type_signal_changed s_signal_activate;
    PropertyValue dummy;
    PropertyMeta meta;

    bool readonly = false;

    std::deque<sigc::connection> connections;

private:
};

class PropertyEditorBool : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;

protected:
    Gtk::Widget *create_editor() override;

private:
    Gtk::Switch *sw = nullptr;
    PropertyValueBool value;
};

class PropertyEditorString : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;

protected:
    Gtk::Widget *create_editor() override;

private:
    Gtk::Entry *en = nullptr;
    void changed();
    void activate();
    bool focus_out_event(GdkEventFocus *e);
    bool modified = false;

    PropertyValueString value;
};

class PropertyEditorDim : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;
    void set_range(int64_t min, int64_t max);

protected:
    Gtk::Widget *create_editor() override;

private:
    class SpinButtonDim *sp = nullptr;
    PropertyValueInt value;
    std::pair<int64_t, int64_t> range = {-1e9, 1e9};
};

class PropertyEditorEnum : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;

protected:
    Gtk::Widget *create_editor() override;

private:
    GenericComboBox<int> *combo = nullptr;
    void changed();
    PropertyValueInt value;
};

class PropertyEditorStringRO : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;

protected:
    Gtk::Widget *create_editor() override;

private:
    Gtk::Label *la = nullptr;
    PropertyValueString value;
};

class PropertyEditorNetClass : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;
    PropertyMeta &get_meta() override
    {
        return my_meta;
    };

protected:
    Gtk::Widget *create_editor() override;

private:
    GenericComboBox<UUID> *combo = nullptr;
    void changed();
    PropertyValueUUID value;
    PropertyMetaNetClasses my_meta;
};

class PropertyEditorLayer : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;
    PropertyMeta &get_meta() override
    {
        return my_meta;
    };
    bool copper_only = false;

protected:
    Gtk::Widget *create_editor() override;

private:
    Gtk::ComboBoxText *combo = nullptr;
    void changed();
    PropertyValueInt value;
    PropertyMetaLayers my_meta;
};

class PropertyEditorAngle : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;

protected:
    Gtk::Widget *create_editor() override;

private:
    Gtk::SpinButton *sp = nullptr;
    PropertyValueInt value;
};

class PropertyEditorStringMultiline : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;
    void construct() override;

protected:
    Gtk::Widget *create_editor() override;

private:
    Gtk::TextView *en = nullptr;
    void changed();
    void activate();
    bool focus_out_event(GdkEventFocus *e);
    bool modified = false;

    PropertyValueString value;
};

class PropertyEditorInt : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;

protected:
    Gtk::Widget *create_editor() override;

    Gtk::SpinButton *sp = nullptr;
    PropertyValueInt value;
    void changed();
};

class PropertyEditorExpand : public PropertyEditorInt {
    using PropertyEditorInt::PropertyEditorInt;

protected:
    Gtk::Widget *create_editor() override;
};

class PropertyEditorDouble : public PropertyEditor {
    using PropertyEditor::PropertyEditor;

public:
    void reload() override;
    PropertyValue &get_value() override;

protected:
    Gtk::Widget *create_editor() override;

    Gtk::SpinButton *sp = nullptr;
    PropertyValueDouble value;
    void changed();
};

class PropertyEditorOpacity : public PropertyEditorDouble {
    using PropertyEditorDouble::PropertyEditorDouble;

protected:
    Gtk::Widget *create_editor() override;
};

} // namespace horizon
