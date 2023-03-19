#pragma once
#include <string>
#include "clipper/clipper.hpp"
#include <cstdint>
#include <range/v3/view.hpp>
#include <range/v3/algorithm.hpp>

namespace horizon {
std::string get_net_name(const class Net *net);

ClipperLib::IntRect get_patch_bb(const ClipperLib::Paths &patch);
bool bbox_test_overlap(const ClipperLib::IntRect &bb1, const ClipperLib::IntRect &bb2, uint64_t clearance);
void format_progress(std::ostringstream &oss, size_t i, size_t n);


template <typename... ArgsGetRule, typename Tfn, typename... ArgsGetClearance>
auto find_clearance(const class BoardRules &rules, Tfn fn, const std::set<int> &layers,
                    std::tuple<ArgsGetRule...> args_get_rule, std::tuple<ArgsGetClearance...> args_get_clearance)
{

    return ranges::min(layers | ranges::views::transform([&rules, fn, &args_get_clearance, &args_get_rule](int layer) {
                           auto &rule = std::apply(
                                   [&rules, fn, layer ](auto &&...args) -> const auto & {
                                       return std::invoke(fn, rules, args..., layer);
                                   },
                                   args_get_rule);

                           return std::apply([&rule](auto &&...args) { return rule.get_clearance(args...); },
                                             args_get_clearance);
                       }));
}

} // namespace horizon
