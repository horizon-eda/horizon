#pragma once
#include "common/common.hpp"
#include "canvas/selectables.hpp"
#include "canvas/target.hpp"
#include "tool_data.hpp"
#include "nlohmann/json_fwd.hpp"
#include "document/documents.hpp"
#include <set>
#include <memory>

namespace horizon {
using json = nlohmann::json;

enum class ToolEventType { NONE, MOVE, CLICK, CLICK_RELEASE, KEY, LAYER_CHANGE, DATA };

enum class ToolID;

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
    ToolID next_tool;
    std::unique_ptr<ToolData> data = nullptr;
    enum class Result { NOP, END, COMMIT, REVERT };
    Result result = Result::NOP;
    bool fast_draw = false;

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

    static ToolResponse fast()
    {
        ToolResponse r;
        r.fast_draw = true;
        return r;
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
    ToolBase(class IDocument *c, ToolID tid);
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

    std::set<SelectableRef> selection;

    virtual ~ToolBase()
    {
    }

protected:
    Documents doc;
    class ImpInterface *imp = nullptr;
    ToolID tool_id;
    bool is_transient = false;
    virtual ToolSettings *get_settings()
    {
        return nullptr;
    }
};
} // namespace horizon
