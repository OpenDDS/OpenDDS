#ifndef OPENDDS_FACE_STRINGMANAGER_HEADER
#define OPENDDS_FACE_STRINGMANAGER_HEADER

#include "FACE/types.hpp"
#include "dds/DCPS/Definitions.h"

#include <cstring>
#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {
  class Serializer;
}

namespace FaceTypes {

template <typename CharT>
struct StringTraits {};

template <>
struct StringTraits<FACE::Char>
{
  static FACE::Char* alloc(FACE::UnsignedLong len) { return FACE::string_alloc(len); }
  static FACE::Char* empty() { return dup(""); }
  static FACE::Char* dup(const FACE::Char* s) { return FACE::string_dup(s); }
  static void free(FACE::Char* s) { if (s) FACE::string_free(s); }

  static int cmp(const FACE::Char* lhs, const FACE::Char* rhs)
  {
    return std::strcmp(lhs, rhs);
  }
};

#ifdef DDS_HAS_WCHAR
template <>
struct StringTraits<FACE::WChar>
{
  static FACE::WChar* alloc(FACE::UnsignedLong len) { return FACE::wstring_alloc(len); }
  static FACE::WChar* empty() { return dup(L""); }
  static FACE::WChar* dup(const FACE::WChar* s) { return FACE::wstring_dup(s); }
  static void free(FACE::WChar* s) { if (s) FACE::wstring_free(s); }

  static int cmp(const FACE::WChar* lhs, const FACE::WChar* rhs)
  {
    return std::wcscmp(lhs, rhs);
  }
};
#endif

template <typename CharT>
class StringBase
{
protected:
  typedef StringTraits<CharT> Traits;

  StringBase()
    : str_(Traits::empty())
  { }

  explicit StringBase(CharT* moved)
    : str_(moved)
  {}

  explicit StringBase(const CharT* copied)
    : str_(Traits::dup(copied))
  {}

  StringBase(const StringBase& copied)
    : str_(Traits::dup(copied))
  {}

  StringBase& operator=(const StringBase& copied)
  {
    StringBase tmp(copied);
    std::swap(str_, tmp.str_);
    return *this;
  }

  ~StringBase()
  {
    Traits::free(str_);
  }

public:
  operator const CharT*() const { return str_; }
  const CharT* in() const { return str_; }
  CharT*& inout() { return str_; }

  CharT*& out()
  {
    Traits::free(str_);
    str_ = 0;
    return str_;
  }

protected:
  CharT* str_;
};

template <typename CharT>
class StringManager : public StringBase<CharT>
{
  typedef StringBase<CharT> Base;
  typedef StringTraits<CharT> Traits;
  using Base::str_;

public:
  StringManager()
    : Base(Traits::empty())
  {}

  StringManager(const StringManager& copied)
    : Base(copied)
  {}

  StringManager(CharT* moved)
    : Base(moved)
  {}

  StringManager(const CharT* copied)
    : Base(copied)
  {}

  StringManager& operator=(const StringManager& copied)
  {
    StringManager tmp(copied);
    std::swap(str_, tmp.str_);
    return *this;
  }

  StringManager& operator=(CharT* moved)
  {
    StringManager tmp(moved);
    std::swap(str_, tmp.str_);
    return *this;
  }

  StringManager& operator=(const CharT* copied)
  {
    StringManager tmp(copied);
    std::swap(str_, tmp.str_);
    return *this;
  }

  CharT* _retn()
  {
    CharT* const tmp = str_;
    str_ = Traits::empty();
    return tmp;
  }

  using Base::operator const CharT*;
};

template <typename CharT>
class String_var : public StringBase<CharT>
{
  typedef StringBase<CharT> Base;
  typedef StringTraits<CharT> Traits;
  using Base::str_;

public:
  String_var()
    : Base()
  {}

  String_var(const String_var& copied)
    : Base(copied)
  {}

  String_var(CharT* moved)
    : Base(moved)
  {}

  String_var(const CharT* copied)
    : Base(copied)
  {}

  String_var& operator=(const String_var& copied)
  {
    String_var tmp(copied);
    std::swap(str_, tmp.str_);
    return *this;
  }

  String_var& operator=(CharT* moved)
  {
    String_var tmp(moved);
    std::swap(str_, tmp.str_);
    return *this;
  }

  String_var& operator=(const CharT* copied)
  {
    String_var tmp(copied);
    std::swap(str_, tmp.str_);
    return *this;
  }

  operator CharT*&() { return str_; }

  CharT* _retn()
  {
    CharT* const retn = str_;
    str_ = 0;
    return retn;
  }

  CharT& operator[](FACE::UnsignedLong index) { return str_[index]; }
  CharT operator[](FACE::UnsignedLong index) const { return str_[index]; }

  using Base::operator const CharT*;
};

template <typename CharT>
class String_out
{
  typedef StringTraits<CharT> Traits;

public:
  String_out(CharT*& p)
    : str_(p)
  {
    str_ = 0;
  }

  String_out(const String_out& s)
    : str_(s.str_)
  {}

  String_out(String_var<CharT>& p)
    : str_(p.out())
  {}

  String_out& operator=(const String_out& s)
  {
    str_ = s.str_;
    return *this;
  }

  String_out& operator=(CharT* p)
  {
    str_ = p;
    return *this;
  }

  String_out& operator=(const CharT* p)
  {
    str_ = Traits::dup(p);
    return *this;
  }

  operator CharT*&() { return str_; }
  operator const CharT*() const { return str_; }

  CharT*& ptr() { return str_; }
  const CharT* ptr() const { return str_;}

private:
  // assignment from String_var disallowed
  void operator=(const String_var<CharT>&);

  CharT*& str_;
};

template <typename CharT>
inline bool operator<(const StringBase<CharT>& lhs,
                      const StringBase<CharT>& rhs)
{
  if (!lhs.in()) {
    return rhs.in();
  }
  return StringTraits<CharT>::cmp(lhs, rhs) < 0;
}

template <typename CharT>
inline bool operator==(const StringBase<CharT>& lhs,
                       const StringBase<CharT>& rhs)
{
  if (!lhs.in()) {
    return !rhs.in();
  }
  return StringTraits<CharT>::cmp(lhs, rhs) == 0;
}

template <typename CharT>
inline bool operator!=(const StringBase<CharT>& lhs,
                       const StringBase<CharT>& rhs)
{
  return !(lhs == rhs);
}

template <typename CharT>
inline bool operator==(const StringBase<CharT>& lhs,
                       const CharT* rhs)
{
  if (!lhs.in()) {
    return !rhs;
  }
  return StringTraits<CharT>::cmp(lhs, rhs) == 0;
}

template <typename CharT>
inline bool operator!=(const StringBase<CharT>& lhs,
                       const CharT* rhs)
{
  return !(lhs == rhs);
}

template <typename CharT>
inline bool operator==(const CharT* lhs,
                       const StringBase<CharT>& rhs)
{
  if (!lhs) {
    return !rhs.in();
  }
  return StringTraits<CharT>::cmp(lhs, rhs) == 0;
}

template <typename CharT>
inline bool operator!=(const CharT* lhs,
                       const StringBase<CharT>& rhs)
{
  return !(lhs == rhs);
}

OpenDDS_FACE_Export
bool operator>>(DCPS::Serializer& ser, StringBase<FACE::Char>& str);

#ifdef DDS_HAS_WCHAR
OpenDDS_FACE_Export
bool operator>>(DCPS::Serializer& ser, StringBase<FACE::WChar>& str);
#endif

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
