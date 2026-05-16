#ifndef YUZU_UTIL_SIDE_TABLE_H
#define YUZU_UTIL_SIDE_TABLE_H

#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace yuzu::util {

/// Vector-indexed `Id → Value` table. `Id` must be either an integer type
/// or a strong-typedef'd `enum class` with an integer underlying type;
/// `Value` is stored by value. Unbound slots return `std::nullopt`.
template <typename Id, typename Value> class SideTable {
public:
  void bind(Id id, Value value) {
    const auto index = toIndex(id);
    if (index >= entries.size()) {
      entries.resize(index + 1);
    }
    entries[index] = std::move(value);
  }

  [[nodiscard]] const Value *get(Id id) const {
    const auto index = toIndex(id);
    if (index >= entries.size()) {
      return nullptr;
    }
    const auto &entry = entries[index];
    return entry.has_value() ? &*entry : nullptr;
  }

private:
  static std::size_t toIndex(Id id) {
    if constexpr (std::is_enum_v<Id>) {
      return static_cast<std::size_t>(
          static_cast<std::underlying_type_t<Id>>(id));
    } else {
      return static_cast<std::size_t>(id);
    }
  }

  std::vector<std::optional<Value>> entries;
};

} // namespace yuzu::util

#endif // YUZU_UTIL_SIDE_TABLE_H
