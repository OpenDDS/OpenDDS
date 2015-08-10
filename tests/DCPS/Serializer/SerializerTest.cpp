#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/Definitions.h"

#include "ace/ACE.h"
#include "ace/Arg_Shifter.h"
#include "ace/Get_Opt.h"
#include "ace/Message_Block.h"
#include "ace/OS_NS_string.h"

#include <iostream>

using OpenDDS::DCPS::Serializer;

const size_t ARRAYSIZE = 15;

bool failed = false;

struct Values {
  ACE_CDR::Octet      octetValue;
  ACE_CDR::Short      shortValue;
  ACE_CDR::Long       longValue;
  ACE_CDR::LongLong   longlongValue;
  ACE_CDR::UShort     ushortValue;
  ACE_CDR::ULong      ulongValue;
  ACE_CDR::ULongLong  ulonglongValue;
  ACE_CDR::Float      floatValue;
  ACE_CDR::Double     doubleValue;
  ACE_CDR::LongDouble longdoubleValue;
  ACE_CDR::Char       charValue;
  ACE_CDR::WChar      wcharValue;
  ACE_CDR::Char*      stringValue;
  ACE_CDR::WChar*     wstringValue;
};

struct ArrayValues {
  ACE_CDR::Octet      octetValue[ARRAYSIZE];
  ACE_CDR::Short      shortValue[ARRAYSIZE];
  ACE_CDR::Long       longValue[ARRAYSIZE];
  ACE_CDR::LongLong   longlongValue[ARRAYSIZE];
  ACE_CDR::UShort     ushortValue[ARRAYSIZE];
  ACE_CDR::ULong      ulongValue[ARRAYSIZE];
  ACE_CDR::ULongLong  ulonglongValue[ARRAYSIZE];
  ACE_CDR::Float      floatValue[ARRAYSIZE];
  ACE_CDR::Double     doubleValue[ARRAYSIZE];
  ACE_CDR::LongDouble longdoubleValue[ARRAYSIZE];
  ACE_CDR::Char       charValue[ARRAYSIZE];
  ACE_CDR::WChar      wcharValue[ARRAYSIZE];
};

void
insertions(ACE_Message_Block* chain, const Values& values,
           bool swap, Serializer::Alignment align)
{
  Serializer serializer(chain, swap, align);

  serializer << ACE_OutputCDR::from_octet(values.octetValue);
  serializer << values.shortValue;
  serializer << values.longValue;
  serializer << values.longlongValue;
  serializer << values.ushortValue;
  serializer << values.ulongValue;
  serializer << values.ulonglongValue;
  serializer << values.floatValue;
  serializer << values.doubleValue;
  serializer << values.longdoubleValue;
  serializer << values.charValue;
  serializer << ACE_OutputCDR::from_wchar(values.wcharValue);
  serializer << ACE_OutputCDR::from_string(values.stringValue, 0);
#ifdef DDS_HAS_WCHAR
  serializer << ACE_OutputCDR::from_wstring(values.wstringValue, 0);
#endif
}

void
array_insertions(ACE_Message_Block* chain, const ArrayValues& values,
                 ACE_CDR::ULong length, bool swap, Serializer::Alignment align)
{
  Serializer serializer(chain, swap, align);

  serializer.write_octet_array(values.octetValue, length);
  serializer.write_short_array(values.shortValue, length);
  serializer.write_long_array(values.longValue, length);
  serializer.write_longlong_array(values.longlongValue, length);
  serializer.write_ushort_array(values.ushortValue, length);
  serializer.write_ulong_array(values.ulongValue, length);
  serializer.write_ulonglong_array(values.ulonglongValue, length);
  serializer.write_float_array(values.floatValue, length);
  serializer.write_double_array(values.doubleValue, length);
  serializer.write_longdouble_array(values.longdoubleValue, length);
  serializer.write_char_array(values.charValue, length);
  serializer.write_wchar_array(values.wcharValue, length);
}

void
extractions(ACE_Message_Block* chain, Values& values,
            bool swap, Serializer::Alignment align)
{
  Serializer serializer(chain, swap, align);

  serializer >> ACE_InputCDR::to_octet(values.octetValue);
  serializer >> values.shortValue;
  serializer >> values.longValue;
  serializer >> values.longlongValue;
  serializer >> values.ushortValue;
  serializer >> values.ulongValue;
  serializer >> values.ulonglongValue;
  serializer >> values.floatValue;
  serializer >> values.doubleValue;
  serializer >> values.longdoubleValue;
  serializer >> values.charValue;
  serializer >> ACE_InputCDR::to_wchar(values.wcharValue);
  serializer >> ACE_InputCDR::to_string(values.stringValue, 0);
#ifdef DDS_HAS_WCHAR
  serializer >> ACE_InputCDR::to_wstring(values.wstringValue, 0);
#endif
}

void
array_extractions(ACE_Message_Block* chain, ArrayValues& values,
                  ACE_CDR::ULong length, bool swap, Serializer::Alignment align)
{
  Serializer serializer(chain, swap, align);

  serializer.read_octet_array(values.octetValue, length);
  serializer.read_short_array(values.shortValue, length);
  serializer.read_long_array(values.longValue, length);
  serializer.read_longlong_array(values.longlongValue, length);
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
  if (expected.charValue       != observed.charValue) {
    std::cout << "char values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.charValue << ", observed: " << observed.charValue << ")." << std::endl;
    failed = true;
  }
  if (expected.doubleValue     != observed.doubleValue) {
    std::cout << "double values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.doubleValue << ", observed: " << observed.doubleValue << ")." << std::endl;
    failed = true;
  }
  if (expected.floatValue      != observed.floatValue) {
    std::cout << "float values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.floatValue << ", observed: " << observed.floatValue << ")." << std::endl;
    failed = true;
  }
  if (expected.longdoubleValue != observed.longdoubleValue) {
    std::cout << "longdouble values not correct after insertion and extraction." << std::endl;
    //    std::cout << "(expected: " << expected.longdoubleValue << ", observed: " << observed.longdoubleValue << ")." << std::endl;
    failed = true;
  }
  if (expected.longlongValue   != observed.longlongValue) {
    std::cout << "longlong values not correct after insertion and extraction." << std::endl;
    //std::cout << "(expected: " << expected.longlongValue << ", observed: " << observed.longlongValue << ")." << std::endl;
    failed = true;
  }
  if (expected.longValue       != observed.longValue) {
    std::cout << "long values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.longValue << ", observed: " << observed.longValue << ")." << std::endl;
    failed = true;
  }
  if (expected.octetValue      != observed.octetValue) {
    std::cout << "octet values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.octetValue << ", observed: " << observed.octetValue << ")." << std::endl;
    failed = true;
  }
  if (expected.shortValue      != observed.shortValue) {
    std::cout << "short values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.shortValue << ", observed: " << observed.shortValue << ")." << std::endl;
    failed = true;
  }
  if (expected.ulonglongValue  != observed.ulonglongValue) {
    std::cout << "ulonglong values not correct after insertion and extraction." << std::endl;
    //std::cout << "(expected: " << expected.ulonglongValue << ", observed: " << observed.ulonglongValue << ")." << std::endl;
    failed = true;
  }
  if (expected.ulongValue      != observed.ulongValue) {
    std::cout << "ulong values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.ulongValue << ", observed: " << observed.ulongValue << ")." << std::endl;
    failed = true;
  }
  if (expected.ushortValue     != observed.ushortValue) {
    std::cout << "ushort values not correct after insertion and extraction." << std::endl;
    std::cout << "(expected: " << expected.ushortValue << ", observed: " << observed.ushortValue << ")." << std::endl;
    failed = true;
  }
  if (expected.wcharValue      != observed.wcharValue) {
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
#endif
}

void
checkArrayValues(const ArrayValues& expected, const ArrayValues& observed)
{
  ACE_TCHAR ebuffer[512];
  ACE_TCHAR obuffer[512];
  for (size_t i = 0; i < ARRAYSIZE; ++i) {
    if (expected.charValue[i]       != observed.charValue[i]) {
      std::cout << "char " << i << " values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.charValue[i] << ", observed: " << observed.charValue[i] << ")." << std::endl;
      ACE::format_hexdump((char*)&(expected.charValue[i]), sizeof(ACE_CDR::Char), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.charValue[i]), sizeof(ACE_CDR::Char), obuffer, sizeof(obuffer));
      std::cout << "char[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.charValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.charValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.doubleValue[i]     != observed.doubleValue[i]) {
      ACE::format_hexdump((char*)&(expected.doubleValue[i]), sizeof(ACE_CDR::Double), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.doubleValue[i]), sizeof(ACE_CDR::Double), obuffer, sizeof(obuffer));
      std::cout << "double[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.doubleValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.doubleValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.floatValue[i]      != observed.floatValue[i]) {
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
    if (expected.longlongValue[i]   != observed.longlongValue[i]) {
      ACE::format_hexdump((char*)&(expected.longlongValue[i]), sizeof(ACE_CDR::LongLong), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.longlongValue[i]), sizeof(ACE_CDR::LongLong), obuffer, sizeof(obuffer));
      std::cout << "longlong[" << i << "] values not correct after insertion and extraction." << std::endl;
      //std::cout << "(expected: " << expected.longlongValue[i] << "/" << ebuffer;
      //std::cout << ", observed: " << observed.longlongValue[i] << "/" << obuffer;
      //std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.longValue[i]       != observed.longValue[i]) {
      ACE::format_hexdump((char*)&(expected.longValue[i]), sizeof(ACE_CDR::Long), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.longValue[i]), sizeof(ACE_CDR::Long), obuffer, sizeof(obuffer));
      std::cout << "long[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.longValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.longValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.octetValue[i]      != observed.octetValue[i]) {
      ACE::format_hexdump((char*)&(expected.octetValue[i]), sizeof(ACE_CDR::Octet), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.octetValue[i]), sizeof(ACE_CDR::Octet), obuffer, sizeof(obuffer));
      std::cout << "octet[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.octetValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.octetValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.shortValue[i]      != observed.shortValue[i]) {
      ACE::format_hexdump((char*)&(expected.shortValue[i]), sizeof(ACE_CDR::Short), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.shortValue[i]), sizeof(ACE_CDR::Short), obuffer, sizeof(obuffer));
      std::cout << "short[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.shortValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.shortValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.ulonglongValue[i]  != observed.ulonglongValue[i]) {
      ACE::format_hexdump((char*)&(expected.ulonglongValue[i]), sizeof(ACE_CDR::ULongLong), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.ulonglongValue[i]), sizeof(ACE_CDR::ULongLong), obuffer, sizeof(obuffer));
      std::cout << "ulonglong[" << i << "] values not correct after insertion and extraction." << std::endl;
      //std::cout << "(expected: " << expected.ulonglongValue[i] << "/" << ebuffer;
      //std::cout << ", observed: " << observed.ulonglongValue[i] << "/" << obuffer;
      //std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.ulongValue[i]      != observed.ulongValue[i]) {
      ACE::format_hexdump((char*)&(expected.ulongValue[i]), sizeof(ACE_CDR::ULong), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.ulongValue[i]), sizeof(ACE_CDR::ULong), obuffer, sizeof(obuffer));
      std::cout << "ulong[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.ulongValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.ulongValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.ushortValue[i]     != observed.ushortValue[i]) {
      ACE::format_hexdump((char*)&(expected.ushortValue[i]), sizeof(ACE_CDR::UShort), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.ushortValue[i]), sizeof(ACE_CDR::UShort), obuffer, sizeof(obuffer));
      std::cout << "ushort[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.ushortValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.ushortValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
    if (expected.wcharValue[i]      != observed.wcharValue[i]) {
      ACE::format_hexdump((char*)&(expected.wcharValue[i]), sizeof(ACE_CDR::WChar), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump((char*)&(observed.wcharValue[i]), sizeof(ACE_CDR::WChar), obuffer, sizeof(obuffer));
      std::cout << "wchar[" << i << "] values not correct after insertion and extraction." << std::endl;
      std::cout << "(expected: " << expected.wcharValue[i] << "/" << ebuffer;
      std::cout << ", observed: " << observed.wcharValue[i] << "/" << obuffer;
      std::cout << ")." << std::endl;
      failed = true;
    }
  }
}

const int chaindefs[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 30, 35, 40, 45, 50, 128, 256, 512, 1024};

void
runTest(const Values& expected, const ArrayValues& expectedArray,
        bool swap, Serializer::Alignment align)
{
  ACE_Message_Block* testchain = getchain(sizeof(chaindefs)/sizeof(chaindefs[0]), chaindefs);
  const char* out = swap ? "" : "OUT";
  std::cout << std::endl << "STARTING INSERTION OF SINGLE VALUES WITH" << out << " SWAPPING" << std::endl;
  insertions(testchain, expected, swap, align);
  size_t bytesWritten = testchain->total_length();
  std::cout << std::endl << "BYTES WRITTEN: " << bytesWritten << std::endl;
  displayChain(testchain);
  std::cout << "EXTRACTING SINGLE VALUES WITH" << out << " SWAPPING" << std::endl;
  Values observed = {0, 0, 0, 0, 0, 0, 0, 0, 0,
                     ACE_CDR_LONG_DOUBLE_INITIALIZER, 0, 0, 0, 0};
  extractions(testchain, observed, swap, align);
  if (testchain->total_length()) {
    std::cout << "ERROR: BYTES READ != BYTES WRITTEN" << std::endl;
    failed = true;
  }
  checkValues(expected, observed);
  testchain->release();

  testchain = getchain(sizeof(chaindefs)/sizeof(chaindefs[0]), chaindefs);
  std::cout << std::endl << "STARTING INSERTION OF ARRAY VALUES WITH" << out << " SWAPPING" << std::endl;
  array_insertions(testchain, expectedArray, ARRAYSIZE, swap, align);
  bytesWritten = testchain->total_length();
  std::cout << std::endl << "BYTES WRITTEN: " << bytesWritten << std::endl;
  displayChain(testchain);
  std::cout << "EXTRACTING ARRAY VALUES WITH" << out << " SWAPPING" << std::endl;
  ArrayValues observedArray;
  array_extractions(testchain, observedArray, ARRAYSIZE, swap, align);
  if (testchain->total_length()) {
    std::cout << "ERROR: BYTES READ != BYTES WRITTEN" << std::endl;
    failed = true;
  }
  checkArrayValues(expectedArray, observedArray);
  testchain->release();
}

bool runAlignmentTest();
bool runAlignmentResetTest();

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  const char string[] = "This is a test of the string serialization.";
#ifdef DDS_HAS_WCHAR
  const ACE_CDR::WChar wstring[] = L"This is a test of the wstring serialization.";
#endif

  Values expected = { 0x01,
                      0x2345,
                      0x67abcdef,
                      ACE_INT64_LITERAL(0x0123456789abcdef),
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
                      const_cast<ACE_CDR::Char*>(string),
#ifdef DDS_HAS_WCHAR
                      const_cast<ACE_CDR::WChar*>(wstring)
#else
                      0
#endif
  };

  ArrayValues expectedArray;

  // Initialize the array
  for (size_t i = 0; i < ARRAYSIZE; ++i) {
    expectedArray.octetValue[i] = (0xff&i);
    expectedArray.shortValue[i] = (0xffff&i);
    expectedArray.longValue[i] = ACE_CDR::Long(0x0f0f0f0f|i);
    expectedArray.longlongValue[i] = ACE_INT64_LITERAL(0x0123456789abcdef);
    expectedArray.ushortValue[i] = ACE_CDR::UShort(0xffff|i);
    expectedArray.ulongValue[i] = ACE_CDR::ULong(0xf0f0f0f0|i);
    expectedArray.ulonglongValue[i] = ACE_UINT64_LITERAL(0xcdef0123456789ab);
    expectedArray.floatValue[i] = (float) 1.0 / (float) i;
    expectedArray.doubleValue[i] = (double) 3.0 / (double) i;
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

  std::cout << "\n\n*** Alignment = ALIGN_NONE" << std::endl;
  Serializer::Alignment align = Serializer::ALIGN_NONE;
  runTest(expected, expectedArray, true /*swap*/, align);
  runTest(expected, expectedArray, false /*swap*/, align);

  std::cout << "\n\n*** Alignment = ALIGN_INITIALIZE" << std::endl;
  align = Serializer::ALIGN_INITIALIZE;
  runTest(expected, expectedArray, true /*swap*/, align);
  runTest(expected, expectedArray, false /*swap*/, align);

  std::cout << "\n\n*** Alignment = ALIGN_CDR" << std::endl;
  align = Serializer::ALIGN_CDR;
  runTest(expected, expectedArray, true /*swap*/, align);
  runTest(expected, expectedArray, false /*swap*/, align);

  if (!runAlignmentTest() || !runAlignmentResetTest()) {
    failed = true;
  }

  if (failed) {
    std::cerr << std::endl << "SerializerTest FAILED" << std::endl;
  } else {
    std::cout << std::endl << "SerializerTest PASSED" << std::endl;
  }
  return failed ? 1 : 0;
}
