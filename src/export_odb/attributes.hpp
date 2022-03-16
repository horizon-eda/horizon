#pragma once
#include <string>

namespace horizon::ODB::attribute {

namespace detail {
std::string make_legal_string_attribute(const std::string &n);
}

template <typename T> struct attribute_name {
};

enum class Type { FLOAT, BOOLEAN, TEXT };

template <typename T, unsigned int n> struct float_attribute {
    double value;
    static constexpr unsigned int ndigits = n;
    static constexpr Type type = Type::FLOAT;
};

template <typename T> struct boolean_attribute {
    bool value = true;
    static constexpr Type type = Type::BOOLEAN;
};

template <typename T> struct text_attribute {
    text_attribute(const std::string &t) : value(detail::make_legal_string_attribute(t))
    {
    }
    std::string value;
    static constexpr Type type = Type::TEXT;
};

#define ATTR_NAME(n)                                                                                                   \
    template <> struct attribute_name<n> {                                                                             \
        static constexpr const char *name = "." #n;                                                                    \
    };

#define MAKE_FLOAT_ATTR(n, nd)                                                                                         \
    using n = float_attribute<struct n##_t, nd>;                                                                       \
    ATTR_NAME(n)

#define MAKE_TEXT_ATTR(n)                                                                                              \
    using n = text_attribute<struct n##_t>;                                                                            \
    ATTR_NAME(n)

#define MAKE_BOOLEAN_ATTR(n)                                                                                           \
    using n = boolean_attribute<struct n##_t>;                                                                         \
    ATTR_NAME(n)

template <typename T> struct is_feature : std::false_type {
};
template <typename T> struct is_net : std::false_type {
};
template <typename T> struct is_pkg : std::false_type {
};

template <class T> inline constexpr bool is_feature_v = is_feature<T>::value;
template <class T> inline constexpr bool is_net_v = is_net<T>::value;
template <class T> inline constexpr bool is_pkg_v = is_pkg<T>::value;

#define ATTR_IS_FEATURE(a)                                                                                             \
    template <> struct is_feature<a> : std::true_type {                                                                \
    };

#define ATTR_IS_NET(a)                                                                                                 \
    template <> struct is_net<a> : std::true_type {                                                                    \
    };

#define ATTR_IS_PKG(a)                                                                                                 \
    template <> struct is_pkg<a> : std::true_type {                                                                    \
    };

enum class drill { PLATED, NON_PLATED, VIA };
ATTR_NAME(drill)
ATTR_IS_FEATURE(drill)

enum class primary_side { TOP, BOTTOM };
ATTR_NAME(primary_side)

enum class pad_usage { TOEPRINT, VIA, G_FIDUCIAL, L_FIDUCIAL, TOOLING_HOLE };
ATTR_NAME(pad_usage)
ATTR_IS_FEATURE(pad_usage)

MAKE_FLOAT_ATTR(drc_max_height, 3)
ATTR_IS_FEATURE(drc_max_height)

MAKE_BOOLEAN_ATTR(smd)
ATTR_IS_FEATURE(smd)

MAKE_TEXT_ATTR(electrical_class)
ATTR_IS_NET(electrical_class)

MAKE_TEXT_ATTR(net_type)
ATTR_IS_NET(net_type)

MAKE_TEXT_ATTR(diff_pair)
ATTR_IS_NET(diff_pair)

MAKE_TEXT_ATTR(string)
ATTR_IS_FEATURE(string)

} // namespace horizon::ODB::attribute
