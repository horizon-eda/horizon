#pragma once
#include <ostream>

namespace horizon {
class StructuredTextWriter {
public:
    StructuredTextWriter(std::ostream &s);
    void write_line(const std::string &var, const std::string &value);
    void write_line(const std::string &var, int value);
    template <typename T> void write_line_enum(const std::string &var, const T &value)
    {
        write_line(var, enum_to_string(value));
    }

    class ArrayProxy {
        friend StructuredTextWriter;

    public:
        ~ArrayProxy();

    private:
        ArrayProxy(StructuredTextWriter &writer, const std::string &a);

        StructuredTextWriter &writer;

        ArrayProxy(ArrayProxy &&) = delete;
        ArrayProxy &operator=(ArrayProxy &&) = delete;

        ArrayProxy(ArrayProxy const &) = delete;
        ArrayProxy &operator=(ArrayProxy const &) = delete;
    };

    [[nodiscard]] ArrayProxy make_array_proxy(const std::string &a)
    {
        return ArrayProxy(*this, a);
    }

private:
    void write_indent();
    void begin_array(const std::string &a);
    void end_array();
    std::ostream &ost;
    bool in_array = false;
};
} // namespace horizon
