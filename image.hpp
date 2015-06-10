
#include <cstdint>

#ifndef __image_hpp
#define __image_hpp

using std::string;

template<typename T, typename E>
class IImage {
public:
  using IColumn = uint64_t;
  using IRow = uint64_t;
  using Metric = uint64_t;
  virtual Metric height() = 0;
  virtual Metric width() = 0;
  virtual const T operator()(IColumn, IRow) const = 0;
  virtual E operator>>(std::string) = 0;
  virtual E save(std::string s) {
    return this >> s;
  }
  virtual ~IImage() {}
};

template<typename T, typename E>
class IComputedImage : public virtual IImage<T, E> {
public:
  using typename IImage<T, E>::IColumn;
  using typename IImage<T, E>::IRow;
  virtual void compute(IColumn, IRow) = 0;
  virtual ~IComputedImage() {}
};

template<typename T, typename E>
class IASyncImage : public virtual IImage<T, E> {
public:
  using typename IImage<T, E>::IColumn;
  using typename IImage<T, E>::IRow;
  using AIPixelHandler = std::function<void(IColumn, IRow, T)>;
  virtual void fetchPixel(IColumn, IRow, AIPixelHandler) = 0;
  virtual ~IASyncImage() {}
};

// TODO: VIRTUAL DESTRUCTORS

class IPagedImage : public virtual ICachedImage<T, E> {
protected:
  virtual string getBackedFilename() = 0;
};

#endif

