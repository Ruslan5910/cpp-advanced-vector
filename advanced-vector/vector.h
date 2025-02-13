#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    
    RawMemory(const RawMemory&) = delete;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) 
    {
    }

    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept 
        : buffer_(std::move(other.buffer_)) 
        , capacity_(other.capacity_)
    {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }
    
    RawMemory& operator=(RawMemory&& other) noexcept {
        if (this != &other) {
            delete[] buffer_;
            Swap(other);
        }
        return *this;
    }

/*
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            buffer_->RawMemory();
            buffer_ = std::move(rhs.buffer_);
            capacity_ = rhs.capacity_;
            rhs.buffer_ = nullptr;
        }
        return *this; 
    }
*/
    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
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

    explicit Vector(size_t size)
        : data_(size)
        , size_(size) 
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }
    
    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {  
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept {
        Swap(other);
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                if (rhs.size_ < size_) {
                    //Размер вектора-источника меньше размера вектора-приёмника. 
                    //Тогда нужно скопировать имеющиеся элементы из источника в приёмник,
                    //а избыточные элементы вектора-приёмника разрушить:
                    for (size_t i = 0; i < rhs.size_; ++i) {   // size_ = 10 rhs = 7
                        data_[i] = rhs.data_[i];
                    }
                    for (size_t i = rhs.size_; i < size_; ++i) {
                        data_[i].~T();
                    }
                } else if (rhs.size_ >= size_) {
                    //Размер вектора-источника больше или равен размеру вектора-приёмника. 
                    //Тогда нужно присвоить существующим элементам приёмника значения соответствующих элементов источника,
                    // а оставшиеся скопировать в свободную область, используя функцию uninitialized_copy 
                    //или uninitialized_copy_n.
                    for (size_t i = 0; i < size_; ++i) {
                        data_[i] = rhs.data_[i];
                    }
                    std::uninitialized_copy_n(rhs.data_ + size_, rhs.size_ - size_, data_ + size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        //    data_ = std::move(rhs.data_);
        //    size_ = rhs.size_;
        }
        return *this;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }
    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }
    
    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }
    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }
    
    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);

        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }

        std::destroy_n(data_.GetAddress(), size_);

        data_.Swap(new_data);
    }
    
    void Resize(size_t new_size) {
    //При уменьшении размера вектора нужно удалить лишние элементы вектора, вызвав их деструкторы, а затем изменить размер вектора:
        if (new_size < size_) { 
        //    std::destroy_n(data_ + new_size, size_ - new_size);
            for (size_t i = new_size; i < size_; ++i) {
                data_[i].~T();
            }
        } else if (new_size > size_) {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        size_ = new_size;
    }

    void PushBack(const T& value) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(value);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else {
            new (data_ + size_) T(value);
        }
        ++size_;
    }

    void PushBack(T&& value) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(std::move(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else {
            new (data_ + size_) T(std::move(value));
        }
        ++size_;
    }

    void PopBack() noexcept {
        data_[size_-1].~T();
        --size_;
    }
    
    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(std::forward<Args>(args) ...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else {
            new (data_ + size_) T(std::forward<Args>(args) ...);
        }
        ++size_;
        return data_[size_ - 1];
    }
    
    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        size_t index = pos - data_.GetAddress();
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + index) T(std::forward<Args>(args) ...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) { 
                try { 
                    std::uninitialized_move_n(data_.GetAddress(), index, new_data.GetAddress()); 
                } catch(...) { 
                    new_data[index].~T(); 
                    throw;                     
                } 
                try { 
                    std::uninitialized_move_n(data_ + index, size_ - index, new_data + (index + 1)); 
                } catch(...) { 
                    std::destroy_n(new_data.GetAddress(), index + 1); 
                    throw; 
                } 
            } else { 
                try { 
                    std::uninitialized_copy_n(data_.GetAddress(), index, new_data.GetAddress()); 
                } catch (...) { 
                    new_data[index].~T(); 
                    throw; 
                } 
                try { 
                    std::uninitialized_copy_n(data_ + index, size_ - index, new_data + (index + 1)); 
                } catch (...) { 
                    std::destroy_n(new_data.GetAddress(), index + 1); 
                    throw; 
                } 
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else { 
            T temp_value(std::forward<Args>(args)...);
            new (data_ + size_) T(std::move(data_[size_ - 1]));
            std::move_backward(data_ + index, data_ + size_ - 1, data_ + size_);
            data_[index] = std::forward<T>(temp_value);
        }
        ++size_;
        return data_ + index;
    }

    iterator Erase(const_iterator pos) {
        size_t index = pos - data_.GetAddress();
        std::move(data_ + (index + 1), data_ + size_, data_ + index);
        PopBack();
        return data_ + index; 
    }
    
    iterator Insert(const_iterator pos, const T& value) {
        size_t index = pos - data_.GetAddress();
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + index) T(std::move(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) { 
                try { 
                    std::uninitialized_move_n(data_.GetAddress(), index, new_data.GetAddress()); 
                } catch(...) { 
                    new_data[index].~T(); 
                    throw;                     
                } 
                try { 
                    std::uninitialized_move_n(data_ + index, size_ - index, new_data + (index + 1)); 
                } catch(...) { 
                    std::destroy_n(new_data.GetAddress(), index + 1); 
                    throw; 
                } 
            } else { 
                try { 
                    std::uninitialized_copy_n(data_.GetAddress(), index, new_data.GetAddress()); 
                } catch (...) { 
                    new_data[index].~T(); 
                    throw; 
                } 
                try { 
                    std::uninitialized_copy_n(data_ + index, size_ - index, new_data + (index + 1)); 
                } catch (...) { 
                    std::destroy_n(new_data.GetAddress(), index + 1); 
                    throw; 
                } 
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else { 
            T temp_value(value);
            new (data_ + size_) T(std::move(data_[size_ - 1]));
            std::move_backward(data_ + index, data_ + size_ - 1, data_ + size_);
            data_[index] = std::move(temp_value);
        }
        ++size_;
        return data_ + index;
    }

    iterator Insert(const_iterator pos, T&& value) {
        size_t index = pos - data_.GetAddress();
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + index) T(std::move(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                try {
                std::uninitialized_move_n(begin(), index, new_data.GetAddress());
                    std::uninitialized_move_n(begin() + index, size_ - index, new_data.GetAddress() + index + 1);
                } catch (...) {
                    std::destroy_n(new_data.GetAddress() + index, 1);
                    throw;
                }
            } else {
                try {
                    std::uninitialized_copy_n(begin(), index, new_data.GetAddress());
                    std::uninitialized_copy_n(begin() + index, size_ - index, new_data.GetAddress() + index + 1);
                } catch (...) {
                std::destroy_n(new_data.GetAddress() + index, 1);
                throw;
                }
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else { 
            T temp_value(std::move(value));
            new (data_ + size_) T(std::move(data_[size_ - 1]));
            std::move_backward(data_ + index, data_ + size_ - 1, data_ + size_);
            data_[index] = std::move(temp_value);
        }
        ++size_;
        return data_ + index;
    }


private:
    RawMemory<T> data_;
    size_t size_ = 0;
};
