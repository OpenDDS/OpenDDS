#include "FACE/StringManager.h"
#include "dds/DCPS/SafetyProfilePool.h"
#include "dds/DCPS/Serializer.h"
#include <cstring>

namespace FACE {

namespace {
  static Char s_empty = 0;
#ifdef DDS_HAS_WCHAR
  static WChar s_wempty = 0;
#endif
}

Char* string_alloc(UnsignedLong len)
{
  if (len == 0) return &s_empty;
  void* const raw =
    ACE_Allocator::instance()->malloc(len + 1);
  Char* const str = static_cast<Char*>(raw);
  if (str) str[0] = static_cast<Char>(0);
  return str;
}

Char* string_dup(const Char* str)
{
  if (!str) return 0;
  if (!*str) return &s_empty;
  const size_t len = std::strlen(str);
  Char* const cpy = string_alloc(static_cast<UnsignedLong>(len));
  if (cpy) std::strncpy(cpy, str, len + 1);
  return cpy;
}

void string_free(Char* str)
{
  if (str != &s_empty) ACE_Allocator::instance()->free(str);
}

#ifdef DDS_HAS_WCHAR
WChar* wstring_alloc(UnsignedLong len)
{
  if (len == 0) return &s_wempty;
  const size_t n = (len + 1) * sizeof(WChar);
  void* const raw = ACE_Allocator::instance()->malloc(n);
  WChar* const str = static_cast<WChar*>(raw);
  if (str) str[0] = static_cast<WChar>(0);
  return str;
}

WChar* wstring_dup(const WChar* str)
{
  if (!str) return 0;
  if (!*str) return &s_wempty;
  const size_t len = std::wcslen(str);
  WChar* const cpy = wstring_alloc(static_cast<UnsignedLong>(len));
  if (cpy) std::wcsncpy(cpy, str, len + 1);
  return cpy;
}

void wstring_free(WChar* str)
{
  if (str != &s_wempty) ACE_Allocator::instance()->free(str);
}
#endif // DDS_HAS_WCHAR
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace FaceTypes {

bool operator>>(DCPS::Serializer& ser, StringBase<FACE::Char>& str)
{
  ser.read_string(str.out(), FACE::string_alloc, FACE::string_free);
  return ser.good_bit();
}

#ifdef DDS_HAS_WCHAR
bool operator>>(DCPS::Serializer& ser, StringBase<FACE::WChar>& str)
{
  ser.read_string(str.out(), FACE::wstring_alloc, FACE::wstring_free);
  return ser.good_bit();
}
#endif

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
