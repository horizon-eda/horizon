#pragma once

/*
 * Specification
 * 1. Fields are separated by a user specified delimiter (default comma) and
 *    newline.
 * 2. White space is never touched.
 * 3. A field may be enclosed in double-quote characters "...".
 * 4. A quoted field may contain commas.
 * 5. A sequence of "" inside a quoted field is interpreted as a single '"'.
 * 6. Fields may be empty; "" and an empty string both represent an empty field.
 *
 * Another description for quote mode (3., 4., 5. above):
 * - When not in quote mode, a double-quote enters quote mode.
 * - In quote mode, a double-quote not followed by a double-quote exits quote
 *   mode.
 * - In quote mode, the sequence "" is emitted as a single '"'.
 * - Delimiters have no special meaning in quote mode.
 *
 * Consequences:
 * - Quote mode does not nest.
 * - A line should have an even number of double-quote characters.
 */

#include <string>
#include <vector>

namespace CSV {

class Csv {
public:
    Csv(const std::string &delim = ",");
    void parseline(const std::string &line);
    std::size_t size() const;
    /* Make each line have at least n fields. */
    void expand(std::size_t n, const std::string &pad = "");
    const std::vector<std::string> &operator[](std::size_t i) const;
    std::vector<std::vector<std::string>>::const_iterator begin() const;
    std::vector<std::vector<std::string>>::const_iterator end() const;

private:
    bool isdelim(char c);
    std::vector<std::vector<std::string>> val;
    std::string delim;
};

std::istream &operator>>(std::istream &is, Csv &obj);

} // namespace CSV
