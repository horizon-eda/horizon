#pragma once
#include "common/common.hpp"
#include "canvas/selectables.hpp"
#include "canvas/target.hpp"
#include "tool_data.hpp"
#include <nlohmann/json_fwd.hpp>
#include "document/documents.hpp"
#include <set>
#include <memory>

namespace horizon {
using json = nlohmann::json;

enum class ToolEventType { NONE, MOVE, ACTION, LAYER_CHANGE, DATA };

enum class ToolID;
enum class InToolActionID;

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
    InToolActionID action;

    Target target;
    int work_layer = 0;
    std::unique_ptr<ToolData> data = nullptr;
    ToolArgs();
};

/**
 * To signal back to the core what the Tool did, a Tool returns a ToolResponse.
 */
class ToolResponse {
public:
    ToolID next_tool;
    std::unique_ptr<ToolData> data = nullptr;
    enum class Result { NOP, END, COMMIT, REVERT };
    Result result = Result::NOP;
    /**
     * Use this if you're done. The Core will then delete the active tool and
     * initiate a rebuild.
     */
    static ToolResponse end()
    {
        return ToolResponse(Result::END);
    }

    static ToolResponse commit()
    {
        return ToolResponse(Result::COMMIT);
    }

    static ToolResponse revert()
    {
        return ToolResponse(Result::REVERT);
    }

    /**
     * If you want another Tool to be launched you've finished, use this one.
     */
    static ToolResponse next(Result res, ToolID t, std::unique_ptr<ToolData> data = nullptr)
    {
        ToolResponse r(res);
        r.next_tool = t;
        r.data = std::move(data);
        return r;
    };

    ToolResponse();

private:
    ToolResponse(Result r);
};

class ToolSettings {
public:
    virtual void load_from_json(const json &j) = 0;
    virtual json serialize() const = 0;
    virtual ~ToolSettings()
    {
    }
};

/**
 * Common interface for all Tools
 */
class ToolBase {
public:
    ToolBase(class IDocument *c, ToolID tid);
    void set_imp_interface(class ImpInterface *i);
    void set_transient();

    virtual void apply_settings()
    {
    }

    virtual std::map<ToolID, ToolSettings *> get_all_settings()
    {
        if (auto s = get_settings())
            return {{tool_id, s}};
        else
            return {};
    }

    virtual std::set<InToolActionID> get_actions() const
    {
        return {};
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

    std::set<SelectableRef> selection;

    virtual ~ToolBase()
    {
    }

protected:
    virtual ToolSettings *get_settings()
    {
        return nullptr;
    }

    Documents doc;
    class ImpInterface *imp = nullptr;
    const ToolID tool_id;
    bool is_transient = false;
};
} // namespace horizon
