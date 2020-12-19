#pragma once
#include "core/tool.hpp"
#include <forward_list>

namespace horizon {


class ToolPlaceJunctionBase : public virtual ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
        };
    }

    virtual ~ToolPlaceJunctionBase()
    {
    }

protected:
    virtual class Junction *get_junction() = 0;
    std::forward_list<Junction *> junctions_placed;

    virtual void insert_junction() = 0;
    virtual bool junction_placed()
    {
        return false;
    }
    void create_junction(const Coordi &c);
    virtual void create_attached()
    {
    }
    virtual void delete_attached()
    {
    }
    virtual bool update_attached(const ToolArgs &args)
    {
        return false;
    }
    virtual bool begin_attached()
    {
        return true;
    }
};

template <typename T> class ToolPlaceJunctionT : public ToolPlaceJunctionBase {
public:
    using ToolPlaceJunctionBase::ToolPlaceJunctionBase;

protected:
    T *temp = nullptr;

    Junction *get_junction() override
    {
        return temp;
    };
};

class ToolPlaceJunction : public ToolPlaceJunctionT<Junction> {
public:
    using ToolPlaceJunctionT<Junction>::ToolPlaceJunctionT;

protected:
    void insert_junction() override;
};

} // namespace horizon
