#pragma once
#include "common/layer.hpp"
#include "util/uuid.hpp"
#include "util/layer_range.hpp"
#include <stdint.h>

namespace horizon {
class PropertyValue {
public:
    enum class Type { INVALID, INT, BOOL, STRING, UUID, DOUBLE, LAYER_RANGE };
    PropertyValue()
    {
    }

    virtual Type get_type() const
    {
        return Type::INVALID;
    };
    virtual ~PropertyValue()
    {
    }

protected:
};

class PropertyValueInt : public PropertyValue {
public:
    PropertyValueInt(const int64_t &v = 0) : value(v)
    {
    }
    Type get_type() const override
    {
        return Type::INT;
    };

    int64_t value;
};

class PropertyValueDouble : public PropertyValue {
public:
    PropertyValueDouble(const double &v = 0) : value(v)
    {
    }
    Type get_type() const override
    {
        return Type::DOUBLE;
    };

    double value;
};

class PropertyValueBool : public PropertyValue {
public:
    PropertyValueBool(bool v = false) : value(v)
    {
    }
    Type get_type() const override
    {
        return Type::BOOL;
    };

    bool value;
};

class PropertyValueString : public PropertyValue {
public:
    PropertyValueString(const std::string &v = "") : value(v)
    {
    }
    Type get_type() const override
    {
        return Type::STRING;
    };

    std::string value;
};

class PropertyValueUUID : public PropertyValue {
public:
    PropertyValueUUID(const UUID &v = UUID()) : value(v)
    {
    }
    Type get_type() const override
    {
        return Type::UUID;
    };

    UUID value;
};

class PropertyValueLayerRange : public PropertyValue {
public:
    PropertyValueLayerRange(const LayerRange &v = LayerRange()) : value(v)
    {
    }
    Type get_type() const override
    {
        return Type::LAYER_RANGE;
    };

    LayerRange value;
};

class PropertyMeta {
public:
    PropertyMeta()
    {
    }
    bool is_settable = true;
    bool is_visible = true;
    virtual ~PropertyMeta()
    {
    }
};

class PropertyMetaNetClasses : public PropertyMeta {
public:
    using PropertyMeta::PropertyMeta;
    std::map<UUID, std::string> net_classes;
};

class PropertyMetaLayers : public PropertyMeta {
public:
    using PropertyMeta::PropertyMeta;
    std::map<int, Layer> layers;
};
} // namespace horizon
