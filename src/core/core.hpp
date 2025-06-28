#pragma once
#include "canvas/selectables.hpp"
#include "common/object_descr.hpp"
#include <nlohmann/json_fwd.hpp>
#include <gdk/gdkkeysyms.h>
#include <deque>
#include <memory>
#include <sigc++/sigc++.h>
#include "tool_pub.hpp"
#include "document/document.hpp"
#include "util/placement.hpp"
#include "util/history_manager.hpp"
namespace horizon {

enum class ToolID;

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
class Core : public virtual Document {
public:
    class Block *get_top_block() override
    {
        return nullptr;
    }

    class Rules *get_rules() override
    {
        return nullptr;
    }

    class GridSettings *get_grid_settings() override
    {
        return nullptr;
    }

    class IPool &get_pool() override
    {
        return m_pool;
    }
    class IPool &get_pool_caching() override
    {
        return m_pool_caching;
    }
    virtual ObjectType get_object_type() const = 0;

    /**
     * Expands the non-working document.
     * And copies the non-working document to the working document.
     */
    void rebuild(const std::string &comment);
    ToolResponse tool_begin(ToolID tool_id, const ToolArgs &args, class ImpInterface *imp, bool transient = false);
    ToolResponse tool_update(ToolArgs &args);
    std::optional<ToolArgs> get_pending_tool_args();
    std::pair<bool, bool> tool_can_begin(ToolID tool_id, const std::set<SelectableRef> &selection);
    void save();
    void autosave();
    virtual void delete_autosave() = 0;

    void undo();
    void redo();

    bool can_undo() const;
    bool can_redo() const;

    const std::string &get_undo_comment() const;
    const std::string &get_redo_comment() const;

    void set_history_max(unsigned int m);
    void set_history_never_forgets(bool x);

    inline bool tool_is_active()
    {
        return tool != nullptr;
    }

    virtual bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                              const class PropertyValue &value);
    virtual bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyValue &value);
    virtual bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property,
                                   class PropertyMeta &meta);

    void set_property_begin();
    void set_property_commit();
    bool get_property_transaction() const;

    /**
     * @returns the current document's meta information.
     * Meta information contains grid spacing and layer setup.
     */
    virtual json get_meta();

    virtual void update_rules()
    {
    }

    virtual std::pair<Coordi, Coordi> get_bbox() = 0;

    virtual ~Core()
    {
    }
    std::set<SelectableRef> &get_tool_selection();
    std::set<InToolActionID> get_tool_actions() const;

    bool get_needs_save() const;
    void set_needs_save();

    virtual const std::string &get_filename() const = 0;

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

    type_signal_rebuilt signal_modified()
    {
        return s_signal_modified;
    }

    type_signal_rebuilt signal_can_undo_redo()
    {
        return s_signal_can_undo_redo;
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
    Core(class IPool &pool, IPool *m_pool_caching);
    class IPool &m_pool;
    class IPool &m_pool_caching;

    type_signal_tool_changed s_signal_tool_changed;
    type_signal_rebuilt s_signal_rebuilt;
    type_signal_rebuilt s_signal_save;
    type_signal_rebuilt s_signal_modified;
    type_signal_rebuilt s_signal_can_undo_redo;
    type_signal_needs_save s_signal_needs_save;
    type_signal_load_tool_settings s_signal_load_tool_settings;
    type_signal_save_tool_settings s_signal_save_tool_settings;
    bool needs_save = false;
    void set_needs_save(bool v);

    void rebuild_finish(bool from_undo, const std::string &comment);

    HistoryManager history_manager;

    virtual std::unique_ptr<HistoryManager::HistoryItem> make_history_item(const std::string &comment) = 0;
    virtual void history_load(const HistoryManager::HistoryItem &it) = 0;
    virtual std::string get_history_comment_context() const;

    bool property_transaction = false;

    void layers_to_meta(class PropertyMeta &meta);
    void get_placement(const Placement &placement, class PropertyValue &value, ObjectProperty::ID property);
    void set_placement(Placement &placement, const class PropertyValue &value, ObjectProperty::ID property);

    virtual void save(const std::string &suffix) = 0;
    static const std::string autosave_suffix;
    json get_meta_from_file(const std::string &filename);

private:
    std::unique_ptr<ToolBase> create_tool(ToolID tool_id);
    std::set<SelectableRef> tool_selection;
    void maybe_end_tool(const ToolResponse &r);
    virtual void rebuild_internal(bool from_undo, const std::string &comment) = 0;

    ToolID tool_id_current;
    std::unique_ptr<ToolBase> tool = nullptr;
    enum class ToolState { NONE, BEGINNING, UPDATING };
    ToolState tool_state = ToolState::NONE;

    std::list<ToolArgs> pending_tool_args;

    class ToolStateSetter {
    public:
        ToolStateSetter(ToolState &s, ToolState target);
        bool check_error() const
        {
            return error;
        }
        ~ToolStateSetter();

    private:
        ToolState &state;
        bool error = false;
        static std::string tool_state_to_string(ToolState s);
    };
};
} // namespace horizon
