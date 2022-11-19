#include "core.hpp"
#include "common/dimension.hpp"
#include "logger/logger.hpp"
#include <memory>
#include "nlohmann/json.hpp"
#include "imp/action_catalog.hpp"
#include "imp/actions.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include <glibmm/ustring.h>
#include <glibmm/fileutils.h>
#include "core/tool_id.hpp"

namespace horizon {

Core::ToolStateSetter::ToolStateSetter(ToolState &s, ToolState target) : state(s)
{
    if (state != ToolState::NONE) {
        Logger::log_critical("can't enter tool state " + tool_state_to_string(target) + " in state "
                                     + tool_state_to_string(state),
                             Logger::Domain::TOOL);
        error = true;
    }
    else {
        state = target;
    }
}

std::string Core::ToolStateSetter::tool_state_to_string(ToolState s)
{
    switch (s) {
    case ToolState::BEGINNING:
        return "beginning";
    case ToolState::UPDATING:
        return "updating";
    case ToolState::NONE:
        return "none";
    default:
        return "??";
    }
}

Core::ToolStateSetter::~ToolStateSetter()
{
    if (!error)
        state = ToolState::NONE;
}

ToolResponse Core::tool_begin(ToolID tool_id, const ToolArgs &args, class ImpInterface *imp, bool transient)
{
    if (tool_is_active()) {
        throw std::runtime_error("can't begin tool while tool is active");
        return ToolResponse::end();
    }
    ToolStateSetter state_setter{tool_state, ToolState::BEGINNING};
    if (state_setter.check_error())
        return ToolResponse::end();

    update_rules(); // write rules to board, so tool has the current rules
    try {
        tool = create_tool(tool_id);

        for (auto [tid, settings] : tool->get_all_settings()) {
            auto j = s_signal_load_tool_settings.emit(tid);
            if (j != nullptr)
                settings->load_from_json(j);
            tool->apply_settings();
        }

        tool->set_imp_interface(imp);
        if (!args.keep_selection) {
            tool->selection.clear();
            tool->selection = args.selection;
        }
        if (transient)
            tool->set_transient();
        if (!tool->can_begin()) { // check if we can actually use this tool
            tool.reset();
            return ToolResponse();
        }
    }
    catch (const std::exception &e) {
        Logger::log_critical("exception thrown in tool constructor of "
                                     + action_catalog.at({ActionID::TOOL, tool_id}).name,
                             Logger::Domain::CORE, e.what());
        return ToolResponse::end();
    }
    if (tool) {
        s_signal_tool_changed.emit(tool_id);
        ToolResponse r;
        try {
            r = tool->begin(args);
        }
        catch (const std::exception &e) {
            tool.reset();
            s_signal_tool_changed.emit(ToolID::NONE);
            Logger::log_critical("exception thrown in tool_begin of "
                                         + action_catalog.at({ActionID::TOOL, tool_id}).name,
                                 Logger::Domain::CORE, e.what());
            tool_id_current = ToolID::NONE;
            history_load(history_manager.get_current());
            rebuild_internal(true, "undo");
            return ToolResponse::end();
        }
        tool_id_current = tool_id;
        maybe_end_tool(r);

        return r;
    }

    return ToolResponse();
}

void Core::maybe_end_tool(const ToolResponse &r)
{
    if (r.result != ToolResponse::Result::NOP) { // end tool
        for (auto [tid, settings] : tool->get_all_settings()) {
            s_signal_save_tool_settings.emit(tid, settings->serialize());
        }
        tool_selection = tool->selection;
        tool.reset();
        s_signal_tool_changed.emit(ToolID::NONE);
        if (r.result == ToolResponse::Result::COMMIT) {
            set_needs_save(true);
            const auto comment = action_catalog.at(tool_id_current).name;
            rebuild_internal(false, comment);
        }
        else if (r.result == ToolResponse::Result::REVERT) {
            history_load(history_manager.get_current());
            rebuild_internal(true, "undo");
        }
        else if (r.result == ToolResponse::Result::END) { // did nothing
            // do nothing
        }
        tool_id_current = ToolID::NONE;
    }
}

std::set<SelectableRef> &Core::get_tool_selection()
{
    if (tool)
        return tool->selection;
    else
        return tool_selection;
}

std::set<InToolActionID> Core::get_tool_actions() const
{
    if (tool)
        return tool->get_actions();
    else
        return {};
}

std::pair<bool, bool> Core::tool_can_begin(ToolID tool_id, const std::set<SelectableRef> &sel)
{
    auto t = create_tool(tool_id);
    t->selection = sel;
    auto r = t->can_begin();
    auto s = t->is_specific();
    return {r, s};
}

ToolResponse Core::tool_update(ToolArgs &args)
{
    if (tool_state != ToolState::NONE) {
        pending_tool_args.emplace_back(std::move(args));
        return {};
    }
    ToolStateSetter state_setter{tool_state, ToolState::UPDATING};
    if (state_setter.check_error())
        return ToolResponse::end();
    if (tool) {
        ToolResponse r;
        try {
            r = tool->update(args);
        }
        catch (const std::exception &e) {
            tool.reset();
            s_signal_tool_changed.emit(ToolID::NONE);
            Logger::log_critical("exception thrown in tool_update", Logger::Domain::CORE, e.what());
            tool_id_current = ToolID::NONE;
            history_load(history_manager.get_current());
            rebuild_internal(true, "undo");
            return ToolResponse::end();
        }
        maybe_end_tool(r);
        return r;
    }
    return ToolResponse();
}

std::optional<ToolArgs> Core::get_pending_tool_args()
{
    if (tool_state != ToolState::NONE)
        return {};
    if (pending_tool_args.empty())
        return {};
    auto r = std::move(pending_tool_args.front());
    pending_tool_args.pop_front();
    return r;
}

void Core::rebuild(const std::string &comment)
{
    rebuild_internal(false, comment);
}

void Core::rebuild_finish(bool from_undo, const std::string &comment)
{
    if (!from_undo) {

        std::string comment_str = comment;
        if (const auto comment_ctx = get_history_comment_context(); comment_ctx.size())
            comment_str += " " + comment_ctx;

        history_manager.push(make_history_item(comment_str));
    }
    s_signal_rebuilt.emit();
    signal_can_undo_redo().emit();
}

std::string Core::get_history_comment_context() const
{
    return {};
}

void Core::undo()
{
    if (!history_manager.can_undo())
        return;
    history_load(history_manager.undo());
    signal_rebuilt().emit();
    set_needs_save();
}

void Core::redo()
{
    if (!history_manager.can_redo())
        return;
    history_load(history_manager.redo());
    signal_rebuilt().emit();
    set_needs_save();
}

const std::string &Core::get_undo_comment() const
{
    return history_manager.get_undo_comment();
}

const std::string &Core::get_redo_comment() const
{
    return history_manager.get_redo_comment();
}

void Core::history_clear()
{
    history_manager.clear();
}

void Core::set_history_max(unsigned int m)
{
    history_manager.set_history_max(m);
}

bool Core::can_redo() const
{
    return history_manager.can_redo();
}

bool Core::can_undo() const
{
    return history_manager.can_undo();
}

void Core::set_property_begin()
{
    if (property_transaction)
        throw std::runtime_error("transaction already in progress");
    property_transaction = true;
}

void Core::set_property_commit()
{
    if (!property_transaction)
        throw std::runtime_error("no transaction in progress");
    rebuild("edit property");
    set_needs_save(true);
    property_transaction = false;
}

bool Core::get_property_transaction() const
{
    return property_transaction;
}

json Core::get_meta()
{
    json j;
    return j;
}

json Core::get_meta_from_file(const std::string &filename)
{
    auto j = load_json_from_file(filename);
    if (j.count("_imp")) {
        return j["_imp"];
    }
    return nullptr;
}

void Core::set_needs_save(bool v)
{
    if (v)
        s_signal_modified.emit();

    if (v != needs_save) {
        needs_save = v;
        s_signal_needs_save.emit(v);
    }
}

void Core::set_needs_save()
{
    set_needs_save(true);
}

bool Core::get_needs_save() const
{
    return needs_save;
}

void Core::save()
{
    save("");
    delete_autosave();
    set_needs_save(false);
}

const std::string Core::autosave_suffix = ".autosave";

void Core::autosave()
{
    if (get_filename().size())
        save(autosave_suffix);
}

Core::Core(IPool &pool, IPool *pool_caching)
    : m_pool(pool), m_pool_caching(pool_caching ? *pool_caching : pool), tool_id_current(ToolID::NONE)
{
    history_manager.signal_changed().connect([this] { s_signal_can_undo_redo.emit(); });
}


} // namespace horizon
