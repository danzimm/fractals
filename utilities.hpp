
#include <functional>

#ifndef __utilities_hpp
#define __utilities_hpp

template<typename T>
struct Functor {
  T value;
  template<typename S>
  virtual Functor<S> map(std::function<S(T)>) = 0;
};

template<typename T>
struct Optional : public virtual Functor {
  bool defined;
  T value;
  Optional() : defined(false) {}
  Optional(T v) : value(v) {}
  Optional<S> map(std::function<S(T)> f) {
    return defined ? Optional(f(value)) : Optional();
  }
  T getOrElse(T el) {
    return defined ? value : el;
  }
};

template<typename L, typename R>
struct Either {
  bool isLeft;
  L left;
  R right;
  Either(L l) : left(l), isLeft(true) {}
  Either(R r) : right(r), isLeft(false) {}
  L getLeftOrElse(L el) {
    return isLeft ? left : el;
  }
  R getRightOrElse(R el) {
    return isLeft ? el : right;
  }
};

template<typename T>
struct Indexed {
  using Index = uint64_t;
  Index index;
  T value;
  const Indexed<T> operator()(Index i) const {
    return Indexed<T>(*this)(i);
  }
  Indexed<T> operator()(Index i) {
    index = i;
    return *this;
  }
};

#endif

