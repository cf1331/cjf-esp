#ifndef C70C5F4B_F66C_4ABC_A02F_ADA88FD3DE79
#define C70C5F4B_F66C_4ABC_A02F_ADA88FD3DE79

/**
 * @file expected_resource.h
 * @brief Wrapper for std::expected containing unique_resource with improved ergonomics
 *
 * This file provides a template wrapper that simplifies working with
 * `std::expected<unique_resource<T, D>, E>` by providing direct access to the
 * underlying resource via `operator->` and `operator*`.
 */

#include <expected>
#include <experimental/scope>
#include <utility>

namespace cjf
{
  /**
   * @brief Wrapper for std::expected<unique_resource<T, D>, E> with direct resource access
   *
   * This class simplifies working with expected values that contain a unique_resource
   * by automatically unwrapping the resource when using `operator->` and `operator*`.
   * It maintains compatibility with `std::expected` while providing more ergonomic
   * access to the underlying resource value.
   *
   * @tparam T The resource type managed by the unique_resource
   * @tparam Deleter The deleter type for the unique_resource
   * @tparam E The error type for the expected value
   */
  template <typename T, typename Deleter, typename E>
  class expected_resource
  {
  public:
    /// Type of the unique_resource wrapper
    using resource_type = std::experimental::unique_resource<T, Deleter>;
    /// Type of the underlying std::expected
    using expected_type = std::expected<resource_type, E>;
    /// The value type (same as T)
    using value_type = T;
    /// The error type
    using error_type = E;

    /**
     * @brief Construct from an rvalue std::expected
     * @param exp An rvalue reference to the expected value
     */
    constexpr expected_resource(expected_type &&exp) noexcept
        : expected_(std::move(exp)) {}

    /**
     * @brief Construct from a const std::expected reference
     * @param exp A const reference to the expected value
     */
    constexpr expected_resource(const expected_type &exp)
        : expected_(exp) {}

    /**
     * @brief Construct from an rvalue unique_resource
     * @param res An rvalue reference to the resource
     */
    constexpr expected_resource(resource_type &&res) noexcept
        : expected_(std::move(res)) {}

    /**
     * @brief Construct from an error value
     * @param unexp An unexpected error value
     */
    constexpr expected_resource(std::unexpected<E> unexp) noexcept
        : expected_(std::move(unexp)) {}

    /**
     * @brief Construct a unique_resource directly from resource and deleter
     * @param resource The resource value
     * @param deleter The deleter for the resource
     */
    constexpr expected_resource(T &&resource, Deleter &&deleter) noexcept(
        noexcept(resource_type(std::move(resource), std::move(deleter))))
        : expected_(std::in_place, std::move(resource), std::move(deleter)) {}

    /// Move constructor - transfers ownership of the resource
    expected_resource(expected_resource &&) noexcept = default;
    /// Move assignment - transfers ownership of the resource
    expected_resource &operator=(expected_resource &&) noexcept = default;

    // Delete copy operations (unique_resource is move-only)
    expected_resource(const expected_resource &) = delete;
    expected_resource &operator=(const expected_resource &) = delete;

    /**
     * @brief Check if the expected contains a value
     * @return true if the expected contains a valid value, false if it contains an error
     */
    constexpr bool has_value() const noexcept { return expected_.has_value(); }
    /**
     * @brief Boolean conversion operator
     * @return true if the expected contains a value, false if it contains an error
     */
    constexpr explicit operator bool() const noexcept { return has_value(); }

    /**
     * @brief Get a reference to the underlying resource (unique_resource wrapper)
     * @return Reference to the resource wrapper
     */
    constexpr resource_type &value() & { return expected_.value(); }
    /**
     * @brief Get a const reference to the underlying resource
     * @return Const reference to the resource wrapper
     */
    constexpr const resource_type &value() const & { return expected_.value(); }
    /**
     * @brief Get an rvalue reference to the underlying resource
     * @return Rvalue reference to the resource wrapper
     */
    constexpr resource_type &&value() && { return std::move(expected_).value(); }
    /**
     * @brief Get a const rvalue reference to the underlying resource
     * @return Const rvalue reference to the resource wrapper
     */
    constexpr const resource_type &&value() const && { return std::move(expected_).value(); }

    /**
     * @brief Get a const reference to the error value
     * @return Const reference to the error
     */
    constexpr const E &error() const & noexcept { return expected_.error(); }
    /**
     * @brief Get a mutable reference to the error value
     * @return Mutable reference to the error
     */
    constexpr E &error() & noexcept { return expected_.error(); }
    /**
     * @brief Get a const rvalue reference to the error value
     * @return Const rvalue reference to the error
     */
    constexpr const E &&error() const && noexcept { return std::move(expected_).error(); }
    /**
     * @brief Get an rvalue reference to the error value
     * @return Rvalue reference to the error
     */
    constexpr E &&error() && noexcept { return std::move(expected_).error(); }

    /**
     * @brief Direct access to the underlying resource T via pointer semantics
     *
     * `unique_resource::get()` only provides `const R&`. Since the expected_resource
     * owns the unique_resource, `const_cast` is safe for the non-const overload.
     *
     * @return Pointer to the resource if value present, nullptr otherwise
     */
    constexpr T *operator->() noexcept
    {
      return expected_.has_value() ? const_cast<T *>(&expected_.value().get()) : nullptr;
    }

    /**
     * @brief Direct access to the underlying resource T via const pointer semantics
     * @return Const pointer to the resource if value present, nullptr otherwise
     */
    constexpr const T *operator->() const noexcept
    {
      return expected_.has_value() ? &expected_.value().get() : nullptr;
    }

    /**
     * @brief Direct access to the underlying resource T via reference semantics
     * @return Reference to the resource (lvalue)
     */
    constexpr T &operator*() & noexcept {
      return const_cast<T &>(expected_.value().get());
    }
    /**
     * @brief Direct access to the underlying resource T via const reference semantics
     * @return Const reference to the resource (lvalue)
     */
    constexpr const T &operator*() const & noexcept {
      return expected_.value().get();
    }
    /**
     * @brief Direct access to the underlying resource T via rvalue reference semantics
     * @return Rvalue reference to the resource
     */
    constexpr T &&operator*() && noexcept {
      return std::move(const_cast<T &>(expected_.value().get()));
    }
    /**
     * @brief Direct access to the underlying resource T via const rvalue reference semantics
     * @return Const rvalue reference to the resource
     */
    constexpr const T &&operator*() const && noexcept {
      return std::move(expected_.value().get());
    }

    /**
     * @brief Return the resource value or a default value if no value present
     * @tparam U Type of the default value
     * @param default_value Value to return if this contains an error
     * @return The resource if value present, otherwise the default value converted to T
     */
    template <typename U>
    constexpr T value_or(U &&default_value) const &
    {
      return has_value() ? **this : static_cast<T>(std::forward<U>(default_value));
    }

    /**
     * @brief Return the resource value (as rvalue) or a default value if no value present
     * @tparam U Type of the default value
     * @param default_value Value to return if this contains an error
     * @return The resource moved if value present, otherwise the default value converted to T
     */
    template <typename U>
    constexpr T value_or(U &&default_value) &&
    {
      return has_value() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
    }

  private:
    /// The underlying std::expected value containing the resource
    expected_type expected_;
  };

} // namespace cjf

#endif /* C70C5F4B_F66C_4ABC_A02F_ADA88FD3DE79 */
