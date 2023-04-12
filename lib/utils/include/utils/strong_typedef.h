#ifndef _FLEXFLOW_UTILS_INCLUDE_STRONG_TYPEDEF_H
#define _FLEXFLOW_UTILS_INCLUDE_STRONG_TYPEDEF_H

#include <type_traits>
#include <functional>
#include "utils/fmt.h"
#include <string>

namespace FlexFlow {

// derived from https://www.foonathan.net/2016/10/strong-typedefs/
template <typename Tag, typename T>
class strong_typedef {
public:
    strong_typedef() = delete;

    explicit strong_typedef(const T& value) 
      : value_(value)
    { }

    explicit strong_typedef(T&& value)
        noexcept(std::is_nothrow_move_constructible<T>::value)
      : value_(std::move(value))
    { }

    explicit operator T&() noexcept
    {
        return value_;
    }

    explicit operator const T&() const noexcept
    {
        return value_;
    }

    friend void swap(strong_typedef& a, strong_typedef& b) noexcept
    {
        using std::swap;
        swap(static_cast<T&>(a), static_cast<T&>(b));
    }

    friend bool operator==(strong_typedef const &lhs, strong_typedef const &rhs) {
      return lhs.value() == rhs.value();
    }

    friend bool operator!=(strong_typedef const &lhs, strong_typedef const &rhs) {
      return lhs.value() != rhs.value();
    }

    friend bool operator<(strong_typedef const &lhs, strong_typedef const &rhs) {
      return lhs.value() < rhs.value();
    }

    T const &value() const noexcept {
      return value_;
    }

private:
    T value_;
};

template <typename Tag, typename T>
T underlying_type_impl(strong_typedef<Tag, T>);

template <typename T>
using underlying_type = decltype(underlying_type_impl(std::declval<T>()));

// derived from https://github.com/foonathan/type_safe/blob/3612e2828b4b4e0d1cc689373e63a6d59d4bfd79/include/type_safe/strong_typedef.hpp
template <typename StrongTypedef>
struct hashable : std::hash<underlying_type<StrongTypedef>> {
    using underlying_ty = underlying_type<StrongTypedef>;
    using underlying_hash = std::hash<underlying_ty>;

    std::size_t operator()(const StrongTypedef& lhs) const
        noexcept(noexcept(underlying_hash{}(std::declval<underlying_ty>())))
    {
        return underlying_hash{}(static_cast<const underlying_ty&>(lhs));
    }
};
}

#define MAKE_TYPEDEF_HASHABLE(TYPEDEF_NAME) \
  namespace std { \
    template <> \
    struct hash<TYPEDEF_NAME> : ::FlexFlow::hashable<TYPEDEF_NAME> { }; \
  } \
  static_assert(true, "")

#define MAKE_TYPEDEF_PRINTABLE(TYPEDEF_NAME, TYPEDEF_SHORTNAME) \
  namespace fmt { \
    template <> \
    struct formatter<TYPEDEF_NAME> : formatter<::std::string> { \
      template <typename FormatContext> \
      auto format(TYPEDEF_NAME const &x, FormatContext &ctx) const -> decltype(ctx.out()) { \
        ::std::string s = fmt::format("{}({})", (TYPEDEF_SHORTNAME), x.value()); \
        return formatter<::std::string>::format(s, ctx); \
      } \
    }; \
  } \
  static_assert(true, "")

#endif
