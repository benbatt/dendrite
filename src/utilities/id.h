#pragma once

#include <functional>

typedef unsigned int IDValue;

template<class TModel>
class ID
{
public:
  explicit ID(IDValue value = 0)
    : mValue(value)
  {}

  IDValue value() const { return mValue; }

  bool operator==(const ID& other) const { return mValue == other.mValue; }

private:
  IDValue mValue;
};

template<class TModel>
struct std::hash<ID<TModel>>
{
  std::size_t operator()(const ID<TModel>& id) const { return std::hash<IDValue>{}(id.value()); }
};
