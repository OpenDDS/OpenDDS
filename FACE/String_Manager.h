#ifndef OPENDDS_FACE_STRING_MANAGER_HEADER
#define OPENDDS_FACE_STRING_MANAGER_HEADER

#include "FACE/types.hpp"

#include <cstring>
#include <algorithm>

namespace OpenDDS {
namespace FaceTypes {

template <typename CharT>
struct String_Traits {};

template <>
struct String_Traits<FACE::Char>
{
  static FACE::Char* empty() { return dup(""); }
  static FACE::Char* dup(const FACE::Char* s) { return FACE::string_dup(s); }
  static void free(FACE::Char* s) { FACE::string_free(s); }

  static int cmp(const FACE::Char* lhs, const FACE::Char* rhs)
  {
    return std::strcmp(lhs, rhs);
  }
};

template <>
struct String_Traits<FACE::WChar>
{
  static FACE::WChar* empty() { return dup(L""); }
  static FACE::WChar* dup(const FACE::WChar* s) { return FACE::wstring_dup(s); }
  static void free(FACE::WChar* s) { FACE::wstring_free(s); }

  static int cmp(const FACE::WChar* lhs, const FACE::WChar* rhs)
  {
    return std::wcscmp(lhs, rhs);
  }
};

template <typename CharT>
class String_Base
{
protected:
  typedef String_Traits<CharT> Traits;

  explicit String_Base(CharT* moved)
    : str_(moved)
  {}

  explicit String_Base(const CharT* copied)
    : str_(Traits::dup(copied))
  {}

  ~String_Base()
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
class String_Manager : public String_Base<CharT>
{
  typedef String_Base<CharT> Base;
  using typename Base::Traits;
  using Base::str_;

public:
  String_Manager()
    : Base(Traits::empty())
  {}

  String_Manager(const String_Manager& copied)
    : Base(copied)
  {}

  String_Manager(CharT* moved)
    : Base(moved)
  {}

  String_Manager(const CharT* copied)
    : Base(copied)
  {}

  String_Manager& operator=(const String_Manager& copied)
  {
    String_Manager tmp(copied);
    std::swap(str_, tmp.str_);
    return *this;
  }

  String_Manager& operator=(CharT* moved)
  {
    String_Manager tmp(moved);
    std::swap(str_, tmp.str_);
    return *this;
  }

  String_Manager& operator=(const CharT* copied)
  {
    String_Manager tmp(copied);
    std::swap(str_, tmp.str_);
    return *this;
  }

  CharT* _retn()
  {
    CharT* const tmp = str_;
    str_ = Traits::empty();
    return tmp;
  }
};

template <typename CharT>
class String_var : public String_Base<CharT>
{
  typedef String_Base<CharT> Base;
  using typename Base::Traits;
  using Base::str_;

public:
  String_var()
    : Base(0)
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
    std::swap(str_, moved.str_);
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
};

template <typename CharT>
class String_out
{
  typedef String_Traits<CharT> Traits;

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
inline bool operator<(const String_Base<CharT>& lhs,
                      const String_Base<CharT>& rhs)
{
  if (!lhs.in()) {
    return rhs.in();
  }
  return String_Traits<CharT>::cmp(lhs, rhs) < 0;
}

}
}

#endif
