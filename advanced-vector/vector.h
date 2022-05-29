#pragma once

#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    RawMemory(const RawMemory&) = delete;

    explicit RawMemory(size_t capacity) : buffer_(Allocate(capacity)), capacity_(capacity) {}
    RawMemory(RawMemory&& other) noexcept {
        this->Allocate(other.capacity_);
        std::swap(capacity_, other.capacity_);
        std::swap(buffer_, other.buffer_);
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            buffer_ = std::move(rhs.buffer_);
            capacity_ = std::move(rhs.capacity_);
        }
        return *this;
    }

    RawMemory& operator=(const RawMemory& rhs) = delete;

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;
    explicit Vector(size_t size);
    Vector(const Vector& other);
    Vector(Vector&& other) noexcept;

    size_t Size() const noexcept;
    size_t Capacity() const noexcept;

    void Resize(size_t new_size);
    T& PushBack(const T& value);
    T& PushBack(T&& value);
    void PopBack();
    void Swap(Vector& other) noexcept;
    void Reserve(size_t new_capacity);
    template <typename... Args>
    T& EmplaceBack(Args&&... args);
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args);
    iterator Erase(const_iterator pos);
    iterator Insert(const_iterator pos, const T& value);
    iterator Insert(const_iterator pos, T&& value);

    // Вызывает деструкторы n объектов массива по адресу buf
    void DestroyN(T* buf, size_t n) noexcept;
    // Создаёт копию объекта elem в сырой памяти по адресу buf
    void CopyConstruct(T* buf, const T& elem);
    // Вызывает деструктор объекта по адресу buf
    void Destroy(T* buf) noexcept;

    Vector& operator=(const Vector& rhs);
    Vector& operator=(Vector&& rhs) noexcept;
    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    ~Vector();

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};

template<typename T>
Vector<T>::Vector(size_t size)
    : data_(size)
    , size_(size)  
{
    std::uninitialized_value_construct_n(data_.GetAddress(), size);
}

template<typename T>
Vector<T>::Vector(const Vector& other)
    : data_(other.size_)
    , size_(other.size_)
{
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }
    else {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }
}

template<typename T>
Vector<T>::Vector(Vector&& other) noexcept {
    std::swap(size_, other.size_);
    data_.Swap(other.data_);
}

template<typename T>
size_t Vector<T>::Size() const noexcept {
    return size_;
}

template<typename T>
size_t Vector<T>::Capacity() const noexcept {
    return data_.Capacity();
}

template<typename T>
void Vector<T>::Resize(size_t new_size) {
    if (new_size > size_) {
        Reserve(new_size);
        std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
    }
    if (new_size < size_) {
        std::destroy(data_.GetAddress() + new_size, data_.GetAddress() + size_);
    }
    size_ = new_size;
}

template<typename T>
T& Vector<T>::PushBack(const T& value) {
    return EmplaceBack(value);
}

template<typename T>
T& Vector<T>::PushBack(T&& value) {
    return EmplaceBack(std::move(value));
}

template<typename T>
void Vector<T>::PopBack() {
    std::destroy_n(data_.GetAddress() + size_ - 1, 1);
    --size_;
}

// Вызывает деструкторы n объектов массива по адресу buf
template<typename T>
void Vector<T>::DestroyN(T* buf, size_t n) noexcept {
    for (size_t i = 0; i != n; ++i) {
        Destroy(buf + i);
    }
}

// Создаёт копию объекта elem в сырой памяти по адресу buf
template<typename T>
void Vector<T>::CopyConstruct(T* buf, const T & elem) {
    new (buf) T(elem);
}

// Вызывает деструктор объекта по адресу buf
template<typename T>
void Vector<T>::Destroy(T* buf) noexcept {
    buf->~T();
}

template<typename T>
void Vector<T>::Swap(Vector& other) noexcept {
    std::swap(size_, other.size_);
    data_.Swap(other.data_);
}

template<typename T>
void Vector<T>::Reserve(size_t new_capacity) {
    if (new_capacity <= data_.Capacity()) {
        return;
    }
    RawMemory<T> new_data(new_capacity);
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
    }
    else {
        std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
    }
    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}

template <typename T>
template <typename... Args>
T& Vector<T>::EmplaceBack(Args&&... args) {
    if (size_ == this->Capacity()) {
        size_t capacity_tmp = 0;
        size_ == 0 ? capacity_tmp += 1 : capacity_tmp += size_ * 2;

        RawMemory<T> new_data(capacity_tmp);
        new (new_data + size_) T(std::forward<Args>(args)...);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    else {
        new (data_ + size_) T(std::forward<Args>(args)...);
    }
    ++size_;
    return data_[size_ - 1];
}

template<typename T>
Vector<T>& Vector<T>::operator=(const Vector<T>& rhs) {
    if (this != &rhs) {
        if (rhs.size_ > data_.Capacity()) {
            Vector temp_data(rhs);
            Swap(temp_data);
        }
        else {
            if (rhs.size_ < size_) {
                for (size_t part = 0; part < rhs.size_; ++part) {
                    data_[part] = rhs.data_[part];
                }
                std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
            }
            else {
                for (size_t ptr = 0; ptr < size_; ++ptr) {
                    data_[ptr] = rhs.data_[ptr];
                }
                std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
            }
        }
    }
    size_ = rhs.size_;
    return *this;
}

template<typename T>
Vector<T>& Vector<T>::operator=(Vector&& rhs) noexcept {
    Swap(rhs);
    return *this;
}

template<typename T>
const T& Vector<T>::operator[](size_t index) const noexcept {
    return const_cast<Vector&>(*this)[index];
}

template<typename T>
T& Vector<T>::operator[](size_t index) noexcept {
    assert(index < size_);
    return data_[index];
}

template<typename T>
typename Vector<T>::iterator Vector<T>::begin() noexcept {
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::iterator Vector<T>::end() noexcept {
    return data_ + size_;
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::begin() const noexcept {
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::end() const noexcept {
    return data_ + size_;
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept {
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::cend() const noexcept {
    return data_ + size_;
}

template<typename T>
template <typename... Args>
typename Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Args&&... args) {
    size_t result = pos - begin();
    if (size_ == result) {
        this->EmplaceBack(std::forward<Args>(args)...);
    }
    else if (size_ < this->Capacity()) {
        T tmp = T(std::forward<Args>(args)...);
        new (data_ + size_) T(std::move(data_[size_ - 1u]));
        std::move_backward(data_.GetAddress() + result, end() - 1, end());
        data_[result] = std::move(tmp);
        ++size_;
    }
    else {
        RawMemory<T> new_data(size_ * 2);
        new(new_data + result) T(std::forward<Args>(args)...);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), result, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), result, new_data.GetAddress());
        }

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress() + result, size_ - result, new_data.GetAddress() + result + 1);
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress() + result, size_ - result, new_data.GetAddress() + result + 1);
        }

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
        ++size_;
    }

    return (data_.GetAddress() + result);
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Erase(const_iterator pos) {
    iterator pos_it = const_cast<iterator>(pos);
    std::move(pos_it + 1, end(), pos_it);
    std::destroy_n(data_.GetAddress() + (--size_), 1);
    return pos_it;
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, const T& value) {
    return Emplace(pos, value);
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, T&& value) {
    return Emplace(pos, std::move(value));
}

template<typename T>
Vector<T>::~Vector() {
    std::destroy(data_.GetAddress(), data_.GetAddress() + size_);
}
