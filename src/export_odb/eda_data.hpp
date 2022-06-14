#pragma once
#include <list>
#include <memory>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "attribute_util.hpp"
#include "surface_data.hpp"

namespace horizon {
class Net;
class Pad;
class Package;
} // namespace horizon

namespace horizon::ODB {

class EDAData : public AttributeProvider {
public:
    EDAData();

    void write(std::ostream &ost) const;

    class FeatureID {
        friend EDAData;

    public:
        enum class Type { COPPER, LAMINATE, HOLE };

        FeatureID(Type t, unsigned int l, unsigned int fid) : type(t), layer(l), feature_id(fid)
        {
        }

        Type type;
        unsigned int layer;
        unsigned int feature_id;

        void write(std::ostream &ost) const;
    };

    class Subnet {
    public:
        Subnet(unsigned int i) : index(i)
        {
        }
        const unsigned int index;
        void write(std::ostream &ost) const;

        std::list<FeatureID> feature_ids;

        virtual ~Subnet() = default;

    protected:
        virtual void write_subnet(std::ostream &ost) const = 0;
    };

    class SubnetVia : public Subnet {
    public:
        using Subnet::Subnet;
        void write_subnet(std::ostream &ost) const override;
    };

    class SubnetTrace : public Subnet {
    public:
        using Subnet::Subnet;
        void write_subnet(std::ostream &ost) const override;
    };

    class SubnetPlane : public Subnet {
    public:
        enum class FillType { SOLID, OUTLINE };
        enum class CutoutType { CIRCLE, RECT, OCTAGON, EXACT };

        SubnetPlane(unsigned int i, FillType ft, CutoutType ct, double fs)
            : Subnet(i), fill_type(ft), cutout_type(ct), fill_size(fs)
        {
        }

        FillType fill_type;
        CutoutType cutout_type;
        double fill_size;

        void write_subnet(std::ostream &ost) const override;
    };

    class SubnetToeprint : public Subnet {
    public:
        enum class Side { TOP, BOTTOM };

        SubnetToeprint(unsigned int i, Side s, unsigned int c, unsigned int t)
            : Subnet(i), side(s), comp_num(c), toep_num(t)
        {
        }

        Side side;

        unsigned int comp_num;
        unsigned int toep_num;

        void write_subnet(std::ostream &ost) const override;
    };

    void add_feature_id(Subnet &subnet, FeatureID::Type type, const std::string &layer, unsigned int feature_id);

    class Net : public RecordWithAttributes {
    public:
        template <typename T> using check_type = attribute::is_net<T>;

        Net(unsigned int index, const std::string &name);
        const unsigned int index;

        std::string name;

        std::list<std::unique_ptr<Subnet>> subnets;

        template <typename T, typename... Args> T &add_subnet(Args &&...args)
        {
            auto f = std::make_unique<T>(subnets.size(), std::forward<Args>(args)...);
            auto &r = *f;
            subnets.push_back(std::move(f));
            return r;
        }

        void write(std::ostream &ost) const;
    };

    Net &add_net(const horizon::Net &net);
    Net &get_net(const UUID &uu)
    {
        return nets_map.at(uu);
    }

    class Outline {
    public:
        virtual void write(std::ostream &ost) const = 0;

        virtual ~Outline() = default;
    };

    class OutlineRectangle : public Outline {
    public:
        OutlineRectangle(const Coordi &l, uint64_t w, uint64_t h) : lower(l), width(w), height(h)
        {
        }
        OutlineRectangle(const std::pair<Coordi, Coordi> &bb)
            : OutlineRectangle(bb.first, bb.second.x - bb.first.x, bb.second.y - bb.first.y)
        {
        }

        Coordi lower;
        uint64_t width;
        uint64_t height;

        void write(std::ostream &ost) const override;
    };

    class OutlineContour : public Outline {
    public:
        SurfaceData data;

        void write(std::ostream &ost) const override;
    };

    class OutlineSquare : public Outline {
    public:
        OutlineSquare(const Coordi &c, uint64_t s) : center(c), half_side(s)
        {
        }
        Coordi center;
        uint64_t half_side;

        void write(std::ostream &ost) const override;
    };

    class OutlineCircle : public Outline {
    public:
        OutlineCircle(const Coordi &c, uint64_t r) : center(c), radius(r)
        {
        }
        Coordi center;
        uint64_t radius;

        void write(std::ostream &ost) const override;
    };

    class Pin {
    public:
        Pin(unsigned int i, const std::string &n);
        std::string name;
        const unsigned int index;

        Coordi center;

        enum class Type { THROUGH_HOLE, BLIND, SURFACE };
        Type type = Type::SURFACE;

        enum class ElectricalType { ELECTRICAL, MECHANICAL, UNDEFINED };
        ElectricalType etype = ElectricalType::UNDEFINED;

        enum class MountType {
            SMT,
            SMT_RECOMMENDED,
            THROUGH_HOLE,
            THROUGH_RECOMMENDED,
            PRESSFIT,
            NON_BOARD,
            HOLE,
            UNDEFINED
        };
        MountType mtype = MountType::UNDEFINED;

        std::list<std::unique_ptr<Outline>> outline;

        void write(std::ostream &ost) const;
    };

    class Package : public RecordWithAttributes {
    public:
        template <typename T> using check_type = attribute::is_pkg<T>;

        Package(const unsigned int i, const std::string &n);
        const unsigned int index;
        std::string name;

        uint64_t pitch;
        int64_t xmin, ymin, xmax, ymax;

        std::list<std::unique_ptr<Outline>> outline;

        Pin &add_pin(const horizon::Pad &pad);
        const Pin &get_pin(const UUID &uu) const
        {
            return pins_map.at(uu);
        }

        void write(std::ostream &ost) const;


    private:
        std::map<UUID, Pin> pins_map;
        std::list<const Pin *> pins;
    };

    Package &add_package(const horizon::Package &pkg);
    const Package &get_package(const UUID &uu) const
    {
        return packages_map.at(uu);
    }

private:
    std::map<UUID, Net> nets_map;
    std::list<const Net *> nets;

    std::map<UUID, Package> packages_map;
    std::list<const Package *> packages;

    unsigned int get_or_create_layer(const std::string &l);

    std::map<std::string, unsigned int> layers_map;
    std::vector<std::string> layers;
};
} // namespace horizon::ODB
