#include "common.h"

#include <tao/CORBA_String.h>

#include <iostream>
#include <typeinfo>

using OpenDDS::DCPS::Serializer;
using OpenDDS::DCPS::Encoding;

struct DataTypeBase {
  virtual ~DataTypeBase() {}

  virtual void write(Serializer& s) = 0;

  virtual void read(Serializer& s) = 0;

  virtual size_t space() = 0;

  size_t alignment(const Encoding& encoding)
  {
    return (std::min)(encoding.max_align(), _alignment());
  }

  virtual size_t _alignment() { return space(); }

  virtual const char* name() = 0;
};

template <typename T>
struct DataType : DataTypeBase {

  void write(Serializer& s) { s << T(); }

  void read(Serializer& s) { T t; s >> t; }

  size_t space() { return sizeof(T); }

  const char* name() { return typeid(T).name(); }
};


// special cases: can't use Serializer::operator<<(T) directly

template <>
struct DataType<ACE_CDR::Boolean> : DataTypeBase {

  void write(Serializer& s) { s << ACE_OutputCDR::from_boolean(true); }

  void read(Serializer& s) { bool b; s >> ACE_InputCDR::to_boolean(b); }

  size_t space() { return 1; }

  const char* name() { return "boolean"; }
};

template <>
struct DataType<ACE_CDR::Octet> : DataTypeBase {

  void write(Serializer& s) { s << ACE_OutputCDR::from_octet(3); }

  void read(Serializer& s) { ACE_CDR::Octet o; s >> ACE_InputCDR::to_octet(o); }

  size_t space() { return 1; }

  const char* name() { return "octet"; }
};

template <>
struct DataType<ACE_CDR::WChar> : DataTypeBase {

  void write(Serializer& s) { s << ACE_OutputCDR::from_wchar(L'a'); }

  void read(Serializer& s) { ACE_CDR::WChar w; s >> ACE_InputCDR::to_wchar(w); }

  size_t space() { return 2; }

  const char* name() { return "wchar"; }
};


// special cases: alignment != space

template <>
struct DataType<ACE_CDR::LongDouble> : DataTypeBase {

  void write(Serializer& s) { s << ACE_CDR::LongDouble(); }

  void read(Serializer& s) { ACE_CDR::LongDouble ld; s >> ld; }

  size_t space() { return 16; }

  const char* name() { return "long double"; }
};

template <>
struct DataType<const ACE_CDR::Char*> : DataTypeBase {

  void write(Serializer& s) { s << "hello"; }

  void read(Serializer& s) { CORBA::String_var str; s >> str.out(); }

  size_t space() { return 4 + sizeof("hello") /*include null*/; }

  size_t _alignment() { return 4; }

  const char* name() { return "string"; }
};

#ifdef DDS_HAS_WCHAR
template <>
struct DataType<const ACE_CDR::WChar*> : DataTypeBase {

  void write(Serializer& s) { s << L"hello"; }

  void read(Serializer& s) { CORBA::WString_var str; s >> str.out(); }

  size_t space() { return 4 + 5 * 2 /*don't include null*/; }

  size_t _alignment() { return 4; }

  const char* name() { return "wstring"; }
};
#endif

DataType<ACE_CDR::Boolean> dt_bool;
DataType<ACE_CDR::Char> dt_char;
DataType<ACE_CDR::WChar> dt_wchar;
DataType<ACE_CDR::Octet> dt_octet;
DataType<ACE_CDR::Short> dt_short;
DataType<ACE_CDR::UShort> dt_ushort;
DataType<ACE_CDR::Long> dt_long;
DataType<ACE_CDR::ULong> dt_ulong;
DataType<ACE_CDR::LongLong> dt_longlong;
DataType<ACE_CDR::ULongLong> dt_ulonglong;
DataType<ACE_CDR::Float> dt_float;
DataType<ACE_CDR::Double> dt_double;
DataType<ACE_CDR::LongDouble> dt_longdouble;
DataType<const ACE_CDR::Char*> dt_string;
#ifdef DDS_HAS_WCHAR
DataType<const ACE_CDR::WChar*> dt_wstring;
#endif

DataTypeBase* types[] = {
  &dt_bool,
  &dt_char,
  &dt_wchar,
  &dt_octet,
  &dt_short,
  &dt_ushort,
  &dt_long,
  &dt_ulong,
  &dt_longlong,
  &dt_ulonglong,
  &dt_float,
  &dt_double,
  &dt_longdouble,
  &dt_string,
#ifdef DDS_HAS_WCHAR
  &dt_wstring,
#endif
};
const size_t type_count = sizeof(types) / sizeof(types[0]);

bool testType(const Encoding& encoding, DataTypeBase* type, bool reset = false)
{
  ACE_Message_Block mb(1024);
  bool ok = true;
  // "offset" represents how far out of alignment the stream is before we write:
  // 8 and 0 should be the same, but test both anyway.
  // "memory" represents how far out of alignment the mb.wr_ptr() is before we
  // give the message block to the serializer (which establishes the logical
  // start of the stream).
  for (size_t memory = 0; memory <= 8; ++memory) {
    for (size_t offset = 0; offset <= 8; ++offset) {
      mb.reset();
      mb.wr_ptr(memory);

      Serializer s(&mb, encoding);
      mb.wr_ptr(offset);
      type->write(s);

      const size_t len1 = mb.length() - memory;
      const size_t align1 = type->alignment(encoding);
      const size_t padding1 = (align1 > 1 && offset % align1)
                             ? align1 - (offset % align1) : 0;
      const size_t expected1 = offset + padding1 + type->space();
      if (len1 != expected1) {
        ok = false;
        std::cerr << "ERROR: expected length " << expected1 << " != actual "
                  << len1 << " with offset " << offset << " and type "
                  << type->name() << " (padding " << padding1 << ")\n";
        continue;
      }

      if (reset) {
        s.reset_alignment();
      }

      type->write(s);
      {
        const size_t len = mb.length() - memory;
        const size_t align = type->alignment(encoding);
        const size_t extraPadding = (reset || !align) ? 0 : (len1 % align);

        const size_t padding = (align > 1 && offset % align)
                               ? align - (offset % align) : 0;
        const size_t expected = offset + extraPadding +
                                padding + type->space() * 2;
        if (len != expected) {
          ok = false;
          std::cerr << "ERROR: expected length2 " << expected << " != actual "
                    << len << " with offset " << offset << " and type "
                    << type->name() << " (padding " << padding << ")\n";
        }
      }

      mb.rd_ptr(memory);

      Serializer s2(&mb, encoding);
      mb.rd_ptr(offset);
      type->read(s2);
      if (reset) {
        s2.reset_alignment();
      }
      type->read(s2);

      const size_t leftover = mb.length(); // gets wr_ptr - rd_ptr
      if (leftover) {
        ok = false;
        std::cerr << "ERROR: " << leftover << " bytes remain after reading "
                  << type->name() << ", offset: " << offset << std::endl;
      }
    }
  }
  return ok;
}

bool runAlignmentTest()
{
  std::cerr << "\nRunning alignment tests...\n";
  bool ok = true;
  for (size_t type = 0; type < type_count; ++type) {
    for (size_t encoding = 0; encoding < encoding_count; ++encoding) {
      ok &= testType(encodings[encoding], types[type]);
    }
  }
  return ok;
}

bool runAlignmentResetTest()
{
  std::cerr << "\nRunning alignment reset tests...\n";
  bool ok = true;
  for (size_t type = 0; type < type_count; ++type) {
    for (size_t encoding = 0; encoding < encoding_count; ++encoding) {
      ok &= testType(encodings[encoding], types[type], true);
    }
  }
  return ok;
}

bool runAlignmentOverrunTest()
{
  std::cerr << "\nRunning alignment overrun test...\n";

  for (size_t encoding = 0; encoding < encoding_count; ++encoding) {
    ACE_Message_Block mb(4);

    Serializer s1(&mb, encodings[encoding]);
    ACE_CDR::Long x = 42;
    if (!(s1 << x)) {
      std::cerr << "runAlignmentOverrunTest: 1st serialization using " <<
        encodings[encoding].to_string() << " failed\n";
      return false;
    }
    if (s1 << x) {
      std::cerr << "runAlignmentOverrunTest: 2st serialization using " <<
        encodings[encoding].to_string() << " succeeded when it should've failed\n";
      return false;
    }

    Serializer s2(&mb, encodings[encoding]);
    if (!(s2 >> x)) {
      std::cerr << "runAlignmentOverrunTest: 1st deserialization using " <<
        encodings[encoding].to_string() << " failed\n";
      return false;
    }
    if (s2 >> x) {
      std::cerr << "runAlignmentOverrunTest: 2nd deserialization using " <<
        encodings[encoding].to_string() << " succeeded when it should've failed\n";
      return false;
    }
  }

  return true;
}
