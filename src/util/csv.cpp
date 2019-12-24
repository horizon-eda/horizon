#include "csv.hpp"
#include <istream>

namespace CSV {
Csv::Csv(const std::string &adelim) : delim(adelim)
{
}

std::size_t Csv::size() const
{
    return val.size();
}

void Csv::expand(std::size_t n, const std::string &pad)
{
    for (auto &line : val) {
        while (line.size() < n) {
            line.push_back(pad);
        }
    }
}

const std::vector<std::string> &Csv::operator[](std::size_t i) const
{
    return val[i];
}

std::vector<std::vector<std::string>>::const_iterator Csv::begin() const
{
    return val.begin();
}

std::vector<std::vector<std::string>>::const_iterator Csv::end() const
{
    return val.end();
}

enum class State { Q0, Q1, ESC };

bool Csv::isdelim(char c)
{
    return delim.find_first_of(c) != std::string::npos;
}

void Csv::parseline(const std::string &line)
{
    State state = State::Q0;
    std::vector<std::string> field;
    std::string str;

    for (char c : line) {
        if (state == State::Q0) {
            if (c == '\"') {
                /* input: foo" */
                state = State::Q1;
            }
            else if (isdelim(c)) {
                /* input: foo, */
                field.push_back(str);
                str = "";
            }
            else {
                /* input: fooc */
                str += c;
            }
        }
        else if (state == State::Q1) {
            if (c == '\"') {
                /* input: "foo" */
                state = State::ESC;
            }
            else {
                /* input: "fooc */
                str += c;
            }
        }
        else if (state == State::ESC) {
            if (c == '\"') {
                /* input: "foo"" */
                str += c;
                state = State::Q1;
            }
            else if (isdelim(c)) {
                /* input: "foo", */
                field.push_back(str);
                str = "";
                state = State::Q0;
            }
            else {
                /* input: "foo"c */
                str += c;
                state = State::Q0;
            }
        }
    }
    field.push_back(str);
    val.push_back(field);
}

std::istream &operator>>(std::istream &is, Csv &csv)
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
