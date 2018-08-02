#include "csv.hpp"
#include <istream>

namespace CSV {
csv::csv(const std::string &delim) : delim(delim)
{
}

std::size_t csv::size() const
{
    return val.size();
}

const std::vector<std::string> &csv::operator[](std::size_t i) const
{
    return val[i];
}

std::vector<std::vector<std::string>>::const_iterator csv::begin() const
{
    return val.begin();
}

std::vector<std::vector<std::string>>::const_iterator csv::end() const
{
  return val.end();
}

enum class State { Q0, Q1, ESC };

bool csv::isdelim(char c)
{
    return delim.find_first_of(c) != std::string::npos;
}

void csv::parseline(const std::string &line)
{
    State state = State::Q0;
    std::vector<std::string> field;
    std::string str;

    for (char c : line) {
        if (state == State::Q0) {
            if (c == '\"') {
                /* input: foo" */
                state = State::Q1;
            } else if (isdelim(c)) {
                /* input: foo, */
                field.push_back(str);
                str = "";
            } else {
                /* input: fooc */
                str += c;
            }
        } else if (state == State::Q1) {
            if (c == '\"') {
                /* input: "foo" */
                state = State::ESC;
            } else {
                /* input: "fooc */
                str += c;
            }
        } else if (state == State::ESC) {
            if (c == '\"') {
                /* input: "foo"" */
                str += c;
                state = State::Q1;
            } else if (isdelim(c)) {
                /* input: "foo", */
                field.push_back(str);
                str = "";
                state = State::Q0;
            } else {
                /* input: "foo"c */
                str += c;
                state = State::Q0;
            }
        }
    }
    field.push_back(str);
    val.push_back(field);
}

std::istream &operator>>(std::istream& is, csv &csv)
{
    while (!is.eof()) {
        std::string line;
        std::getline(is, line);
        csv.parseline(line);
    }

    if (0) {
        is.setstate(std::ios::failbit);
    }

    return is;
}

} // namespace CSV

