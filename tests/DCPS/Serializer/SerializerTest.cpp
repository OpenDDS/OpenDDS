#include "common.h"

#include <dds/DCPS/Definitions.h>

#include <ace/ACE.h>
#include <ace/Message_Block.h>
#include <ace/OS_NS_string.h>

#include <iostream>
#include <string>

using OpenDDS::DCPS::Serializer;
using OpenDDS::DCPS::Encoding;

const size_t ARRAYSIZE = 15;

bool failed = false;

struct Values {
  ACE_CDR::Octet octetValue;
#if OPENDDS_HAS_EXPLICIT_INTS
  ACE_CDR::Int8 int8Value;
#endif
  ACE_CDR::Short shortValue;
  ACE_CDR::Long longValue;
  ACE_CDR::LongLong longlongValue;
#if OPENDDS_HAS_EXPLICIT_INTS
  ACE_CDR::UInt8 uint8Value;
#endif
  ACE_CDR::UShort ushortValue;
  ACE_CDR::ULong ulongValue;
  ACE_CDR::ULongLong ulonglongValue;
  ACE_CDR::Float floatValue;
  ACE_CDR::Double doubleValue;
  ACE_CDR::LongDouble longdoubleValue;
  ACE_CDR::Char charValue;
  ACE_CDR::WChar wcharValue;
  ACE_CDR::Char* stringValue;
#ifndef OPENDDS_SAFETY_PROFILE
  std::string stdstringValue;
#endif
#ifdef DDS_HAS_WCHAR
  ACE_CDR::WChar* wstringValue;
#ifndef OPENDDS_SAFETY_PROFILE
  std::wstring stdwstringValue;
#endif
#endif
};

struct ArrayValues {
  ACE_CDR::Octet octetValue[ARRAYSIZE];
#if OPENDDS_HAS_EXPLICIT_INTS
  ACE_CDR::Int8 int8Value[ARRAYSIZE];
#endif
  ACE_CDR::Short shortValue[ARRAYSIZE];
  ACE_CDR::Long longValue[ARRAYSIZE];
  ACE_CDR::LongLong longlongValue[ARRAYSIZE];
#if OPENDDS_HAS_EXPLICIT_INTS
  ACE_CDR::UInt8 uint8Value[ARRAYSIZE];
#endif
  ACE_CDR::UShort ushortValue[ARRAYSIZE];
  ACE_CDR::ULong ulongValue[ARRAYSIZE];
  ACE_CDR::ULongLong ulonglongValue[ARRAYSIZE];
  ACE_CDR::Float floatValue[ARRAYSIZE];
  ACE_CDR::Double doubleValue[ARRAYSIZE];
  ACE_CDR::LongDouble longdoubleValue[ARRAYSIZE];
  ACE_CDR::Char charValue[ARRAYSIZE];
  ACE_CDR::WChar wcharValue[ARRAYSIZE];
};

void insertions(ACE_Message_Block* chain, const Values& values,
  const Encoding& encoding)
{
  Serializer serializer(chain, encoding);

  serializer << ACE_OutputCDR::from_octet(values.octetValue);
#if OPENDDS_HAS_EXPLICIT_INTS
  serializer << ACE_OutputCDR::from_int8(values.int8Value);
#endif
  serializer << values.shortValue;
  serializer << values.longValue;
  serializer << values.longlongValue;
#if OPENDDS_HAS_EXPLICIT_INTS
  serializer << ACE_OutputCDR::from_uint8(values.uint8Value);
#endif
  serializer << values.ushortValue;
  serializer << values.ulongValue;
  serializer << values.ulonglongValue;
  serializer << values.floatValue;
  serializer << values.doubleValue;
  serializer << values.longdoubleValue;
  serializer << values.charValue;
  serializer << ACE_OutputCDR::from_wchar(values.wcharValue);
  serializer << ACE_OutputCDR::from_string(values.stringValue, 0);
#ifndef OPENDDS_SAFETY_PROFILE
  serializer << values.stdstringValue;
#endif
#ifdef DDS_HAS_WCHAR
  serializer << ACE_OutputCDR::from_wstring(values.wstringValue, 0);
#ifndef OPENDDS_SAFETY_PROFILE
  serializer << values.stdwstringValue;
#endif
#endif
}

void array_insertions(
  ACE_Message_Block* chain, const ArrayValues& values,
  ACE_CDR::ULong length, const Encoding& encoding)
{
  Serializer serializer(chain, encoding);

  serializer.write_octet_array(values.octetValue, length);
#if OPENDDS_HAS_EXPLICIT_INTS
  serializer.write_int8_array(values.int8Value, length);
#endif
  serializer.write_short_array(values.shortValue, length);
  serializer.write_long_array(values.longValue, length);
  serializer.write_longlong_array(values.longlongValue, length);
#if OPENDDS_HAS_EXPLICIT_INTS
  serializer.write_uint8_array(values.uint8Value, length);
#endif
  serializer.write_ushort_array(values.ushortValue, length);
  serializer.write_ulong_array(values.ulongValue, length);
  serializer.write_ulonglong_array(values.ulonglongValue, length);
  serializer.write_float_array(values.floatValue, length);
  serializer.write_double_array(values.doubleValue, length);
  serializer.write_longdouble_array(values.longdoubleValue, length);
  serializer.write_char_array(values.charValue, length);
  serializer.write_wchar_array(values.wcharValue, length);
}

size_t skip(size_t pos, size_t typeSize, size_t maxAlign)
{
  if (maxAlign == 0) {
    return 0;
  }
  #undef min
  size_t align = std::min(typeSize, maxAlign);
  return (align - (pos % align)) % align;
}

void print(const size_t& pos, const size_t& expectedPos,
  size_t& prevPos, bool& readPosOk, std::string type)
{
  if (pos != expectedPos && readPosOk) {
    std::cerr << "ERROR: Read " << type << " -- Prev position: " << prevPos << ". Read: "
              << pos << ". Expected: " << expectedPos << std::endl;
    readPosOk = false;
  }
  prevPos = pos;
}

bool extractions(Serializer& serializer, Values& values,
  const Encoding& encoding, const bool checkPos)
{
  bool readPosOk = true;
  size_t pos = serializer.rpos(), expectedPos = 0, prevPos = pos;
  if (pos != expectedPos) {
    std::cerr << "ERROR: Initial position is" << pos
              << ". It should be 0." << std::endl;
    readPosOk = false;
  }

  serializer >> ACE_InputCDR::to_octet(values.octetValue);
  if (checkPos) {
    expectedPos += 1;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "octet");
  }

#if OPENDDS_HAS_EXPLICIT_INTS
  serializer >> ACE_InputCDR::to_int8(values.int8Value);
  if (checkPos) {
    expectedPos += OpenDDS::DCPS::int8_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "int8");
  }
#endif

  serializer >> values.shortValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::int16_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::int16_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "short");
  }

  serializer >> values.longValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::int32_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::int32_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "long");
  }

  serializer >> values.longlongValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::int64_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::int64_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "long long");
  }

#if OPENDDS_HAS_EXPLICIT_INTS
  serializer >> ACE_InputCDR::to_uint8(values.uint8Value);
  if (checkPos) {
    expectedPos += OpenDDS::DCPS::uint8_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "uint8");
  }
#endif

  serializer >> values.ushortValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::uint16_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::uint16_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "ushort");
  }

  serializer >> values.ulongValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::uint32_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::uint32_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "ulong");
  }

  serializer >> values.ulonglongValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::uint64_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::uint64_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "ulong long");
  }

  serializer >> values.floatValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::float32_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::float32_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "float");
  }

  serializer >> values.doubleValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::float64_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::float64_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "double");
  }

  serializer >> values.longdoubleValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::float128_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::float128_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "long double");
  }

  serializer >> values.charValue;
  if (checkPos) {
    expectedPos += OpenDDS::DCPS::char8_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "char");
  }

  serializer >> ACE_InputCDR::to_wchar(values.wcharValue);
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::char16_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::char16_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "wchar");
  }

  serializer >> ACE_InputCDR::to_string(values.stringValue, 0);
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::uint32_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::uint32_cdr_size + ACE_OS::strlen(values.stringValue) + 1;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "string");
  }
#ifndef OPENDDS_SAFETY_PROFILE
  serializer >> values.stdstringValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::uint32_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::uint32_cdr_size + values.stdstringValue.size() + 1;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "std string");
  }
#endif
#ifdef DDS_HAS_WCHAR
  serializer >> ACE_InputCDR::to_wstring(values.wstringValue, 0);
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::uint32_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::uint32_cdr_size +
      ACE_OS::strlen(values.wstringValue) * OpenDDS::DCPS::char16_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "wstring");
  }
#ifndef OPENDDS_SAFETY_PROFILE
  serializer >> values.stdwstringValue;
  if (checkPos) {
    expectedPos += skip(pos, OpenDDS::DCPS::uint32_cdr_size, encoding.max_align()) +
      OpenDDS::DCPS::uint32_cdr_size +
      values.stdwstringValue.size() * OpenDDS::DCPS::char16_cdr_size;
    pos = serializer.rpos();
    print(pos, expectedPos, prevPos, readPosOk, "std wstring");
  }
#endif
#endif

  return readPosOk;
}

void array_extractions(ACE_Message_Block* chain, ArrayValues& values,
  ACE_CDR::ULong length, const Encoding& encoding)
{
  Serializer serializer(chain, encoding);

  serializer.read_octet_array(values.octetValue, length);
#if OPENDDS_HAS_EXPLICIT_INTS
  serializer.read_int8_array(values.int8Value, length);
#endif
  serializer.read_short_array(values.shortValue, length);
  serializer.read_long_array(values.longValue, length);
  serializer.read_longlong_array(values.longlongValue, length);
#if OPENDDS_HAS_EXPLICIT_INTS
  serializer.read_uint8_array(values.uint8Value, length);
#endif
  serializer.read_ushort_array(values.ushortValue, length);
  serializer.read_ulong_array(values.ulongValue, length);
  serializer.read_ulonglong_array(values.ulonglongValue, length);
  serializer.read_float_array(values.floatValue, length);
  serializer.read_double_array(values.doubleValue, length);
  serializer.read_longdouble_array(values.longdoubleValue, length);
  serializer.read_char_array(values.charValue, length);
  serializer.read_wchar_array(values.wcharValue, length);
}

ACE_Message_Block*
getchain(size_t blocks, const int* defs)
{
  //std::cout << "Creating a chain with " << blocks << " blocks." << std::endl;

  ACE_Message_Block* head = 0;
  ACE_Message_Block* current = 0;

  for (size_t i = 0; i < blocks; ++i) {
    //std::cout << "Creating new block with " << defs[i] << " bytes." << std::endl;
    ACE_Message_Block* b = new ACE_Message_Block(defs[i]);
    if (head) {
      current->cont(b);
    } else {
      head = b;
    }
    current = b;
  }
  return head;
}

void
displayChain(ACE_Message_Block* chain)
{
  //std::cout << "DISPLAYING CHAIN" << std::endl;
  for (ACE_Message_Block* current = chain; current; current = current->cont()) {
    if (current->length() > 0) {
      //std::cout << "DISPLAYING BLOCK" << std::endl;
      ACE_TCHAR buffer[4096];
      ACE::format_hexdump(current->base(), current->length(), buffer, sizeof(buffer));
      std::cout << buffer << std::endl;
    }
  }
}

void
checkValues(const Values& expected, const Values& observed)
{
  ACE_TCHAR ebuffer[512];
  ACE_TCHAR obuffer[512];
  if (expected.charValue != observed.charValue) {
    std::cout << "char values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.charValue << ", observed: " << observed.charValue << ")." << std::endl;
    failed = true;
  }
  if (expected.doubleValue != observed.doubleValue) {
    std::cout << "double values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.doubleValue << ", observed: " << observed.doubleValue << ")." << std::endl;
    failed = true;
  }
  if (expected.floatValue != observed.floatValue) {
    std::cout << "float values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.floatValue << ", observed: " << observed.floatValue << ")." << std::endl;
    failed = true;
  }
  if (expected.longdoubleValue != observed.longdoubleValue) {
    std::cout << "longdouble values not correct after insertion and extraction." << std::endl;
    // std::cout << "(expected: " << expected.longdoubleValue << ", observed: " << observed.longdoubleValue << ")." << std::endl;
    failed = true;
  }
  if (expected.longlongValue != observed.longlongValue) {
    std::cout << "longlong values not correct after insertion and extraction." << std::endl;
    //std::cout << "(expected: " << expected.longlongValue << ", observed: " << observed.longlongValue << ")." << std::endl;
    failed = true;
  }
  if (expected.longValue != observed.longValue) {
    std::cout << "long values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.longValue << ", observed: " << observed.longValue << ")." << std::endl;
    failed = true;
  }
  if (expected.octetValue != observed.octetValue) {
    std::cout << "octet values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.octetValue << ", observed: " << observed.octetValue << ")." << std::endl;
    failed = true;
  }
#if OPENDDS_HAS_EXPLICIT_INTS
  if (expected.int8Value != observed.int8Value) {
    std::cout << "int8 values not correct after insertion and extraction." << std::endl
      << "(expected: " << expected.int8Value << ", observed: " << observed.int8Value
      << ")." << std::endl;
    failed = true;
  }
#endif
  if (expected.shortValue != observed.shortValue) {
    std::cout << "short values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.shortValue << ", observed: " << observed.shortValue << ")." << std::endl;
    failed = true;
  }
  if (expected.ulonglongValue != observed.ulonglongValue) {
    std::cout << "ulonglong values not correct after insertion and extraction." << std::endl;
    //std::cout << "(expected: " << expected.ulonglongValue << ", observed: " << observed.ulonglongValue << ")." << std::endl;
    failed = true;
  }
  if (expected.ulongValue != observed.ulongValue) {
    std::cout << "ulong values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.ulongValue << ", observed: " << observed.ulongValue << ")." << std::endl;
    failed = true;
  }
#if OPENDDS_HAS_EXPLICIT_INTS
  if (expected.uint8Value != observed.uint8Value) {
    std::cout << "uint8 values not correct after insertion and extraction." << std::endl
      << "(expected: " << expected.uint8Value << ", observed: " << observed.uint8Value
      << ")." << std::endl;
    failed = true;
  }
#endif
  if (expected.ushortValue != observed.ushortValue) {
    std::cout << "ushort values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.ushortValue << ", observed: " << observed.ushortValue << ")." << std::endl;
    failed = true;
  }
  if (expected.wcharValue != observed.wcharValue) {
    ACE::format_hexdump((char*)&(expected.wcharValue), sizeof(ACE_CDR::WChar), ebuffer, sizeof(ebuffer));
    ACE::format_hexdump((char*)&(observed.wcharValue), sizeof(ACE_CDR::WChar), obuffer, sizeof(obuffer));
    std::cout << "wchar values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.wcharValue << "/" << ebuffer;
    std::cout << ", observed: " << observed.wcharValue << "/" << obuffer;
    std::cout << ")." << std::endl;
    failed = true;
  }
  if((expected.stringValue != 0) && (0 != ACE_OS::strcmp(expected.stringValue, observed.stringValue))) {
    ACE::format_hexdump(expected.stringValue, ACE_OS::strlen(expected.stringValue), ebuffer, sizeof(ebuffer));
    ACE::format_hexdump(observed.stringValue, ACE_OS::strlen(observed.stringValue), obuffer, sizeof(obuffer));
    std::cout << "string values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.stringValue << "/" << ebuffer;
    std::cout << ", observed: " << observed.stringValue << "/" << obuffer;
    std::cout << ")." << std::endl;
    failed = true;
  }
#ifndef OPENDDS_SAFETY_PROFILE
  if(expected.stdstringValue != observed.stdstringValue) {
    ACE::format_hexdump(expected.stdstringValue.c_str(), expected.stdstringValue.length(), ebuffer, sizeof(ebuffer));
    ACE::format_hexdump(observed.stdstringValue.c_str(), observed.stdstringValue.length(), obuffer, sizeof(obuffer));
    std::cout << "std string values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.stdstringValue << "/" << ebuffer;
    std::cout << ", observed: " << observed.stdstringValue << "/" << obuffer;
    std::cout << ")." << std::endl;
    failed = true;
  }
#endif
#ifdef DDS_HAS_WCHAR
  if((expected.wstringValue != 0) && (0 != ACE_OS::strcmp(expected.wstringValue, observed.wstringValue))) {
    ACE::format_hexdump(reinterpret_cast<char*>(expected.wstringValue), ACE_OS::strlen(expected.wstringValue), ebuffer, sizeof(ebuffer));
    ACE::format_hexdump(reinterpret_cast<char*>(observed.wstringValue), ACE_OS::strlen(observed.wstringValue), obuffer, sizeof(obuffer));
    std::cout << "wstring values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.wstringValue << "/" << ebuffer;
    std::cout << ", observed: " << observed.wstringValue << "/" << obuffer;
    std::cout << ")." << std::endl;
    failed = true;
  }
#ifndef OPENDDS_SAFETY_PROFILE
  if(expected.stdwstringValue != observed.stdwstringValue) {
    ACE::format_hexdump(reinterpret_cast<const char*>(expected.stdwstringValue.c_str()), expected.stdwstringValue.length(), ebuffer, sizeof(ebuffer));
    ACE::format_hexdump(reinterpret_cast<const char*>(observed.stdwstringValue.c_str()), observed.stdwstringValue.length(), obuffer, sizeof(obuffer));
    std::cout << "std wstring values not correct after insertion and extraction." << std::endl;
    std::wcout << "(expected: " << expected.stdwstringValue << "/" << ebuffer;
    std::wcout << ", observed: " << observed.stdwstringValue << "/" << obuffer;
    std::cout << ")." << std::endl;
    failed = true;
  }
#endif
#endif
}

void
checkArrayValues(const ArrayValues& expected, const ArrayValues& observed)
{
  ACE_TCHAR ebuffer[512];
  ACE_TCHAR obuffer[512];
  for (size_t i = 0; i < ARRAYSIZE; ++i) {
    if (expected.charValue[i] != observed.charValue[i]) {
      ACE::format_hexdump((char*)&(expected.charValue[i]), sizeof(ACE_CDR::Char), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.charValue[i]), sizeof(ACE_CDR::Char), obuffer, sizeof(obuffer));
      std::cout << "char[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.charValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.charValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.doubleValue[i] != observed.doubleValue[i]) {
      ACE::format_hexdump((char*)&(expected.doubleValue[i]), sizeof(ACE_CDR::Double), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.doubleValue[i]), sizeof(ACE_CDR::Double), obuffer, sizeof(obuffer));
      std::cout << "double[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.doubleValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.doubleValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.floatValue[i] != observed.floatValue[i]) {
      ACE::format_hexdump((char*)&(expected.floatValue[i]), sizeof(ACE_CDR::Float), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.floatValue[i]), sizeof(ACE_CDR::Float), obuffer, sizeof(obuffer));
      std::cout << "float[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.floatValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.floatValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.longdoubleValue[i] != observed.longdoubleValue[i]) {
      ACE::format_hexdump((char*)&(expected.longdoubleValue[i]), sizeof(ACE_CDR::LongDouble), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.longdoubleValue[i]), sizeof(ACE_CDR::LongDouble), obuffer, sizeof(obuffer));
      std::cout << "longdouble[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << "/" << ebuffer;
      std::cout << ", observed: " << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.longlongValue[i] != observed.longlongValue[i]) {
      ACE::format_hexdump((char*)&(expected.longlongValue[i]), sizeof(ACE_CDR::LongLong), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.longlongValue[i]), sizeof(ACE_CDR::LongLong), obuffer, sizeof(obuffer));
      std::cout << "longlong[" << i << "] values not correct after insertion and extraction." << std::endl;
      //std::cout << "(expected: " << expected.longlongValue[i] << "/" << ebuffer;
      //std::cout << ", observed: " << observed.longlongValue[i] << "/" << obuffer;
      //std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.longValue[i] != observed.longValue[i]) {
      ACE::format_hexdump((char*)&(expected.longValue[i]), sizeof(ACE_CDR::Long), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.longValue[i]), sizeof(ACE_CDR::Long), obuffer, sizeof(obuffer));
      std::cout << "long[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.longValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.longValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.octetValue[i] != observed.octetValue[i]) {
      ACE::format_hexdump((char*)&(expected.octetValue[i]), sizeof(ACE_CDR::Octet), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.octetValue[i]), sizeof(ACE_CDR::Octet), obuffer, sizeof(obuffer));
      std::cout << "octet[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.octetValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.octetValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.shortValue[i] != observed.shortValue[i]) {
      ACE::format_hexdump((char*)&(expected.shortValue[i]), sizeof(ACE_CDR::Short), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.shortValue[i]), sizeof(ACE_CDR::Short), obuffer, sizeof(obuffer));
      std::cout << "short[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.shortValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.shortValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.ulonglongValue[i] != observed.ulonglongValue[i]) {
      ACE::format_hexdump((char*)&(expected.ulonglongValue[i]), sizeof(ACE_CDR::ULongLong), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.ulonglongValue[i]), sizeof(ACE_CDR::ULongLong), obuffer, sizeof(obuffer));
      std::cout << "ulonglong[" << i << "] values not correct after insertion and extraction." << std::endl;
      //std::cout << "(expected: " << expected.ulonglongValue[i] << "/" << ebuffer;
      //std::cout << ", observed: " << observed.ulonglongValue[i] << "/" << obuffer;
      //std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.ulongValue[i] != observed.ulongValue[i]) {
      ACE::format_hexdump((char*)&(expected.ulongValue[i]), sizeof(ACE_CDR::ULong), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.ulongValue[i]), sizeof(ACE_CDR::ULong), obuffer, sizeof(obuffer));
      std::cout << "ulong[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.ulongValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.ulongValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.ushortValue[i] != observed.ushortValue[i]) {
      ACE::format_hexdump((char*)&(expected.ushortValue[i]), sizeof(ACE_CDR::UShort), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.ushortValue[i]), sizeof(ACE_CDR::UShort), obuffer, sizeof(obuffer));
      std::cout << "ushort[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.ushortValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.ushortValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.wcharValue[i] != observed.wcharValue[i]) {
      ACE::format_hexdump((char*)&(expected.wcharValue[i]), sizeof(ACE_CDR::WChar), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.wcharValue[i]), sizeof(ACE_CDR::WChar), obuffer, sizeof(obuffer));
      std::cout << "wchar[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.wcharValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.wcharValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
#if OPENDDS_HAS_EXPLICIT_INTS
    if (expected.uint8Value[i] != observed.uint8Value[i]) {
      ACE::format_hexdump((char*)&(expected.uint8Value[i]), sizeof(ACE_CDR::UInt8),
        ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.uint8Value[i]), sizeof(ACE_CDR::UInt8),
        obuffer, sizeof(obuffer));
      std::cout
        << "uint8[" << i << "] values not correct after insertion and extraction." << std::endl
        << "(expected: " << expected.uint8Value[i] << "/" << ebuffer
        << ", observed: " << observed.uint8Value[i] << "/" << obuffer
        << ")." << std::endl;
      failed = true;
    }
    if (expected.int8Value[i] != observed.int8Value[i]) {
      ACE::format_hexdump((char*)&(expected.int8Value[i]), sizeof(ACE_CDR::Int8),
        ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.int8Value[i]), sizeof(ACE_CDR::Int8),
        obuffer, sizeof(obuffer));
      std::cout
        << "int8[" << i << "] values not correct after insertion and extraction." << std::endl
        << "(expected: " << expected.int8Value[i] << "/" << ebuffer
        << ", observed: " << observed.int8Value[i] << "/" << obuffer
        << ")." << std::endl;
      failed = true;
    }
#endif
  }
}

const int chaindefs[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 30, 35, 40, 45, 50, 128, 256, 512, 1024};

void runTest(const Values& expected, const ArrayValues& expectedArray,
  const Encoding& encoding, const bool checkPos)
{
  ACE_Message_Block* testchain = getchain(sizeof(chaindefs)/sizeof(chaindefs[0]), chaindefs);
  const char* out = encoding.endianness() == OpenDDS::DCPS::ENDIAN_NATIVE ? "OUT" : "";
  std::cout << std::endl << "STARTING INSERTION OF SINGLE VALUES WITH" << out << " SWAPPING" << std::endl;
  insertions(testchain, expected, encoding);
  size_t bytesWritten = testchain->total_length();
  std::cout << std::endl << "BYTES WRITTEN: " << bytesWritten << std::endl;
  displayChain(testchain);
  std::cout << "EXTRACTING SINGLE VALUES WITH" << out << " SWAPPING" << std::endl;
  Values observed = {
    0,
#if OPENDDS_HAS_EXPLICIT_INTS
    0,
#endif
    0, 0, 0,
#if OPENDDS_HAS_EXPLICIT_INTS
    0,
#endif
    0, 0, 0, 0, 0,
    ACE_CDR_LONG_DOUBLE_INITIALIZER, 0, 0, 0
#ifndef OPENDDS_SAFETY_PROFILE
    , ""
#endif
#ifdef DDS_HAS_WCHAR
    , 0
#ifndef OPENDDS_SAFETY_PROFILE
    , L""
#endif
#endif
  };
  Serializer serializer(testchain, encoding);
  bool readPosOk = extractions(serializer, observed, encoding, checkPos);
  if (!readPosOk) {
    failed = true;
  }
  if (testchain->total_length()) {
    std::cerr << "ERROR: BYTES READ != BYTES WRITTEN" << std::endl;
    failed = true;
  }
  checkValues(expected, observed);

  serializer.free_string(observed.stringValue);
#ifdef DDS_HAS_WCHAR
  serializer.free_string(observed.wstringValue);
#endif
  testchain->release();

  testchain = getchain(sizeof(chaindefs)/sizeof(chaindefs[0]), chaindefs);
  std::cout << std::endl << "STARTING INSERTION OF ARRAY VALUES WITH" << out << " SWAPPING" << std::endl;
  array_insertions(testchain, expectedArray, ARRAYSIZE, encoding);
  bytesWritten = testchain->total_length();
  std::cout << std::endl << "BYTES WRITTEN: " << bytesWritten << std::endl;
  displayChain(testchain);
  std::cout << "EXTRACTING ARRAY VALUES WITH" << out << " SWAPPING" << std::endl;
  ArrayValues observedArray;
  array_extractions(testchain, observedArray, ARRAYSIZE, encoding);
  if (testchain->total_length()) {
    std::cerr << "ERROR: BYTES READ != BYTES WRITTEN" << std::endl;
    failed = true;
  }
  checkArrayValues(expectedArray, observedArray);
  testchain->release();
}

const int chainCaps[] = {2, 4, 4};

bool runOverrunTest()
{
  std::cout << "\nRunning overrun test..." << std::endl;

  for (size_t i = 0; i < encoding_count; ++i) {
    ACE_Message_Block* chain = getchain(sizeof(chainCaps)/sizeof(chainCaps[0]), chainCaps);

    // Overrun write
    Serializer s1(chain, encodings[i]);
    ACE_CDR::ULong ul = 1234;
    if (!(s1 << ul)) {
      std::cerr << "runOverrunTest: 1st insert ulong using "
                << encodings[i].to_string() << " failed" << std::endl;
      chain->release();
      return false;
    }
    ACE_CDR::Float flt = 3.14f;
    if (!(s1 << flt)) {
      std::cerr << "runOverrunTest: 1st insert float using "
                << encodings[i].to_string() << " failed" << std::endl;
      chain->release();
      return false;
    }
    ACE_CDR::LongLong ll = 987654321;
    if (s1 << ll) {
      std::cerr << "runOverrunTest: 1st insert long long using "
                << encodings[i].to_string()
                << " succeeded when it should've failed" << std::endl;
      chain->release();
      return false;
    }
    chain->release();

    chain = getchain(sizeof(chainCaps)/sizeof(chainCaps[0]), chainCaps);
    Serializer s2(chain, encodings[i]);
    if (!(s2 << ul)) {
      std::cerr << "runOverrunTest: 2nd insert ulong using "
                << encodings[i].to_string() << " failed" << std::endl;
      chain->release();
      return false;
    }
    if (!(s2 << flt)) {
      std::cerr << "runOverrunTest: 2nd insert float using "
                << encodings[i].to_string() << " failed" << std::endl;
      chain->release();
      return false;
    }
    ACE_CDR::UShort us = 12;
    if (!(s2 << us)) {
      std::cerr << "runOverrunTest: 2nd insert ushort using "
                << encodings[i].to_string() << " failed" << std::endl;
      chain->release();
      return false;
    }

    // Overrun read
    Serializer s3(chain, encodings[i]);
    if (!(s3 >> ul)) {
      std::cerr << "runOverrunTest: 1st extract ulong using "
                << encodings[i].to_string() << " failed" << std::endl;
      chain->release();
      return false;
    }
    if (!(s3 >> flt)) {
      std::cerr << "runOverrunTest: 1st extract float using "
                << encodings[i].to_string() << " failed" << std::endl;
      chain->release();
      return false;
    }
    if (s3 >> ll) {
      std::cerr << "runOverrunTest: 1st extract long long using "
                << encodings[i].to_string()
                << " succeeded when it should've failed" << std::endl;
      chain->release();
      return false;
    }
    chain->release();
  }

  return true;
}

bool runEncapsulationOptionsTest()
{
  std::cerr << "\nRunning encapsulation options tests...\n";
  const ACE_CDR::Octet arr[4] = {0};
  OpenDDS::DCPS::EncapsulationHeader encap;
  const OpenDDS::DCPS::Encoding encoding = OpenDDS::DCPS::Encoding(
    OpenDDS::DCPS::Encoding::KIND_XCDR2, OpenDDS::DCPS::ENDIAN_BIG);

  if (!encap.from_encoding(encoding, OpenDDS::DCPS::APPENDABLE)) {
    std::cerr << "EncapsulationHeader::from_encoding failed" << std::endl;
    return false;
  }

  bool status = true;
  for (unsigned int i = 1; i <= OpenDDS::DCPS::EncapsulationHeader::padding_marker_alignment; i++) {
    OpenDDS::DCPS::Message_Block_Ptr mb;
    ACE_Message_Block* tmp_mb;

    ACE_NEW_RETURN(tmp_mb,
      ACE_Message_Block(4 + i, ACE_Message_Block::MB_DATA),
      false);
    mb.reset(tmp_mb);

    OpenDDS::DCPS::Serializer serializer(mb.get(), encoding);
    if (!(serializer << encap) || !serializer.write_octet_array(arr, i)) {
      std::cerr << "Serialization failed in runEncapsulationOptionsTest" << std::endl;
      return false;
    }

    if (!OpenDDS::DCPS::EncapsulationHeader::set_encapsulation_options(mb)) {
      std::cerr << "EncapsulationHeader::set_encapsulation_options failed. Size: " << mb->length() << std::endl;
      return false;
    }

    unsigned int padding_marker_alignment = mb->rd_ptr()[OpenDDS::DCPS::EncapsulationHeader::padding_marker_byte_index] & 0x03;
    if (padding_marker_alignment != (OpenDDS::DCPS::EncapsulationHeader::padding_marker_alignment - i)) {
      std::cerr << "EncapsulationHeader::set_encapsulation_options failed for "
        << i << " bytes, padding marker alignment: " << padding_marker_alignment << std::endl;
      status = false;
    }
  }

  return status;
}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  const char string[] = "This is a test of the string serialization.";
#ifdef DDS_HAS_WCHAR
  const ACE_CDR::WChar wstring[] = L"This is a test of the wstring serialization.";
#endif
#ifndef OPENDDS_SAFETY_PROFILE
  std::string stdstring = "This is a test of the std string serialization.";
#ifdef DDS_HAS_WCHAR
  std::wstring stdwstring = L"This is a test of the std wstring serialization.";
#endif
#endif

  Values expected = {
    0x01,
#if OPENDDS_HAS_EXPLICIT_INTS
    0x11,
#endif
    0x2345,
    0x67abcdef,
    ACE_INT64_LITERAL(0x0123456789abcdef),
#if OPENDDS_HAS_EXPLICIT_INTS
    0x22,
#endif
    0x0123,
    0x456789ab,
    ACE_UINT64_LITERAL(0xcdef0123456789ab),
    0.1f,
    0.2,
#if ACE_SIZEOF_LONG_DOUBLE == 16
    0x89abcdef01234567LL,
#else
    {{0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77}},
#endif
    0x1a,
    0xb2,
    const_cast<ACE_CDR::Char*>(string)
#ifndef OPENDDS_SAFETY_PROFILE
    , stdstring
#endif
#ifdef DDS_HAS_WCHAR
    , const_cast<ACE_CDR::WChar*>(wstring)
#ifndef OPENDDS_SAFETY_PROFILE
    , stdwstring
#endif
#endif
  };

  ArrayValues expectedArray;

  // Initialize the array
  for (size_t i = 0; i < ARRAYSIZE; ++i) {
    expectedArray.octetValue[i] = (0xff&i);
#if OPENDDS_HAS_EXPLICIT_INTS
    expectedArray.int8Value[i] = (0xdd&i);
#endif
    expectedArray.shortValue[i] = (0xffff&i);
    expectedArray.longValue[i] = ACE_CDR::Long(0x0f0f0f0f|i);
    expectedArray.longlongValue[i] = ACE_INT64_LITERAL(0x0123456789abcdef);
#if OPENDDS_HAS_EXPLICIT_INTS
    expectedArray.uint8Value[i] = (0xdd|i);
#endif
    expectedArray.ushortValue[i] = ACE_CDR::UShort(0xffff|i);
    expectedArray.ulongValue[i] = ACE_CDR::ULong(0xf0f0f0f0|i);
    expectedArray.ulonglongValue[i] = ACE_UINT64_LITERAL(0xcdef0123456789ab);
    if (i == 0) {
      expectedArray.floatValue[i] = (float) 0.0;
      expectedArray.doubleValue[i] = (double) 0.0;
    } else {
      expectedArray.floatValue[i] = (float) 1.0 / (float)i;
      expectedArray.doubleValue[i] = (double) 3.0 / (double)i;
    }
#if ACE_SIZEOF_LONG_DOUBLE == 16
    expectedArray.longdoubleValue[i] = 0x89abcdef01234567LL;
#else
    ACE_CDR::LongDouble ldarray = {{0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77}};
    expectedArray.longdoubleValue[i] = ldarray;
#endif
    expectedArray.charValue[i] = (0xff&i);
    expectedArray.wcharValue[i] = (0xff&i);
  }

  std::cout << "Size of Values: " << sizeof(Values) << std::endl;
  std::cout << "Size of ArrayValues: " << sizeof(ArrayValues) << std::endl;

  for (size_t encoding = 0; encoding < encoding_count; ++encoding) {
    std::cout << "\n\n*** " << encodings[encoding].to_string() << std::endl;
    runTest(expected, expectedArray, encodings[encoding], true);
  }

  if (!runOverrunTest()) {
    failed = true;
  }

  if (!runAlignmentTest() || !runAlignmentResetTest() || !runAlignmentOverrunTest()) {
    failed = true;
  }

  if (!runEncapsulationOptionsTest()) {
    failed = true;
  }

  if (failed) {
    std::cerr << std::endl << "SerializerTest FAILED" << std::endl;
  } else {
    std::cout << std::endl << "SerializerTest PASSED" << std::endl;
  }
  return failed ? 1 : 0;
}
