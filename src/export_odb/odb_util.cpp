#include "odb_util.hpp"
#include <iomanip>
#include <glibmm/ustring.h>
#include <glibmm/convert.h>
#include "board/board_layers.hpp"
#include "common/layer_provider.hpp"

namespace horizon::ODB {

const char *endl = "\r\n";

std::ostream &operator<<(std::ostream &os, const Coordi &c)
{
    return os << std::fixed << std::setprecision(6) << c.x / 1e6 << " " << c.y / 1e6;
}

std::ostream &operator<<(std::ostream &os, Angle a)
{
    return os << std::fixed << std::setprecision(1) << std::fixed << a.angle;
}

std::ostream &operator<<(std::ostream &os, Dim d)
{
    return os << std::fixed << std::setprecision(6) << std::fixed << d.dim;
}

std::ostream &operator<<(std::ostream &os, DimUm d)
{
    return os << std::fixed << std::setprecision(3) << std::fixed << d.dim;
}

static bool check_have_transliteration()
{
    auto ic = g_iconv_open("ascii//TRANSLIT", "utf-8");
    const bool have_translit = ic != ((GIConv)-1);
    if (!have_translit)
        return false;
    g_iconv_close(ic);
    return true;
}

std::string utf8_to_ascii(const std::string &s)
{
    static bool have_translit = check_have_transliteration();
    return Glib::convert_with_fallback(s, have_translit ? "ascii//TRANSLIT" : "ascii", "utf-8");
}

std::string make_legal_name(const std::string &n)
{
    std::string out;
    out.reserve(n.size());
    for (auto c : utf8_to_ascii(n)) {
        if (c == ';' || isspace(c))
            c = '_';
        else if (isgraph(c))
            ;
        else
            c = '_';
        out.append(1, c);
    }

    return out;
}

std::string make_legal_entity_name(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (auto c : utf8_to_ascii(s)) {
        if (isalpha(c))
            c = tolower(c);
        else if (isdigit(c) || (c == '-') || (c == '_') || (c == '+'))
            ; // it's okay
        else
            c = '_';
        out.append(1, c);
    }

    return out;
}

std::string get_layer_name(int id, const LayerProvider &lprv)
{
    if (id == BoardLayers::TOP_COPPER) {
        return "signal_top";
    }
    else if (id == BoardLayers::BOTTOM_COPPER) {
        return "signal_bottom";
    }
    else if (id < BoardLayers::TOP_COPPER && id > BoardLayers::BOTTOM_COPPER) {
        return "signal_inner_" + std::to_string(-id);
    }
    else if (id == BoardLayers::TOP_SILKSCREEN) {
        return "silkscreen_top";
    }
    else if (id == BoardLayers::BOTTOM_SILKSCREEN) {
        return "silkscreen_bottom";
    }
    else if (id == BoardLayers::TOP_MASK) {
        return "mask_top";
    }
    else if (id == BoardLayers::BOTTOM_MASK) {
        return "mask_bottom";
    }
    else if (id == BoardLayers::TOP_PASTE) {
        return "paste_top";
    }
    else if (id == BoardLayers::BOTTOM_PASTE) {
        return "paste_bottom";
    }
    else if (id == BoardLayers::TOP_ASSEMBLY) {
        return "assembly_top";
    }
    else if (id == BoardLayers::BOTTOM_ASSEMBLY) {
        return "assembly_bottom";
    }
    else if (BoardLayers::is_user(id)) {
        return make_legal_entity_name(lprv.get_layer_name(id));
    }
    else {
        return "layer_id_" + std::to_string(id);
    }
}

std::string get_drills_layer_name(const LayerRange &span, const LayerProvider &lprv)
{
    return "drills-" + get_layer_name(span.end(), lprv) + "-" + get_layer_name(span.start(), lprv);
}

std::string make_symbol_circle(uint64_t diameter)
{
    std::ostringstream oss;
    oss << "r" << DimUm{diameter} << " M";
    return oss.str();
}

std::string make_symbol_rect(uint64_t w, uint64_t h)
{
    std::ostringstream oss;
    oss << "rect" << DimUm{w} << "x" << DimUm{h} << " M";
    return oss.str();
}

std::string make_symbol_oval(uint64_t w, uint64_t h)
{
    std::ostringstream oss;
    oss << "oval" << DimUm{w} << "x" << DimUm{h} << " M";
    return oss.str();
}

} // namespace horizon::ODB
