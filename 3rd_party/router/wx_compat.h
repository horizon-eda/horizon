#pragma once
#include <string>
#include <cassert>

template <class... Args> void wxLogTrace(const char *mask, const char *formatString, Args &&...args)
{
}

template <class... Args> void wxLogWarning(const char *formatString, Args &&...args)
{
}

class wxString : public std::string {
public:
    wxString() = default;

    wxString(const std::string &s) : std::string(s)
    {
    }

    wxString(const char *s) : std::string(s)
    {
    }

    std::string ToStdString() const
    {
        return *this;
    }

    template <class... Args> static wxString Format(const wxString &format, Args &&...args)
    {
        return format;
    }
};

const wxString wxEmptyString;

using wxChar = char;

class wxPoint {
public:
    wxPoint() : x(0), y(0)
    {
    }

    wxPoint(int ax, int ay) : x(ax), y(ay)
    {
    }

    int x;
    int y;
};

inline wxPoint operator+(const wxPoint &p1, const wxPoint &p2)
{
    return wxPoint(p1.x + p2.x, p1.y + p2.y);
}

inline wxPoint operator-(const wxPoint &p1, const wxPoint &p2)
{
    return wxPoint(p1.x - p2.x, p1.y - p2.y);
}

class wxSize {
public:
    wxSize() : x(0), y(0)
    {
    }

    int x;
    int y;
};

#define wxASSERT assert
#define wxASSERT_MSG(cond, msg) assert((cond))
#define dyn_cast dynamic_cast

#define wxCHECK2_MSG(cond, op, msg)                                                                                    \
    do {                                                                                                               \
        if (cond) {                                                                                                    \
        }                                                                                                              \
        else {                                                                                                         \
            op;                                                                                                        \
        }                                                                                                              \
    } while (0)


#define wxCHECK_MSG(cond, rc, msg) wxCHECK2_MSG(cond, return rc, msg)
#define wxCHECK_RET(cond, msg) wxCHECK2_MSG(cond, return, msg)
#define wxCHECK(cond, rc) wxCHECK_MSG(cond, rc, (const char *)NULL)

#define wxFAIL_MSG(msg) assert(false)
#define wxT(x) x
