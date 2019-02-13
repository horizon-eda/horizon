#pragma once
#include "canvas/selectables.hpp"
#include "canvas/target.hpp"
#include "common/layer.hpp"
#include "common/object_descr.hpp"
#include "common/keepout.hpp"
#include "cores.hpp"
#include "tool_data.hpp"
#include "dialogs/dialogs.hpp"
#include "nlohmann/json_fwd.hpp"
#include "pool/pool.hpp"
#include "pool/symbol.hpp"
#include <gdk/gdkkeysyms.h>
#include <iostream>
#include <memory>
#include <sigc++/sigc++.h>
#include "tool_id.hpp"

namespace horizon {
enum class ToolEventType { NONE, MOVE, CLICK, CLICK_RELEASE, KEY, LAYER_CHANGE, DATA };


/**
 * This is what a Tool receives when the user did something.
 * i.e. moved the cursor or pressed key
 */
class ToolArgs {
public:
    ToolEventType type = ToolEventType::NONE;
    Coordi coords;
    std::set<SelectableRef> selection;
    bool keep_selection = false;
    unsigned int button = 0;
    unsigned int key = 0;
    enum Modifieres {
        MOD_FINE = (1 << 0),
        MOD_ALT = (1 << 1),
        MOD_CTRL = (1 << 2),
    };
    unsigned int mod = 0;

    Target target;
    int work_layer = 0;
    std::unique_ptr<ToolData> data = nullptr;
    ToolArgs()
    {
    }
};

/**
 * To signal back to the core what the Tool did, a Tool returns a ToolResponse.
 */
class ToolResponse {
public:
    ToolID next_tool = ToolID::NONE;
    bool end_tool = false;
    int layer = 10000;
    bool fast_draw = false;

    ToolResponse()
    {
    }
    /**
     * Use this if you're done. The Core will then delete the active tool and
     * initiate a rebuild.
     */
    static ToolResponse end()
    {
        ToolResponse r;
        r.end_tool = true;
        return r;
    }

    static ToolResponse fast()
    {
        ToolResponse r;
        r.fast_draw = true;
        return r;
    }

    /**
     * Use this for changing the work layer from a Tool.
     */
    static ToolResponse change_layer(int l)
    {
        ToolResponse r;
        r.layer = l;
        return r;
    }

    /**
     * If you want another Tool to be launched you've finished, use this one.
     */
    static ToolResponse next(ToolID t)
    {
        ToolResponse r;
        r.end_tool = true;
        r.next_tool = t;
        return r;
    };
};

class ToolSettings {
public:
    virtual void load_from_json(const json &j) = 0;
    virtual json serialize() const = 0;
    virtual ~ToolSettings()
    {
    }
};

class ToolSettingsProxy {
public:
    ToolSettingsProxy(class ToolBase *t, ToolSettings *s) : tool(t), settings(s)
    {
    }
    ToolSettings &operator*()
    {
        return *settings;
    }
    operator ToolSettings *()
    {
        return settings;
    }
    ToolSettings *operator->()
    {
        return settings;
    }
    ~ToolSettingsProxy();

private:
    ToolBase *tool;
    ToolSettings *settings;
};


/**
 * Common interface for all Tools
 */
class ToolBase {
public:
    ToolBase(class Core *c, ToolID tid);
    void set_imp_interface(class ImpInterface *i);
    void set_transient();
    virtual ToolID get_tool_id_for_settings() const
    {
        return tool_id;
    }
    virtual const ToolSettings *get_settings_const() const
    {
        return nullptr;
    }
    ToolSettingsProxy get_settings_proxy()
    {
        return ToolSettingsProxy(this, get_settings());
    }
    virtual void apply_settings()
    {
    }


    /**
     * Gets called right after the constructor has finished.
     * Used to get the initial placement right and set things up.
     * For non-interactive Tools (e.g. DELETE), this one may return
     * ToolResponse::end()
     */
    virtual ToolResponse begin(const ToolArgs &args) = 0;

    /**
     * Gets called whenever the user generated some sort of input.
     */
    virtual ToolResponse update(const ToolArgs &args) = 0;

    /**
     * @returns true if this Tool can begin in sensible way
     */
    virtual bool can_begin()
    {
        return false;
    }

    /**
     * @returns true if this Tool is specific to the selection
     */
    virtual bool is_specific()
    {
        return false;
    }

    /**
     * @returns true if this Tool can handle esc by itself
     */
    virtual bool handles_esc()
    {
        return false;
    }

    virtual ~ToolBase()
    {
    }

protected:
    Cores core;
    class ImpInterface *imp = nullptr;
    ToolID tool_id = ToolID::NONE;
    bool is_transient = false;
    virtual ToolSettings *get_settings()
    {
        return nullptr;
    }
};

/**
 * Where Tools and and documents meet.
 * The Core provides a unified interface for Tools to access
 * the objects common to all documents (whatever is being edited).
 * It also provides the property interface for the property editor.
 *
 * A Core always stores to copies of the document, one of which is
 * the working copy. Tools always operate on this one. Tools use Core::commit()
 * to
 * commit their changes by replacing the non-working document with the working
 * document.
 * Core::revert() does the opposite thing by replacing the working document with
 * the non-working document, thereby discarding the changes made to the working
 * copy.
 * Usually, calling Core::commit() or Core::revert() is the last thing a Tool
 * does before
 * finishing.
 *
 * After a Tool has finished its work by returning ToolResponse::end(), the Core
 * will
 * initiate a rebuild. For CoreSchematic, a rebuild will update the Schematic
 * according to its Block.
 *
 * The Core also handles undo/redo by storing a full copy for each step.
 */
class Core {
public:
    virtual bool has_object_type(ObjectType ty) const
    {
        return false;
    }

    virtual class Junction *insert_junction(const UUID &uu, bool work = true);
    virtual class Junction *get_junction(const UUID &uu, bool work = true);
    virtual void delete_junction(const UUID &uu, bool work = true);

    virtual class Line *insert_line(const UUID &uu, bool work = true);
    virtual class Line *get_line(const UUID &uu, bool work = true);
    virtual void delete_line(const UUID &uu, bool work = true);

    virtual class Arc *insert_arc(const UUID &uu, bool work = true);
    virtual class Arc *get_arc(const UUID &uu, bool work = true);
    virtual void delete_arc(const UUID &uu, bool work = true);

    virtual class Text *insert_text(const UUID &uu, bool work = true);
    virtual class Text *get_text(const UUID &uu, bool work = true);
    virtual void delete_text(const UUID &uu, bool work = true);

    virtual class Polygon *insert_polygon(const UUID &uu, bool work = true);
    virtual class Polygon *get_polygon(const UUID &uu, bool work = true);
    virtual void delete_polygon(const UUID &uu, bool work = true);

    virtual class Hole *insert_hole(const UUID &uu, bool work = true);
    virtual class Hole *get_hole(const UUID &uu, bool work = true);
    virtual void delete_hole(const UUID &uu, bool work = true);

    virtual class Dimension *insert_dimension(const UUID &uu);
    virtual class Dimension *get_dimension(const UUID &uu);
    virtual void delete_dimension(const UUID &uu);

    virtual class Keepout *insert_keepout(const UUID &uu);
    virtual class Keepout *get_keepout(const UUID &uu);
    virtual void delete_keepout(const UUID &uu);

    virtual std::vector<Line *> get_lines(bool work = true);
    virtual std::vector<Arc *> get_arcs(bool work = true);
    virtual std::vector<Keepout *> get_keepouts();

    virtual class Block *get_block(bool work = true)
    {
        return nullptr;
    }

    /**
     * Expands the non-working document.
     * And copies the non-working document to the working document.
     */
    virtual void rebuild(bool from_undo = false);
    ToolResponse tool_begin(ToolID tool_id, const ToolArgs &args, class ImpInterface *imp, bool transient = false);
    ToolResponse tool_update(const ToolArgs &args);
    std::pair<bool, bool> tool_can_begin(ToolID tool_id, const std::set<SelectableRef> &selection);
    bool tool_handles_esc();
    virtual void commit() = 0;
    virtual void revert() = 0;
    virtual void save() = 0;
    void undo();
    void redo();

    bool can_undo() const;
    bool can_redo() const;

    inline bool tool_is_active()
    {
        return tool != nullptr;
    }

    virtual bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                              const class PropertyValue &value);
    virtual bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyValue &value);
    virtual bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                                   class PropertyMeta &meta);

    virtual std::string get_display_name(ObjectType type, const UUID &uu);
    virtual std::string get_display_name(ObjectType type, const UUID &uu, const UUID &sheet);

    void set_property_begin();
    void set_property_commit();
    bool get_property_transaction() const;

    virtual class LayerProvider *get_layer_provider()
    {
        return nullptr;
    };

    /**
     * @returns the current document's meta information.
     * Meta information contains grid spacing and layer setup.
     */
    virtual json get_meta();

    virtual class Rules *get_rules()
    {
        return nullptr;
    }
    virtual void update_rules()
    {
    }

    virtual bool can_search_for_object_type(ObjectType type) const
    {
        return false;
    };

    class SearchQuery {
    public:
        std::string query;
        std::set<ObjectType> types;
        std::pair<Coordf, Coordf> area_visible;
    };

    class SearchResult {
    public:
        SearchResult(ObjectType ty, const UUID &uu) : type(ty), uuid(uu)
        {
        }
        ObjectType type;
        UUID uuid;
        Coordi location;
        UUID sheet;
        bool selectable = false;
    };

    virtual std::list<SearchResult> search(const SearchQuery &q)
    {
        return {};
    };

    virtual std::pair<Coordi, Coordi> get_bbox() = 0;

    virtual ~Core()
    {
    }
    std::set<SelectableRef> selection;
    Pool *m_pool;

    bool get_needs_save() const;
    void set_needs_save();

    typedef sigc::signal<void, ToolID> type_signal_tool_changed;
    type_signal_tool_changed signal_tool_changed()
    {
        return s_signal_tool_changed;
    }
    typedef sigc::signal<void> type_signal_rebuilt;
    type_signal_rebuilt signal_rebuilt()
    {
        return s_signal_rebuilt;
    }
    /**
     * Gets emitted right before saving. Gives the Imp an opportunity to write
     * additional
     * information to the document.
     */
    type_signal_rebuilt signal_save()
    {
        return s_signal_save;
    }

    type_signal_rebuilt signal_can_undo_redo()
    {
        return s_signal_can_undo_redo;
    }

    typedef sigc::signal<json> type_signal_request_save_meta;
    /**
     * connect to this signal for providing meta information when the document
     * is saved
     */
    type_signal_request_save_meta signal_request_save_meta()
    {
        return s_signal_request_save_meta;
    }

    typedef sigc::signal<void, bool> type_signal_needs_save;
    type_signal_needs_save signal_needs_save()
    {
        return s_signal_needs_save;
    }

    typedef sigc::signal<json, ToolID> type_signal_load_tool_settings;
    type_signal_load_tool_settings signal_load_tool_settings()
    {
        return s_signal_load_tool_settings;
    }

    typedef sigc::signal<void, ToolID, json> type_signal_save_tool_settings;
    type_signal_save_tool_settings signal_save_tool_settings()
    {
        return s_signal_save_tool_settings;
    }

    virtual void reload_pool()
    {
    }

protected:
    virtual std::map<UUID, Junction> *get_junction_map(bool work = true)
    {
        return nullptr;
    }
    virtual std::map<UUID, Line> *get_line_map(bool work = true)
    {
        return nullptr;
    }
    virtual std::map<UUID, Arc> *get_arc_map(bool work = true)
    {
        return nullptr;
    }
    virtual std::map<UUID, Text> *get_text_map(bool work = true)
    {
        return nullptr;
    }
    virtual std::map<UUID, Polygon> *get_polygon_map(bool work = true)
    {
        return nullptr;
    }
    virtual std::map<UUID, Hole> *get_hole_map(bool work = true)
    {
        return nullptr;
    }
    virtual std::map<UUID, Dimension> *get_dimension_map()
    {
        return nullptr;
    }
    virtual std::map<UUID, Keepout> *get_keepout_map()
    {
        return nullptr;
    }

    bool reverted = false;
    std::unique_ptr<ToolBase> tool = nullptr;
    type_signal_tool_changed s_signal_tool_changed;
    type_signal_rebuilt s_signal_rebuilt;
    type_signal_rebuilt s_signal_save;
    type_signal_rebuilt s_signal_can_undo_redo;
    type_signal_request_save_meta s_signal_request_save_meta;
    type_signal_needs_save s_signal_needs_save;
    type_signal_load_tool_settings s_signal_load_tool_settings;
    type_signal_save_tool_settings s_signal_save_tool_settings;
    bool needs_save = false;
    void set_needs_save(bool v);

    class HistoryItem {
    public:
        // Symbol sym;
        // HistoryItem(const Symbol &s): sym(s) {}
        std::string comment;
        virtual ~HistoryItem()
        {
        }
    };
    std::deque<std::unique_ptr<HistoryItem>> history;
    int history_current = -1;
    virtual void history_push() = 0;
    virtual void history_load(unsigned int i) = 0;
    void history_clear();

    bool property_transaction = false;

    void layers_to_meta(class PropertyMeta &meta);
    void get_placement(const Placement &placement, class PropertyValue &value, ObjectProperty::ID property);
    void set_placement(Placement &placement, const class PropertyValue &value, ObjectProperty::ID property);

    void sort_search_results(std::list<Core::SearchResult> &results, const SearchQuery &q);

private:
    std::unique_ptr<ToolBase> create_tool(ToolID tool_id);
};
} // namespace horizon
