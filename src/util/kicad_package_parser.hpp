#pragma once
#include "common/common.hpp"
#include <set>
#include <functional>

namespace SEXPR {
class SEXPR;
}

namespace horizon {
class KiCadModuleParser {
public:
    using LogCb = std::function<void(const std::string &, const std::string &)>;
    void set_log_cb(LogCb cb);

protected:
    KiCadModuleParser();
    class Line *parse_line(const SEXPR::SEXPR *data);
    void parse_poly(const SEXPR::SEXPR *data);
    static int get_layer(const std::string &l);
    static Coordi get_coord(const SEXPR::SEXPR *data, size_t offset = 1);
    static Coordi get_size(const SEXPR::SEXPR *data, size_t offset = 1);
    std::map<Coordi, class Junction *> junctions;
    Junction *get_or_create_junction(const Coordi &c);
    LogCb log_cb;

    virtual class Junction &create_junction() = 0;
    virtual class Polygon &create_polygon() = 0;
    virtual class Line &create_line() = 0;

public:
    virtual ~KiCadModuleParser()
    {
    }
};

class KiCadPackageParser : public KiCadModuleParser {
public:
    KiCadPackageParser(class Package &p, class IPool &po);

    class Meta {
    public:
        std::string name;
        std::string descr;
        std::set<std::string> tags;
    };

    Meta parse(const SEXPR::SEXPR *data);

private:
    void parse_pad(const SEXPR::SEXPR *data);

    class Junction &create_junction() override;
    class Polygon &create_polygon() override;
    class Line &create_line() override;

    Package &package;
    IPool &pool;
};

class KiCadDecalParser : public KiCadModuleParser {
public:
    KiCadDecalParser(class Decal &d);
    void parse(const SEXPR::SEXPR *data);

private:
    class Junction &create_junction() override;
    class Polygon &create_polygon() override;
    class Line &create_line() override;

    Decal &decal;
};

} // namespace horizon
