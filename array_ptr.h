#pragma once

#include <cstddef>
#include <utility>

template <typename Type>
class ArrayPtr {
public:
    ArrayPtr() noexcept = default;

    explicit ArrayPtr(size_t size)
        : data_(size > 0 ? new Type[size]() : nullptr) {
    }

    ArrayPtr(const ArrayPtr&) = delete;
    ArrayPtr& operator=(const ArrayPtr&) = delete;

    ArrayPtr(ArrayPtr&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)) {
    }

    ArrayPtr& operator=(ArrayPtr&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = std::exchange(other.data_, nullptr);
        }
        return *this;
    }

    ~ArrayPtr() {
        delete[] data_;
    }

    Type* Get() const noexcept {
        return data_;
    }

    Type* Release() noexcept {
        return std::exchange(data_, nullptr);
    }

    void Reset(Type* ptr = nullptr) noexcept {
        if (data_ != ptr) {
            delete[] data_;
            data_ = ptr;
        }
    }

    void Swap(ArrayPtr& other) noexcept {
        std::swap(data_, other.data_);
    }

    Type& operator[](size_t index) noexcept {
        return data_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return data_[index];
    }

    explicit operator bool() const noexcept {
        return data_ != nullptr;
    }

private:
    Type* data_ = nullptr;
};
