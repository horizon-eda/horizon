#pragma once
#include <git2.h>
#include <type_traits>

namespace horizon::git_util {
template <typename T, typename = void> struct git_commit_needs_const : std::false_type {};

template <typename T>
struct git_commit_needs_const<
        T, std::void_t<decltype(git_commit_create(std::declval<git_oid *>(), std::declval<git_repository *>(),
                                                  std::declval<const char *>(), std::declval<const git_signature *>(),
                                                  std::declval<const git_signature *>(), std::declval<const char *>(),
                                                  std::declval<const char *>(), std::declval<const git_tree *>(),
                                                  std::declval<size_t>(), std::declval<const T **>()))>>
    : std::true_type {};

using git_commit_maybe_const =
        std::conditional_t<git_commit_needs_const<git_commit>::value, const git_commit, git_commit>;


} // namespace horizon::git_util
