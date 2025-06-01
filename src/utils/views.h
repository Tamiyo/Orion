#ifndef UTILS_VIEWS_H_
#define UTILS_VIEWS_H_

#include <algorithm>
#include <concepts>
#include <functional>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

namespace yuzu::utils {
template <typename>
struct IsOptional : std::false_type {};

template <typename T>
struct IsOptional<std::optional<T>> : std::true_type {};

template <typename T>
inline constexpr bool IsOptionalValue = IsOptional<T>::value;

template <typename Pred, typename Arg>
concept ReturnsOptional = requires(Pred pred, Arg arg) {
  { pred(arg) }
  ->std::same_as<std::invoke_result_t<Pred, Arg>>;
  requires IsOptionalValue<std::invoke_result_t<Pred, Arg>>;
};

template <typename Pred>
class FilterMapAdaptor {
 public:
  constexpr explicit FilterMapAdaptor(Pred pred) : pred_(std::move(pred)) {}
  constexpr FilterMapAdaptor() = delete;

  template <std::ranges::viewable_range R>
  requires ReturnsOptional<Pred,
                           std::ranges::range_reference_t<R>> constexpr auto
  operator()(R&& range) const {
    return std::forward<R>(range) |
           std::views::transform([pred = pred_](auto&& x) {
             return pred(std::forward<decltype(x)>(x));
           }) |
           std::views::filter([](auto&& x) { return x.has_value(); }) |
           std::views::transform([](auto&& x) { return x.value(); });
  }

  template <std::ranges::viewable_range R>
  requires ReturnsOptional<
      Pred, std::ranges::range_reference_t<R>> friend constexpr auto
  operator|(R&& range, const FilterMapAdaptor& adaptor) {
    return adaptor(std::forward<R>(range));
  }

 private:
  const Pred pred_;
};

template <typename Pred>
class FindAdaptor {
 public:
  constexpr explicit FindAdaptor(Pred pred) : pred_(std::move(pred)) {}
  constexpr FindAdaptor() = delete;

  template <std::ranges::viewable_range R>
  auto operator()(R&& range) const
      -> std::optional<std::ranges::range_value_t<R>> {
    auto it = std::ranges::find_if(range, pred_);
    if (it != std::ranges::end(range)) {
      return *it;
    }
    return std::nullopt;
  }

  template <std::ranges::viewable_range R>
  friend auto operator|(R&& range, const FindAdaptor& adaptor)
      -> std::optional<std::ranges::range_value_t<R>> {
    return adaptor(std::forward<R>(range));
  }

 private:
  const Pred pred_;
};

class NthItemAdaptor {
 public:
  constexpr explicit NthItemAdaptor(const std::size_t nth) : nth_(nth) {}
  constexpr NthItemAdaptor() = delete;

  template <std::ranges::input_range R>
  requires std::ranges::sized_range<R> constexpr auto operator()(
      R&& range) const -> std::optional<std::ranges::range_value_t<R>> {
    if (nth_ >= std::ranges::size(range)) {
      return std::nullopt;
    }

    auto it = std::ranges::begin(range);
    std::advance(it, nth_);
    if (it != std::ranges::end(range)) {
      return *it;
    }
    return std::nullopt;
  }

  template <std::ranges::input_range R>
  friend auto operator|(R&& range, const NthItemAdaptor& adaptor)
      -> std::optional<std::ranges::range_value_t<R>> {
    return adaptor(std::forward<R>(range));
  }

 private:
  const std::size_t nth_;
};

inline constexpr auto FilterMap = []<typename Pred>(Pred pred) {
  return FilterMapAdaptor(std::move(pred));
};

inline constexpr auto Filter = std::views::filter;

inline constexpr auto Find = []<typename Pred>(Pred pred) {
  return FindAdaptor(std::move(pred));
};

inline constexpr auto Map = std::views::transform;

inline constexpr auto Nth = [](const std::size_t nth) {
  return NthItemAdaptor(nth);
};

}  // namespace yuzu::utils

#endif  // UTILS_VIEWS_H_
