#ifndef MYVECTOR_H_
#define MYVECTOR_H_

#include <cstring>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <type_traits>
#include <utility>

/*
 *  The index value type for large size.
 */
typedef unsigned long long LargeSizeType;

/*
 *  Thrown when a MyVector buffer cannot be reallocated
 */
class MyVectorBadAlloc : public std::bad_alloc {
private:
  std::string _message;

public:
  MyVectorBadAlloc(unsigned long long _requested) {
    _message = "MyVector: reallocation of " + std::to_string(_requested) +
               " elements failed";
  }

  const char *what() const noexcept override { return _message.c_str(); }
};

template <class T, class SizeType = unsigned int> class MyVector {
public:
  MyVector();

  ~MyVector() noexcept;

  MyVector(const MyVector &) = delete;

  MyVector &operator=(const MyVector &) = delete;

  MyVector(MyVector &&other) noexcept;

  MyVector &operator=(MyVector &&other) noexcept;

  /*
   *  Add the given element to the back.
   */
  void push_back(T const &_val);

  void push_back(T &&_val);

  /*
   *  Remove the last element from the array.
   */
  void pop_back();

  /*
   *  Allocate memory to the array with specified length.
   */
  void reserve(SizeType len);

  /*
   *  Shrink the array to fit the occupied size.
   */
  void shrink_to_fit();

  /*
   *  Shrink the length of the array by half.
   */
  void shrink_to_half();

  /*
   *  Clear elements in the array but do not modify its capacity.
   */
  void clear();

  /*
   *  Release the space of the array, namely the array will be deleted.
   */
  void release_space();

  /*
   *  Return the size of the array.
   */
  SizeType size() const;

  /*
   *  Return the capacity of the array.
   */
  SizeType capacity() const;

  /*
   *  Return the element list.
   */
  T *get_list() const;

  /*
   *  Return the element at the given index.
   */
  T &operator[](SizeType i) const {
    if (i < this->elementNum)
      return this->elementList[i];
    else {
      throw std::out_of_range(
          "MyVector: index " + std::to_string((unsigned long long)i) +
          " out of range (size = " +
          std::to_string((unsigned long long)elementNum) +
          ", capacity = " + std::to_string((unsigned long long)length) + ")");
    }
  }

protected:
  // public:
  /*
   *  The available length of this vector.
   */
  SizeType length;

  /*
   *  The current number of elements.
   */
  SizeType elementNum;

  /*
   *  The array of elements.
   */
  T *elementList;

  /*
   *  Enlarge the length of the array by a factor of 2.
   */
  void enlarge();

  /*
   *  Change capacity to new_length.
   */
  void reallocate_to(SizeType new_length);
};

template <class T, class SizeType> MyVector<T, SizeType>::MyVector() {
  // TODO Auto-generated constructor stub
  this->elementList = NULL;
  this->elementNum = 0;
  this->length = 0;
}

template <class T, class SizeType> MyVector<T, SizeType>::~MyVector() noexcept {
  release_space();
}

template <class T, class SizeType> T *MyVector<T, SizeType>::get_list() const {
  return this->elementList;
}

template <class T, class SizeType>
SizeType MyVector<T, SizeType>::size() const {
  return this->elementNum;
}

template <class T, class SizeType>
SizeType MyVector<T, SizeType>::capacity() const {
  return this->length;
}

template <class T, class SizeType>
void MyVector<T, SizeType>::push_back(T const &_val) {
  if (this->elementNum >= this->length) {
    this->enlarge();
  }
  new (&this->elementList[elementNum]) T(_val);
  elementNum++;
}

template <class T, class SizeType>
void MyVector<T, SizeType>::push_back(T &&_val) {
  if (this->elementNum >= this->length) {
    this->enlarge();
  }
  new (&this->elementList[elementNum]) T(std::move(_val));
  elementNum++;
}

template <class T, class SizeType> void MyVector<T, SizeType>::pop_back() {
  if (this->elementNum > 0) {
    this->elementNum--;
    this->elementList[elementNum].~T();
    // Added by jhgan at 4:32pm on Sept 5, 2016.
    // Shrink the length of the array when necessary.
    if (this->elementNum * 4 <= this->length)
      this->shrink_to_half();
  } else {
    throw std::out_of_range("MyVector::pop_back: called on an empty vector");
  }
}

template <class T, class SizeType>
void MyVector<T, SizeType>::reserve(SizeType len) {
  if (len > this->length) {
    this->reallocate_to(len);
  }
}

template <class T, class SizeType> void MyVector<T, SizeType>::shrink_to_fit() {
  if (this->elementNum == 0) {
    this->release_space();
    return;
  }

  if (this->elementNum < this->length) {
    this->reallocate_to(this->elementNum);
  }
}

template <class T, class SizeType>
void MyVector<T, SizeType>::shrink_to_half() {
  if (this->length <= 4) {
    return;
  }
  SizeType halfLength = this->length / 2;
  if (this->elementNum < halfLength) {
    this->reallocate_to(halfLength);
  }
}

template <class T, class SizeType>
MyVector<T, SizeType>::MyVector(MyVector &&other) noexcept
    : length(other.length), elementNum(other.elementNum),
      elementList(other.elementList) {
  other.elementList = NULL;
  other.elementNum = 0;
  other.length = 0;
}

template <class T, class SizeType>
MyVector<T, SizeType> &
MyVector<T, SizeType>::operator=(MyVector &&other) noexcept {
  if (this != &other) {
    release_space();
    elementList = other.elementList;
    elementNum = other.elementNum;
    length = other.length;
    other.elementList = NULL;
    other.elementNum = 0;
    other.length = 0;
  }
  return *this;
}

template <class T, class SizeType> void MyVector<T, SizeType>::clear() {
  while (this->elementNum > 0) {
    this->elementNum--;
    this->elementList[elementNum].~T();
  }
}

template <class T, class SizeType> void MyVector<T, SizeType>::release_space() {
  while (this->elementNum > 0) {
    this->elementNum--;
    this->elementList[elementNum].~T();
  }
  this->length = 0;
  if (this->elementList != NULL) {
    free(this->elementList);
    this->elementList = NULL;
  }
}

template <class T, class SizeType>
void MyVector<T, SizeType>::reallocate_to(SizeType new_length) {
  T *old_list = this->elementList;

  if constexpr (std::is_trivially_copyable<T>::value) {
    T *new_list = (T *)realloc(old_list, sizeof(T) * new_length);
    if (new_list == NULL) {
      throw MyVectorBadAlloc((unsigned long long)new_length);
    }

    this->elementList = new_list;
    this->length = new_length;
  } else {
    SizeType old_num = this->elementNum;

    T *new_list = (T *)malloc(sizeof(T) * new_length);
    if (new_list == NULL) {
      throw MyVectorBadAlloc((unsigned long long)new_length);
    }

    for (SizeType i = 0; i < old_num; ++i) {
      new (&new_list[i]) T(std::move(old_list[i]));
      old_list[i].~T();
    }
    if (old_list != NULL) {
      free(old_list);
    }

    this->elementList = new_list;
    this->length = new_length;
  }
}

template <class T, class SizeType> void MyVector<T, SizeType>::enlarge() {
  if (this->length == 0) {
    this->reallocate_to(2);
  } else {
    this->reserve(this->length * 2);
  }
}

#endif /* MYVECTOR_H_ */
