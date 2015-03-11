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
class String_Manager
{
  typedef String_Traits<CharT> Traits;

public:
  String_Manager()
    : str_(Traits::empty())
  {}

  String_Manager(CharT* moved)
    : str_(moved)
  {}

  String_Manager(const CharT* copied)
    : str_(Traits::dup(copied))
  {}

  String_Manager(const String_Manager& copied)
    : str_(Traits::dup(copied.str_))
  {}

  String_Manager& operator=(CharT* moved)
  {
    Traits::free(str_);
    str_ = moved;
    return *this;
  }

  String_Manager& operator=(const CharT* copied)
  {
    String_Manager cpy(copied);
    std::swap(str_, cpy.str_);
    return *this;
  }

  String_Manager& operator=(const String_Manager& copied)
  {
    String_Manager cpy(copied);
    std::swap(str_, cpy.str_);
    return *this;
  }

  ~String_Manager()
  {
    Traits::free(str_);
  }

  operator const CharT*() const { return str_; }
  const CharT* in() const { return str_; }
  CharT*& inout() { return str_; }

  CharT*& out()
  {
    Traits::free(str_);
    str_ = 0;
    return str_;
  }

  CharT* _retn()
  {
    CharT* tmp = str_;
    str_ = Traits::empty();
    return tmp;
  }

private:
  CharT* str_;
};

template <typename CharT>
inline bool operator<(const String_Manager<CharT>& lhs,
                      const String_Manager<CharT>& rhs)
{
  return String_Traits<CharT>::cmp(lhs, rhs) < 0;
}

}
}

#endif
