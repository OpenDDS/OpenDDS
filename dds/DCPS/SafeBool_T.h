/**
 * \file
 * Implements the "Safe Bool" idiom, which is a safer alternative to operator
 * bool. Based on:
 *   https://www.artima.com/articles/the-safe-bool-idiom
 *
 * If you want the boolean test function to be virtual, implement it as:
 *   virtual bool boolean_test() const;
 * and derive the class from SafeBool_T with no template argument:
 *   class YourClass : public SafeBool_T<>.
 *
 * If you do NOT want the boolean test function to be virtual, implement it as:
 *   bool boolean_test() const;
 * and derive the class from SafeBool_T with the class as the template argument:
 *   class YourClass : public SafeBool<YourClass>.
 */

#ifndef OPENDDS_DCPS_SAFE_BOOL_T_H
#define OPENDDS_DCPS_SAFE_BOOL_T_H

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class SafeBoolBase {
public:
  typedef void (SafeBoolBase::*BoolType)() const;
  void this_type_does_not_support_comparisons() const {}

protected:
  SafeBoolBase() {}
  SafeBoolBase(const SafeBoolBase&) {}
  SafeBoolBase& operator=(const SafeBoolBase&) {return *this;}
  ~SafeBoolBase() {}
};

template <typename DerivedNonVirtual = void>
class SafeBool_T : public SafeBoolBase {
public:
  operator BoolType() const
  {
    return (static_cast<const DerivedNonVirtual*>(this))->boolean_test()
      ? &SafeBoolBase::this_type_does_not_support_comparisons : 0;
  }

protected:
  ~SafeBool_T() {}
};

template<>
class SafeBool_T<void> : public SafeBoolBase {
public:
  operator BoolType() const
  {
    return boolean_test() ?
      &SafeBoolBase::this_type_does_not_support_comparisons : 0;
  }

protected:
  virtual bool boolean_test() const = 0;
  virtual ~SafeBool_T() {}
};

template <typename X, typename Y>
bool operator==(const SafeBool_T<X>& x, const SafeBool_T<Y>& y)
{
  x.this_type_does_not_support_comparisons();
  return false;
}

template <typename X, typename Y>
bool operator!=(const SafeBool_T<X>& x, const SafeBool_T<Y>& y)
{
  x.this_type_does_not_support_comparisons();
  return false;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
