#pragma once

namespace arx
{
	// A pointer that can't be default initialized.
	// It's NOT a smart pointer.
	// It's NOT a not_null pointer.
	// It's purpose is to avoid unexpected nullptr initialization, but it can be set as nullptr explicitly.
	template<typename E>
	struct ptr 
	{
	public:
		using element_type = E;

		ptr() = delete;
		constexpr ptr(E* raw) noexcept : raw(raw) {}
		constexpr ptr(const ptr& other) noexcept : raw(other.raw) {}
		constexpr ptr(ptr&& other) noexcept : raw(other.raw) {
			other.raw = nullptr;
		}
		constexpr ptr& operator=(const ptr& other) noexcept {
			raw = other.raw;
			return *this;
		}
		constexpr ptr& operator=(ptr&& other) noexcept {
			raw = other.raw;
			other.raw = nullptr;
			return *this;
		}
		template<typename F>
		constexpr ptr(const ptr<F>& other) noexcept : raw(other.raw) {}
		template<typename F>
		constexpr ptr(ptr<F>&& other) noexcept : raw(other.raw) {
			other.raw = nullptr;
		}
		template<typename F>
		constexpr ptr& operator=(const ptr<F>& other) noexcept {
			raw = other.raw;
			return *this;
		}
		template<typename F>
		constexpr ptr& operator=(ptr<F>&& other) noexcept {
			raw = other.raw;
			other.raw = nullptr;
			return *this;
		}

		constexpr operator E* () const noexcept {
			return raw;
		}
		constexpr E* operator->() const noexcept {
			return raw;
		}
		constexpr E& operator*() const noexcept {
			return *raw;
		}

		constexpr bool operator==(const ptr& other) const noexcept {
			return raw == other.raw;
		}
		constexpr bool operator!=(const ptr& other) const noexcept {
			return raw != other.raw;
		}
		constexpr bool operator==(E* other) const noexcept {
			return raw == other;
		}
		constexpr bool operator!=(E* other) const noexcept {
			return raw != other;
		}

		constexpr operator bool() const noexcept {
			return raw != nullptr;
		}

	public:
		E* raw;
	};
}