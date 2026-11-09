#pragma once

#include "array_ptr.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <utility>

struct ReserveTag {
    explicit ReserveTag(size_t capacity)
        : capacity(capacity) {
    }

    size_t capacity;
};

inline ReserveTag Reserve(size_t capacity) {
    return ReserveTag(capacity);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size) : data_(size),
    size_(size), capacity_(size) {
    }

    explicit SimpleVector(ReserveTag reserve) : data_(reserve.capacity), 
    capacity_(reserve.capacity) {
    }

    SimpleVector(size_t size, const Type& value) : SimpleVector(size) {
        std::fill(begin(), end(), value);
    }

    SimpleVector(std::initializer_list<Type> init) : SimpleVector(init.size()) {
        std::copy(init.begin(), init.end(), begin());
    }

    SimpleVector(const SimpleVector& other) : data_(other.size_),
    size_(other.size_), capacity_(other.size_) {
        std::copy(other.begin(), other.end(), begin());
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector temp(rhs);
            swap(temp);
        }
        return *this;
    }

    SimpleVector(SimpleVector&& other) noexcept: data_(std::move(other.data_)), 
    size_(std::exchange(other.size_, 0)), capacity_(std::exchange(other.capacity_, 0)) {
    }

    SimpleVector& operator=(SimpleVector&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
        }
        return *this;
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    Type& operator[](size_t index) noexcept {
        return data_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return data_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }

        ArrayPtr<Type> new_data(new_capacity);

        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move_if_noexcept(data_[i]);
        }

        data_.Swap(new_data);
        capacity_ = new_capacity;
    }

    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
            return;
        }

        if (new_size <= capacity_) {
            std::generate(
                begin() + size_,
                begin() + new_size,
                [] {
                    return Type{};
                }
            );

            size_ = new_size;
            return;
        }

        ArrayPtr<Type> new_data(new_size);

        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move_if_noexcept(data_[i]);
        }

        data_.Swap(new_data);
        size_ = new_size;
        capacity_ = new_size;
    }

    void PushBack(const Type& item) {
        if (size_ < capacity_) {
            data_[size_] = item;
            ++size_;
            return;
        }

        const size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;

        ArrayPtr<Type> new_data(new_capacity);

        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move_if_noexcept(data_[i]);
        }

        new_data[size_] = item;

        data_.Swap(new_data);
        ++size_;
        capacity_ = new_capacity;
    }

    void PushBack(Type&& item) {
        if (size_ < capacity_) {
            data_[size_] = std::move(item);
            ++size_;
            return;
        }

        Type value(std::move(item));

        const size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;

        ArrayPtr<Type> new_data(new_capacity);

        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move_if_noexcept(data_[i]);
        }

        new_data[size_] = std::move(value);

        data_.Swap(new_data);
        ++size_;
        capacity_ = new_capacity;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        Type value_copy(value);
        return InsertImpl(pos, std::move(value_copy));
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        Type value_copy(std::move(value));
        return InsertImpl(pos, std::move(value_copy));
    }

    void PopBack() noexcept {
        assert(size_ > 0);
        --size_;
    }

    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos < end());

        const size_t index =
            static_cast<size_t>(pos - begin());

        std::move(
            begin() + index + 1, end(),
            begin() + index
        );

        --size_;

        return begin() + index;
    }

    void swap(SimpleVector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    Iterator begin() noexcept {
        return data_.Get();
    }

    Iterator end() noexcept {
        return data_ ? data_.Get() + size_ : nullptr;
    }

    ConstIterator begin() const noexcept {
        return data_.Get();
    }

    ConstIterator end() const noexcept {
        return data_ ? data_.Get() + size_ : nullptr;
    }

    ConstIterator cbegin() const noexcept {
        return begin();
    }

    ConstIterator cend() const noexcept {
        return end();
    }

private:
    Iterator InsertImpl(ConstIterator pos, Type&& value) {
        const size_t index = size_ == 0? 0 : static_cast<size_t>(pos - begin());

        if (size_ == capacity_) {
            const size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;

            ArrayPtr<Type> new_data(new_capacity);

            for (size_t i = 0; i < index; ++i) {
                new_data[i] = std::move_if_noexcept(data_[i]);
            }

            new_data[index] = std::move(value);

            for (size_t i = index; i < size_; ++i) {
                new_data[i + 1] = std::move_if_noexcept(data_[i]);
            }

            data_.Swap(new_data);
            capacity_ = new_capacity;
        } else {
            std::move_backward( begin() + index, end(), end() + 1
            );

            data_[index] = std::move(value);
        }

        ++size_;

        return begin() + index;
    }

    ArrayPtr<Type> data_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(
    const SimpleVector<Type>& lhs,
    const SimpleVector<Type>& rhs
) {
    return lhs.GetSize() == rhs.GetSize()
        && std::equal(
            lhs.begin(),
            lhs.end(),
            rhs.begin()
        );
}

template <typename Type>
inline bool operator!=(
    const SimpleVector<Type>& lhs,
    const SimpleVector<Type>& rhs
) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(
    const SimpleVector<Type>& lhs,
    const SimpleVector<Type>& rhs
) {
    return std::lexicographical_compare(
        lhs.begin(),
        lhs.end(),
        rhs.begin(),
        rhs.end()
    );
}

template <typename Type>
inline bool operator<=(
    const SimpleVector<Type>& lhs,
    const SimpleVector<Type>& rhs
) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(
    const SimpleVector<Type>& lhs,
    const SimpleVector<Type>& rhs
) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(
    const SimpleVector<Type>& lhs,
    const SimpleVector<Type>& rhs
) {
    return !(lhs < rhs);
}
