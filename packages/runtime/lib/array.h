//
// Created by Micha Reiser on 14.03.17.
//

#ifndef SPEEDYJS_RUNTIME_ARRAY_H
#define SPEEDYJS_RUNTIME_ARRAY_H

#include <stdexcept>
#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <new>
#include <algorithm>
#include "macros.h"

const size_t CAPACITY_GROW_FACTOR = 2;
const size_t DEFAULT_CAPACITY = 16;

/**
 * Implementation of the JS Array. Grows when more elements are added.
 *
 * Differences to vector:
 * This Class does use memset to default initialize the elements and not the allocator.
 * @tparam T type of the elements stored in the array
 * TODO Change size to int32_t to match ts
 */
template<typename T>
class Array {
private:
    /**
     * The length of the array
     */
    size_t length;

    /**
     * The capacity of the {@link elements}
     */
    size_t capacity;

    /**
     * The elements stored in the array. Has the size of {@link capacity}. All elements up to {@link length} are initialized
     * with zero. Is the nullptr if the length is zero (no allocation is needed in this case)
     */
    T* elements;
public:
    /**
     * Creates a new array of the given size
     * @param size the size (length) of the new array
     * @param elements the elements contained in the array with the length equal to size. If absent, the elements are default
     * initialized.
     */
    Array(size_t size=0, const T* elements = nullptr) {
        if (size == 0) {
            this->elements = nullptr;
        } else {
            this->elements = Array<T>::allocateElements(size);

            if (elements == nullptr) {
                // This is quite expensive, if there is a GC that guarantees zeroed memory, this is no longer needed
                std::fill_n(this->elements, size, T {});
            } else {
                std::copy(elements, elements + size, this->elements);
            }
        }

        this->capacity = size;
        this->length = size;
    }

    inline ~Array() {
        std::free(this->elements);
    }

    /**
     * Returns the element at the given index
     * @param index the index of the element to return
     * @return the element at the given index
     * @throws {@link std::out_of_range} if the index is <= length
     */
    inline T get(size_t index) const {
        if (length <= index) {
            throw std::out_of_range{"Index out of bound"};
        }

        return elements[index];
    }

    /**
     * Sets the value at the given index position. The array is resized to a length of the index + 1 if index >= length.
     * @param index the index of the element where the value is to be set
     * @param value the value to set at the given index
     */
    inline void set(size_t index, T value) {
        if (length <= index) {
            this->resize(index + 1);
        }

        elements[index] = value;
    }

    inline void fill(const T value, int32_t start=0) {
        this->fill(value, start, this->length);
    }

    /**
     * Sets the value of the array elements in between start and end to the given constant
     * @param value the value to set
     * @param start the start from which the values should be initialized
     * @param end the end where the value should no longer be set (exclusive)
     * @see https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/fill
     */
    inline void fill(const T value, int32_t start, int32_t end) {
        size_t startIndex = start < 0 ? this->length + start : static_cast<size_t>(start);
        size_t endIndex = end < 0 ? this->length + end : static_cast<size_t>(end);

        if (startIndex >= this->length) {
            throw std::out_of_range { "Start index is out of range" };
        }

        if (endIndex > this->length) {
            throw std::out_of_range { "End index is out of range" };
        }

        if (endIndex < startIndex) {
            return;
        }

        std::fill(this->elements + startIndex, this->elements + endIndex, value);
    }

    /**
     * Adds one or several new elements to the and of the array
     * @param elements the elements to add, not a nullptr
     * @param numElements the number of elements to add
     * @return the new length of the array
     */
    inline size_t push(const T* elements, size_t numElements) {
        const auto newLength = this->length + numElements;
        this->ensureCapacity(newLength);

        std::copy(elements, elements + numElements, &this->elements[this->length]);
        this->length = newLength;

        return newLength;
    }

    /**
     * Adds an element to the beginning of the array and returns the new length
     * @param elements the elements to add
     * @param numElements the number of elements to add
     * @return the new length after inserting the given element
     * @see https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/unshift
     */
    inline size_t unshift(const T* elements, size_t numElements) {
        const size_t newLength = this->length + numElements;
        this->ensureCapacity(newLength);

        std::copy(this->elements, this->elements + this->length, this->elements + numElements);
        std::copy(elements, elements + numElements, this->elements);

        this->length = newLength;
        return newLength;
    }

    /**
     * Removes the last element and returns it
     * @return the last element
     * @throws {@link std::out_of_range} if the array is empty
     * @see https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/pop
     */
    inline T pop() {
        if (this->length == 0) {
            throw std::out_of_range { "Array is empty" };
        }

        return this->elements[--this->length];
    }

    /**
     * Removes the first element and returns it
     * @return the first element
     * @throws {@link std::out_of_range} if the array is empty
     * @see https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/shift
     */
    inline T shift() {
        if (this->length == 0) {
            throw std::out_of_range { "Array is empty"};
        }

        const T element = this->elements[0];
        std::copy(this->elements + 1, this->elements + this->length, this->elements);
        --this->length;
        return element;
    }

    /**
     * Returns the size of the array
     * @return the size
     */
    inline size_t size() const {
        return length;
    }

    /**
     * Resizes the array to the new size.
     * @param newSize the new size
     */
    inline void resize(size_t newSize) {
        ensureCapacity(newSize);

        // No reduce
        if (this->length < newSize) {
            std::fill_n(&this->elements[this->length], newSize - this->length, T {}); // Default initialize values
        }

        length = newSize;
    }

private:
    /**
     * Ensures that the capacity of the array is at lest of the given size
     * @param min the minimal required capacity
     */
    void ensureCapacity(size_t min) {
        if (capacity >= min) {
            return;
        }

        if (min > INT32_MAX) {
            throw std::out_of_range { "Array size exceeded max limit"};
        }

        size_t newCapacity = capacity == 0 ? DEFAULT_CAPACITY : capacity * CAPACITY_GROW_FACTOR;

        if (newCapacity < min) {
            newCapacity = min;
        }

        if (newCapacity > INT32_MAX) {
            newCapacity = INT32_MAX;
        }

        this->elements = Array<T>::allocateElements(newCapacity, this->elements);
        this->capacity = newCapacity;
    }

    /**
     * (Re) Allocates an array for the elements with the given capacity
     * @param capacity the capacity to allocate
     * @param elements existing pointer to the elements array, in this case, a reallocate is performed
     * @returns the pointer to the allocated array
     */
    static inline T* allocateElements(size_t capacity, T* elements = nullptr) {
        if (capacity > INT32_MAX) {
            throw std::out_of_range { "Array size exceeded max limit"};
        }

        auto* allocation = std::realloc(elements, capacity * sizeof(T));

        if (allocation == nullptr) {
            throw std::bad_alloc {};
        }

        return static_cast<T*>(allocation);
    }
};

#endif //SPEEDYJS_RUNTIME_ARRAY_H