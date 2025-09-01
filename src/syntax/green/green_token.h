#ifndef SYNTAX_GREEN_GREEN_TOKEN_H_
#define SYNTAX_GREEN_GREEN_TOKEN_H_

#include <memory>
#include <string>

#include "syntax/syntax_kind.h"

namespace yuzu::syntax {
class GreenTokenData {
 public:
  explicit GreenTokenData(const SyntaxKind kind, const std::u32string source)
      : kind_(kind), source_(source) {}

  GreenTokenData() = delete;

  [[nodiscard]] SyntaxKind Kind() const noexcept { return kind_; }

  [[nodiscard]] std::u32string_view Source() const noexcept { return source_; }

  bool operator==(const GreenTokenData& other) const noexcept {
    return kind_ == other.kind_ && source_ == other.source_;
  }

 private:
  const SyntaxKind kind_;
  const std::u32string source_;
};

class GreenToken {
 public:
  explicit GreenToken(const SyntaxKind kind, const std::u32string source)
      : data_(std::make_shared<GreenTokenData>(GreenTokenData(kind, source))) {}

  GreenToken() = delete;

  [[nodiscard]] SyntaxKind Kind() const noexcept { return data_->Kind(); }

  [[nodiscard]] std::u32string_view Source() const noexcept {
    return data_->Source();
  }

  [[nodiscard]] size_t UseCount() const noexcept { return data_.use_count(); }

  bool operator==(const GreenToken& other) const noexcept {
    return data_ == other.data_;
  }

 private:
  const std::shared_ptr<GreenTokenData> data_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_GREEN_GREEN_TOKEN_H_
