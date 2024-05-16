/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/XTypes/TypeObject.h"

#include "gtest/gtest.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using namespace OpenDDS::XTypes;
using namespace OpenDDS::DCPS;

const Encoding xcdr2_be(Encoding::KIND_XCDR2, ENDIAN_BIG);
const Encoding xcdr2_le(Encoding::KIND_XCDR2, ENDIAN_LITTLE);

class Destruction
{
public:
  Destruction(bool& ref)
    : ref_(ref) {
    ref_ = false;
  }
  virtual ~Destruction() {
    ref_ = true;
  }
private:
  bool& ref_;
};

// @appendable
struct Struct {
  ACE_CDR::ULong member;
};

void serialized_size(const Encoding& encoding, size_t& size, const Struct&)
{
  primitive_serialized_size_ulong(encoding, size); // DHeader
  primitive_serialized_size_ulong(encoding, size); // member
}

bool operator<<(Serializer& strm, const Struct& stru)
{
  return strm << ACE_CDR::ULong(4)
    && strm << stru.member;
}

bool operator>>(Serializer& strm, Struct& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }
  const size_t end_of_stru = strm.rpos() + total_size;
  if (!(strm >> stru.member)) {
    return false;
  }
  return strm.skip(end_of_stru - strm.rpos());
}

template<typename T, size_t N>
void check_encode(const T& object, const Encoding& enc, const unsigned char (&expected)[N])
{
  const size_t size = serialized_size(enc, object);
  EXPECT_EQ(size, N);
  ACE_Message_Block buffer(size);
  {
    Serializer ser(&buffer, enc);
    EXPECT_TRUE(ser << object);
    EXPECT_EQ(buffer.length(), N);
    EXPECT_EQ(0, std::memcmp(expected, buffer.rd_ptr(), N));
  }
  Serializer ser(&buffer, enc);
  T object2;
  EXPECT_TRUE(ser >> object2);
}

template<typename T, size_t N>
void check_decode(T& object, const Encoding& enc, const char (&expected)[N])
{
  ACE_Message_Block buffer(expected, N);
  buffer.wr_ptr(N);
  Serializer ser(&buffer, enc);
  EXPECT_TRUE(ser >> object);
  EXPECT_EQ(buffer.length(), 0U);
}

TypeObject getTypeObject()
{
  using namespace OpenDDS;
  return XTypes::TypeObject(XTypes::MinimalTypeObject(XTypes::MinimalStructType(XTypes::IS_APPENDABLE, XTypes::MinimalStructHeader(XTypes::TypeIdentifier(XTypes::TK_NONE), XTypes::MinimalTypeDetail()), XTypes::MinimalStructMemberSeq().append(XTypes::MinimalStructMember(XTypes::CommonStructMember(0, XTypes::TRY_CONSTRUCT1 | XTypes::IS_KEY, XTypes::TypeIdentifier(XTypes::TK_INT32)), XTypes::MinimalMemberDetail(60, 110, 11, 138))))));
}

std::ostream& operator<<(std::ostream& os, const TypeIdentifier& ti)
{
  std::ostream os_hex(os.rdbuf());
  os_hex << std::hex << std::showbase;
  os_hex << "Kind: " << int(ti.kind()) << ' ';
  if (ti.kind() == EK_COMPLETE || ti.kind() == EK_MINIMAL) {
    os_hex << "Equiv Hash: ";
    for (size_t i = 0; i < sizeof ti.equivalence_hash(); ++i) {
      os_hex << int(ti.equivalence_hash()[i]) << ' ';
    }
  }
  return os << '\n';
}

TEST(dds_DCPS_XTypes_TypeObject, maintest)
{
  Sequence<ACE_CDR::ULong> sequ;
  const unsigned char length_only[] = {0, 0, 0, 0};
  check_encode(sequ, xcdr2_be, length_only);
  check_encode(sequ, xcdr2_le, length_only);

  Sequence<Struct> seqs;
  const unsigned char dheader_length_be[] = {
    0, 0, 0, 4,
    0, 0, 0, 0,
  };
  const unsigned char dheader_length_le[] = {
    4, 0, 0, 0,
    0, 0, 0, 0,
  };
  check_encode(seqs, xcdr2_be, dheader_length_be);
  check_encode(seqs, xcdr2_le, dheader_length_le);

  const char appended_be[] = {
    0, 0, 0, 16, // DHeader (seq)
    0, 0, 0, 1,  // Length
    0, 0, 0, 8,  // DHeader (structure)
    0, 0, 0, 2,  // member
    1, 2, 3, 4,  // extra appended data
  };
  check_decode(seqs, xcdr2_be, appended_be);
  EXPECT_EQ(seqs.length(), 1U);
  EXPECT_EQ(seqs[0].member, 2U);

  const char invalid_encoding[] = {0, 0, 0, 0};
  check_decode(seqs, xcdr2_be, invalid_encoding);
  EXPECT_EQ(seqs.length(), 0U);

  const TypeObject testObject = getTypeObject();
  const TypeIdentifier goodIdentifier = makeTypeIdentifier(testObject);
  //std::cout << goodIdentifier << std::endl;
  using namespace OpenDDS;
  const XTypes::TypeIdentifier good(XTypes::EK_MINIMAL, XTypes::EquivalenceHashWrapper(47, 43, 14, 5, 138, 206, 143, 33, 189, 131, 116, 89, 176, 60));
  EXPECT_EQ(goodIdentifier, good);

  Encoding encoding(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  encoding.skip_sequence_dheader(true);
  const TypeIdentifier badIdentifier = makeTypeIdentifier(testObject, &encoding);
  //std::cout << badIdentifier << std::endl;
  const XTypes::TypeIdentifier bad(XTypes::EK_MINIMAL,XTypes::EquivalenceHashWrapper(100, 92, 123, 199, 182, 88, 54, 251, 172, 100, 66, 123, 137, 217));
  EXPECT_EQ(badIdentifier, bad);
}

TEST(dds_DCPS_XTypes_TypeObject, Sequence_equal)
{
  Sequence<int> uut1;
  Sequence<int> uut2;
  uut2.append(1);
  Sequence<int> uut3;
  uut3.append(1);
  Sequence<int> uut4;
  uut4.append(1);
  uut4.append(2);

  EXPECT_EQ(uut2, uut3);

  EXPECT_NE(uut1, uut2);
  EXPECT_NE(uut3, uut4);
}

TEST(dds_DCPS_XTypes_TypeObject, ExtendedAnnotationParameterValue_equal)
{
  ExtendedAnnotationParameterValue uut1;
  ExtendedAnnotationParameterValue uut2;

  EXPECT_EQ(uut1, uut2);
}

TEST(dds_DCPS_XTypes_TypeObject, AnnotationParameterValue_equal)
{
  {
    AnnotationParameterValue uut1;
    AnnotationParameterValue uut2;

    EXPECT_EQ(uut1.kind(), TK_NONE);
    EXPECT_EQ(uut2.kind(), TK_NONE);
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_BOOLEAN);
    AnnotationParameterValue uut2(TK_BOOLEAN);

    uut1.boolean_value() = true;
    uut2.boolean_value() = true;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_BOOLEAN);
    AnnotationParameterValue uut2(TK_BOOLEAN);

    uut1.boolean_value() = true;
    uut2.boolean_value() = false;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_BYTE);
    AnnotationParameterValue uut2(TK_BYTE);

    uut1.byte_value() = 1;
    uut2.byte_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_BYTE);
    AnnotationParameterValue uut2(TK_BYTE);

    uut1.byte_value() = 1;
    uut2.byte_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_INT8);
    AnnotationParameterValue uut2(TK_INT8);

    uut1.int8_value() = 1;
    uut2.int8_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_INT8);
    AnnotationParameterValue uut2(TK_INT8);

    uut1.int8_value() = 1;
    uut2.int8_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_UINT8);
    AnnotationParameterValue uut2(TK_UINT8);

    uut1.uint8_value() = 1;
    uut2.uint8_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_UINT8);
    AnnotationParameterValue uut2(TK_UINT8);

    uut1.uint8_value() = 1;
    uut2.uint8_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_INT16);
    AnnotationParameterValue uut2(TK_INT16);

    uut1.int16_value() = 1;
    uut2.int16_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_INT16);
    AnnotationParameterValue uut2(TK_INT16);

    uut1.int16_value() = 1;
    uut2.int16_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_UINT16);
    AnnotationParameterValue uut2(TK_UINT16);

    uut1.uint16_value() = 1;
    uut2.uint16_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_UINT16);
    AnnotationParameterValue uut2(TK_UINT16);

    uut1.uint16_value() = 1;
    uut2.uint16_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_INT32);
    AnnotationParameterValue uut2(TK_INT32);

    uut1.int32_value() = 1;
    uut2.int32_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_INT32);
    AnnotationParameterValue uut2(TK_INT32);

    uut1.int32_value() = 1;
    uut2.int32_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_UINT32);
    AnnotationParameterValue uut2(TK_UINT32);

    uut1.uint32_value() = 1;
    uut2.uint32_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_UINT32);
    AnnotationParameterValue uut2(TK_UINT32);

    uut1.uint32_value() = 1;
    uut2.uint32_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_INT64);
    AnnotationParameterValue uut2(TK_INT64);

    uut1.int64_value() = 1;
    uut2.int64_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_INT64);
    AnnotationParameterValue uut2(TK_INT64);

    uut1.int64_value() = 1;
    uut2.int64_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_UINT64);
    AnnotationParameterValue uut2(TK_UINT64);

    uut1.uint64_value() = 1;
    uut2.uint64_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_UINT64);
    AnnotationParameterValue uut2(TK_UINT64);

    uut1.uint64_value() = 1;
    uut2.uint64_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_FLOAT32);
    AnnotationParameterValue uut2(TK_FLOAT32);

    uut1.float32_value() = 1;
    uut2.float32_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_FLOAT32);
    AnnotationParameterValue uut2(TK_FLOAT32);

    uut1.float32_value() = 1;
    uut2.float32_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_FLOAT64);
    AnnotationParameterValue uut2(TK_FLOAT64);

    uut1.float64_value() = 1;
    uut2.float64_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_FLOAT64);
    AnnotationParameterValue uut2(TK_FLOAT64);

    uut1.float64_value() = 1;
    uut2.float64_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_FLOAT128);
    AnnotationParameterValue uut2(TK_FLOAT128);

    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(uut1.float128_value(), 1.0);
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(uut2.float128_value(), 1.0);
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_FLOAT128);
    AnnotationParameterValue uut2(TK_FLOAT128);

    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(uut1.float128_value(), 1.0);
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(uut2.float128_value(), 2.0);
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_CHAR8);
    AnnotationParameterValue uut2(TK_CHAR8);

    uut1.char_value() = 1;
    uut2.char_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_CHAR8);
    AnnotationParameterValue uut2(TK_CHAR8);

    uut1.char_value() = 1;
    uut2.char_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_CHAR16);
    AnnotationParameterValue uut2(TK_CHAR16);

    uut1.wchar_value() = 1;
    uut2.wchar_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_CHAR16);
    AnnotationParameterValue uut2(TK_CHAR16);

    uut1.wchar_value() = 1;
    uut2.wchar_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_ENUM);
    AnnotationParameterValue uut2(TK_ENUM);

    uut1.enumerated_value() = 1;
    uut2.enumerated_value() = 1;
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_ENUM);
    AnnotationParameterValue uut2(TK_ENUM);

    uut1.enumerated_value() = 1;
    uut2.enumerated_value() = 2;
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_STRING8);
    AnnotationParameterValue uut2(TK_STRING8);

    uut1.string8_value() = "1";
    uut2.string8_value() = "1";
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_STRING8);
    AnnotationParameterValue uut2(TK_STRING8);

    uut1.string8_value() = "1";
    uut2.string8_value() = "2";
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_STRING16);
    AnnotationParameterValue uut2(TK_STRING16);

    uut1.string16_value() = L"1";
    uut2.string16_value() = L"1";
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_STRING16);
    AnnotationParameterValue uut2(TK_STRING16);

    uut1.string16_value() = L"1";
    uut2.string16_value() = L"2";
    EXPECT_NE(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_STRING16);
    AnnotationParameterValue uut2(TK_STRING16);

    uut1.string16_value() = L"1";
    uut2.string16_value() = L"1";
    EXPECT_EQ(uut1, uut2);
  }

  {
    AnnotationParameterValue uut1(TK_NONE);
    AnnotationParameterValue uut2(TK_BOOLEAN);

    uut2.boolean_value() = false;
    EXPECT_NE(uut1, uut2);
  }
}

TEST(dds_DCPS_XTypes_TypeObject, AppliedAnnotationParameter_equal)
{
  AppliedAnnotationParameter uut1;
  uut1.paramname_hash[0] = 1;
  uut1.value = AnnotationParameterValue(TK_BOOLEAN);
  uut1.value.boolean_value() = true;
  AppliedAnnotationParameter uut2;
  uut2.paramname_hash[0] = 1;
  uut2.value = AnnotationParameterValue(TK_BOOLEAN);
  uut2.value.boolean_value() = true;
  AppliedAnnotationParameter uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, AppliedVerbatimAnnotation_equal)
{
  AppliedVerbatimAnnotation uut1("placement", "language", "text");
  AppliedVerbatimAnnotation uut2("placement", "language", "text");
  AppliedVerbatimAnnotation uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, AppliedBuiltinMemberAnnotations_equal)
{
  AppliedBuiltinMemberAnnotations uut1;
  uut1.unit = OPENDDS_OPTIONAL_NS::optional<OpenDDS::DCPS::String>("meters");
  AppliedBuiltinMemberAnnotations uut2;
  uut2.unit = OPENDDS_OPTIONAL_NS::optional<OpenDDS::DCPS::String>("meters");
  AppliedBuiltinMemberAnnotations uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonStructMember_equal)
{
  CommonStructMember uut1(1, 2, TypeIdentifier(TK_BOOLEAN));
  CommonStructMember uut2(1, 2, TypeIdentifier(TK_BOOLEAN));
  CommonStructMember uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteMemberDetail_equal)
{
  CompleteMemberDetail uut1;
  uut1.name = "name";
  CompleteMemberDetail uut2;
  uut2.name = "name";
  CompleteMemberDetail uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalMemberDetail_equal)
{
  MinimalMemberDetail uut1;
  uut1.name_hash[0] = 1;
  MinimalMemberDetail uut2;
  uut2.name_hash[0] = 1;
  MinimalMemberDetail uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteStructMember_equal)
{
  CompleteStructMember uut1;
  uut1.detail.name = "name";
  CompleteStructMember uut2;
  uut2.detail.name = "name";
  CompleteStructMember uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalStructMember_equal)
{
  MinimalStructMember uut1;
  uut1.detail.name_hash[0] = 1;
  MinimalStructMember uut2;
  uut2.detail.name_hash[0] = 1;
  MinimalStructMember uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, AppliedBuiltinTypeAnnotations_equal)
{
  AppliedVerbatimAnnotation value;
  value.text = "text";

  AppliedBuiltinTypeAnnotations uut1;
  uut1.verbatim = OPENDDS_OPTIONAL_NS::optional<AppliedVerbatimAnnotation>(value);
  AppliedBuiltinTypeAnnotations uut2;
  uut2.verbatim = OPENDDS_OPTIONAL_NS::optional<AppliedVerbatimAnnotation>(value);
  AppliedBuiltinTypeAnnotations uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalTypeDetail_equal)
{
  MinimalTypeDetail uut1;
  MinimalTypeDetail uut2;

  EXPECT_EQ(uut1, uut2);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteTypeDetail_equal)
{
  CompleteTypeDetail uut1;
  uut1.type_name = "name";
  CompleteTypeDetail uut2;
  uut2.type_name = "name";
  CompleteTypeDetail uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteStructHeader_equal)
{
  CompleteStructHeader uut1;
  uut1.detail.type_name = "name";
  CompleteStructHeader uut2;
  uut2.detail.type_name = "name";
  CompleteStructHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalStructHeader_equal)
{
  MinimalStructHeader uut1;
  uut1.base_type = TypeIdentifier(TK_BOOLEAN);
  MinimalStructHeader uut2;
  uut2.base_type = TypeIdentifier(TK_BOOLEAN);
  MinimalStructHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteStructType_equal)
{
  CompleteStructType uut1;
  uut1.struct_flags = 1;
  uut1.header.detail.type_name = "name";
  CompleteStructType uut2;
  uut2.struct_flags = 1;
  uut2.header.detail.type_name = "name";
  CompleteStructType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalStructType_equal)
{
  MinimalStructType uut1;
  uut1.struct_flags = 1;
  uut1.header.base_type = TypeIdentifier(TK_BOOLEAN);
  MinimalStructType uut2;
  uut2.struct_flags = 1;
  uut2.header.base_type = TypeIdentifier(TK_BOOLEAN);
  MinimalStructType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonUnionMember_equal)
{
  CommonUnionMember uut1;
  uut1.member_id = 1;
  uut1.member_flags = 2;
  CommonUnionMember uut2;
  uut2.member_id = 1;
  uut2.member_flags = 2;
  CommonUnionMember uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteUnionMember_equal)
{
  CompleteUnionMember uut1;
  uut1.common.member_id = 1;
  uut1.common.member_flags = 2;
  CompleteUnionMember uut2;
  uut2.common.member_id = 1;
  uut2.common.member_flags = 2;
  CompleteUnionMember uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalUnionMember_equal)
{
  MinimalUnionMember uut1;
  uut1.common.member_id = 1;
  uut1.common.member_flags = 2;
  MinimalUnionMember uut2;
  uut2.common.member_id = 1;
  uut2.common.member_flags = 2;
  MinimalUnionMember uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonDiscriminatorMember_equal)
{
  CommonDiscriminatorMember uut1;
  uut1.member_flags = 2;
  uut1.type_id = TypeIdentifier(TK_BOOLEAN);
  CommonDiscriminatorMember uut2;
  uut2.member_flags = 2;
  uut2.type_id = TypeIdentifier(TK_BOOLEAN);
  CommonDiscriminatorMember uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteDiscriminatorMember_equal)
{
  CompleteDiscriminatorMember uut1;
  uut1.common.member_flags = 2;
  uut1.common.type_id = TypeIdentifier(TK_BOOLEAN);
  CompleteDiscriminatorMember uut2;
  uut2.common.member_flags = 2;
  uut2.common.type_id = TypeIdentifier(TK_BOOLEAN);
  CompleteDiscriminatorMember uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalDiscriminatorMember_equal)
{
  MinimalDiscriminatorMember uut1;
  uut1.common.member_flags = 2;
  uut1.common.type_id = TypeIdentifier(TK_BOOLEAN);
  MinimalDiscriminatorMember uut2;
  uut2.common.member_flags = 2;
  uut2.common.type_id = TypeIdentifier(TK_BOOLEAN);
  MinimalDiscriminatorMember uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteUnionHeader_equal)
{
  CompleteUnionHeader uut1;
  uut1.detail.type_name = "name";
  CompleteUnionHeader uut2;
  uut2.detail.type_name = "name";
  CompleteUnionHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalUnionHeader_equal)
{
  MinimalUnionHeader uut1;
  MinimalUnionHeader uut2;

  EXPECT_EQ(uut1, uut2);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteUnionType_equal)
{
  CompleteUnionType uut1;
  uut1.union_flags = 1;
  uut1.header.detail.type_name = "name";
  uut1.discriminator.common.member_flags = 2;
  uut1.discriminator.common.type_id = TypeIdentifier(TK_BOOLEAN);
  CompleteUnionType uut2;
  uut2.union_flags = 1;
  uut2.header.detail.type_name = "name";
  uut2.discriminator.common.member_flags = 2;
  uut2.discriminator.common.type_id = TypeIdentifier(TK_BOOLEAN);
  CompleteUnionType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalUnionType_equal)
{
  MinimalUnionType uut1;
  uut1.union_flags = 1;
  uut1.discriminator.common.member_flags = 2;
  uut1.discriminator.common.type_id = TypeIdentifier(TK_BOOLEAN);
  MinimalUnionType uut2;
  uut2.union_flags = 1;
  uut2.discriminator.common.member_flags = 2;
  uut2.discriminator.common.type_id = TypeIdentifier(TK_BOOLEAN);
  MinimalUnionType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonAnnotationParameter_equal)
{
  CommonAnnotationParameter uut1;
  uut1.member_flags = 1;
  uut1.member_type_id = TypeIdentifier(TK_BOOLEAN);
  CommonAnnotationParameter uut2;
  uut2.member_flags = 1;
  uut2.member_type_id = TypeIdentifier(TK_BOOLEAN);
  CommonAnnotationParameter uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteAnnotationParameter_equal)
{
  CompleteAnnotationParameter uut1;
  uut1.common.member_flags = 1;
  uut1.common.member_type_id = TypeIdentifier(TK_BOOLEAN);
  uut1.name = "name";
  uut1.default_value = AnnotationParameterValue(TK_BOOLEAN);
  uut1.default_value.boolean_value() = true;
  CompleteAnnotationParameter uut2;
  uut2.common.member_flags = 1;
  uut2.common.member_type_id = TypeIdentifier(TK_BOOLEAN);
  uut2.name = "name";
  uut2.default_value = AnnotationParameterValue(TK_BOOLEAN);
  uut2.default_value.boolean_value() = true;
  CompleteAnnotationParameter uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalAnnotationParameter_equal)
{
  MinimalAnnotationParameter uut1;
  uut1.common.member_flags = 1;
  uut1.common.member_type_id = TypeIdentifier(TK_BOOLEAN);
  uut1.name_hash[0] = 1;
  uut1.default_value = AnnotationParameterValue(TK_BOOLEAN);
  uut1.default_value.boolean_value() = true;
  MinimalAnnotationParameter uut2;
  uut2.common.member_flags = 1;
  uut2.common.member_type_id = TypeIdentifier(TK_BOOLEAN);
  uut2.name_hash[0] = 1;
  uut2.default_value = AnnotationParameterValue(TK_BOOLEAN);
  uut2.default_value.boolean_value() = true;
  MinimalAnnotationParameter uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteAnnotationHeader_equal)
{
  CompleteAnnotationHeader uut1;
  uut1.annotation_name = "name";
  CompleteAnnotationHeader uut2;
  uut2.annotation_name = "name";
  CompleteAnnotationHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalAnnotationHeader_equal)
{
  MinimalAnnotationHeader uut1;
  MinimalAnnotationHeader uut2;

  EXPECT_EQ(uut1, uut2);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteAnnotationType_equal)
{
  CompleteAnnotationType uut1;
  uut1.annotation_flag = 1;
  uut1.header.annotation_name = "name";
  CompleteAnnotationType uut2;
  uut2.annotation_flag = 1;
  uut2.header.annotation_name = "name";
  CompleteAnnotationType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalAnnotationType_equal)
{
  MinimalAnnotationType uut1;
  uut1.annotation_flag = 1;
  MinimalAnnotationType uut2;
  uut2.annotation_flag = 1;
  MinimalAnnotationType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonAliasBody_equal)
{
  CommonAliasBody uut1;
  uut1.related_flags = 1;
  uut1.related_type = TypeIdentifier(TK_BOOLEAN);
  CommonAliasBody uut2;
  uut2.related_flags = 1;
  uut2.related_type = TypeIdentifier(TK_BOOLEAN);
  CommonAliasBody uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteAliasBody_equal)
{
  CompleteAliasBody uut1;
  uut1.common.related_flags = 1;
  uut1.common.related_type = TypeIdentifier(TK_BOOLEAN);
  CompleteAliasBody uut2;
  uut2.common.related_flags = 1;
  uut2.common.related_type = TypeIdentifier(TK_BOOLEAN);
  CompleteAliasBody uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalAliasBody_equal)
{
  MinimalAliasBody uut1;
  uut1.common.related_flags = 1;
  uut1.common.related_type = TypeIdentifier(TK_BOOLEAN);
  MinimalAliasBody uut2;
  uut2.common.related_flags = 1;
  uut2.common.related_type = TypeIdentifier(TK_BOOLEAN);
  MinimalAliasBody uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteAliasHeader_equal)
{
  CompleteAliasHeader uut1;
  uut1.detail.type_name = "name";
  CompleteAliasHeader uut2;
  uut2.detail.type_name = "name";
  CompleteAliasHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalAliasHeader_equal)
{
  MinimalAliasHeader uut1;
  MinimalAliasHeader uut2;

  EXPECT_EQ(uut1, uut2);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteAliasType_equal)
{
  CompleteAliasType uut1;
  uut1.alias_flags = 1;
  uut1.header.detail.type_name = "name";
  uut1.body.common.related_flags = 2;
  uut1.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  CompleteAliasType uut2;
  uut2.alias_flags = 1;
  uut2.header.detail.type_name = "name";
  uut2.body.common.related_flags = 2;
  uut2.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  CompleteAliasType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalAliasType_equal)
{
  MinimalAliasType uut1;
  uut1.alias_flags = 1;
  uut1.body.common.related_flags = 2;
  uut1.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  MinimalAliasType uut2;
  uut2.alias_flags = 1;
  uut2.body.common.related_flags = 2;
  uut2.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  MinimalAliasType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteElementDetail_equal)
{
  AppliedBuiltinMemberAnnotations builtin;
  builtin.unit = OPENDDS_OPTIONAL_NS::optional<OpenDDS::DCPS::String>("meters");

  CompleteElementDetail uut1;
  uut1.ann_builtin = OPENDDS_OPTIONAL_NS::optional<AppliedBuiltinMemberAnnotations>(builtin);
  CompleteElementDetail uut2;
  uut2.ann_builtin = OPENDDS_OPTIONAL_NS::optional<AppliedBuiltinMemberAnnotations>(builtin);
  CompleteElementDetail uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonCollectionElement_equal)
{
  CommonCollectionElement uut1;
  uut1.element_flags = 1;
  uut1.type = TypeIdentifier(TK_BOOLEAN);
  CommonCollectionElement uut2;
  uut2.element_flags = 1;
  uut2.type = TypeIdentifier(TK_BOOLEAN);
  CommonCollectionElement uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteCollectionElement_equal)
{
  CompleteCollectionElement uut1;
  uut1.common.element_flags = 1;
  uut1.common.type = TypeIdentifier(TK_BOOLEAN);
  CompleteCollectionElement uut2;
  uut2.common.element_flags = 1;
  uut2.common.type = TypeIdentifier(TK_BOOLEAN);
  CompleteCollectionElement uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalCollectionElement_equal)
{
  MinimalCollectionElement uut1;
  uut1.common.element_flags = 1;
  uut1.common.type = TypeIdentifier(TK_BOOLEAN);
  MinimalCollectionElement uut2;
  uut2.common.element_flags = 1;
  uut2.common.type = TypeIdentifier(TK_BOOLEAN);
  MinimalCollectionElement uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonCollectionHeader_equal)
{
  CommonCollectionHeader uut1;
  uut1.bound = 1;
  CommonCollectionHeader uut2;
  uut2.bound = 1;
  CommonCollectionHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteCollectionHeader_equal)
{
  CompleteCollectionHeader uut1;
  uut1.common.bound = 1;
  CompleteCollectionHeader uut2;
  uut2.common.bound = 1;
  CompleteCollectionHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalCollectionHeader_equal)
{
  MinimalCollectionHeader uut1;
  uut1.common.bound = 1;
  MinimalCollectionHeader uut2;
  uut2.common.bound = 1;
  MinimalCollectionHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteSequenceType_equal)
{
  CompleteSequenceType uut1;
  uut1.collection_flag = 1;
  uut1.header.common.bound = 1;
  uut1.element.common.element_flags = 2;
  uut1.element.common.type = TypeIdentifier(TK_BOOLEAN);
  CompleteSequenceType uut2;
  uut2.collection_flag = 1;
  uut2.header.common.bound = 1;
  uut2.element.common.element_flags = 2;
  uut2.element.common.type = TypeIdentifier(TK_BOOLEAN);
  CompleteSequenceType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalSequenceType_equal)
{
  MinimalSequenceType uut1;
  uut1.collection_flag = 1;
  uut1.header.common.bound = 1;
  uut1.element.common.element_flags = 2;
  uut1.element.common.type = TypeIdentifier(TK_BOOLEAN);
  MinimalSequenceType uut2;
  uut2.collection_flag = 1;
  uut2.header.common.bound = 1;
  uut2.element.common.element_flags = 2;
  uut2.element.common.type = TypeIdentifier(TK_BOOLEAN);
  MinimalSequenceType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonArrayHeader_equal)
{
  CommonArrayHeader uut1;
  uut1.bound_seq.length(1);
  uut1.bound_seq[0] = 38;
  CommonArrayHeader uut2;
  uut2.bound_seq.length(1);
  uut2.bound_seq[0] = 38;
  CommonArrayHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteArrayHeader_equal)
{
  CompleteArrayHeader uut1;
  uut1.common.bound_seq.length(1);
  uut1.common.bound_seq[0] = 38;
  uut1.detail.type_name = "name";
  CompleteArrayHeader uut2;
  uut2.common.bound_seq.length(1);
  uut2.common.bound_seq[0] = 38;
  uut2.detail.type_name = "name";
  CompleteArrayHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalArrayHeader_equal)
{
  MinimalArrayHeader uut1;
  uut1.common.bound_seq.length(1);
  uut1.common.bound_seq[0] = 38;
  MinimalArrayHeader uut2;
  uut2.common.bound_seq.length(1);
  uut2.common.bound_seq[0] = 38;
  MinimalArrayHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteArrayType_equal)
{
  CompleteArrayType uut1;
  uut1.collection_flag = 1;
  uut1.header.common.bound_seq.length(1);
  uut1.header.common.bound_seq[0] = 38;
  uut1.header.detail.type_name = "name";
  uut1.element.common.element_flags = 1;
  uut1.element.common.type = TypeIdentifier(TK_BOOLEAN);
  CompleteArrayType uut2;
  uut2.collection_flag = 1;
  uut2.header.common.bound_seq.length(1);
  uut2.header.common.bound_seq[0] = 38;
  uut2.header.detail.type_name = "name";
  uut2.element.common.element_flags = 1;
  uut2.element.common.type = TypeIdentifier(TK_BOOLEAN);
  CompleteArrayType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalArrayType_equal)
{
  MinimalArrayType uut1;
  uut1.collection_flag = 1;
  uut1.header.common.bound_seq.length(1);
  uut1.header.common.bound_seq[0] = 38;
  uut1.element.common.element_flags = 1;
  uut1.element.common.type = TypeIdentifier(TK_BOOLEAN);
  MinimalArrayType uut2;
  uut2.collection_flag = 1;
  uut2.header.common.bound_seq.length(1);
  uut2.header.common.bound_seq[0] = 38;
  uut2.element.common.element_flags = 1;
  uut2.element.common.type = TypeIdentifier(TK_BOOLEAN);
  MinimalArrayType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteMapType_equal)
{
  CompleteMapType uut1;
  uut1.collection_flag = 1;
  uut1.header.common.bound = 38;
  uut1.key.common.element_flags = 1;
  uut1.key.common.type = TypeIdentifier(TK_BOOLEAN);
  uut1.element.common.element_flags = 1;
  uut1.element.common.type = TypeIdentifier(TK_BOOLEAN);
  CompleteMapType uut2;
  uut2.collection_flag = 1;
  uut2.header.common.bound = 38;
  uut2.key.common.element_flags = 1;
  uut2.key.common.type = TypeIdentifier(TK_BOOLEAN);
  uut2.element.common.element_flags = 1;
  uut2.element.common.type = TypeIdentifier(TK_BOOLEAN);
  CompleteMapType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalMapType_equal)
{
  MinimalMapType uut1;
  uut1.collection_flag = 1;
  uut1.header.common.bound = 38;
  uut1.key.common.element_flags = 1;
  uut1.key.common.type = TypeIdentifier(TK_BOOLEAN);
  uut1.element.common.element_flags = 1;
  uut1.element.common.type = TypeIdentifier(TK_BOOLEAN);
  MinimalMapType uut2;
  uut2.collection_flag = 1;
  uut2.header.common.bound = 38;
  uut2.key.common.element_flags = 1;
  uut2.key.common.type = TypeIdentifier(TK_BOOLEAN);
  uut2.element.common.element_flags = 1;
  uut2.element.common.type = TypeIdentifier(TK_BOOLEAN);
  MinimalMapType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonEnumeratedLiteral_equal)
{
  CommonEnumeratedLiteral uut1;
  uut1.value = 1;
  uut1.flags = 2;
  CommonEnumeratedLiteral uut2;
  uut2.value = 1;
  uut2.flags = 2;
  CommonEnumeratedLiteral uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteEnumeratedLiteral_equal)
{
  CompleteEnumeratedLiteral uut1;
  uut1.common.value = 1;
  uut1.common.flags = 2;
  uut1.detail.name = "name";
  CompleteEnumeratedLiteral uut2;
  uut2.common.value = 1;
  uut2.common.flags = 2;
  uut2.detail.name = "name";
  CompleteEnumeratedLiteral uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalEnumeratedLiteral_equal)
{
  MinimalEnumeratedLiteral uut1;
  uut1.common.value = 1;
  uut1.common.flags = 2;
  uut1.detail.name_hash[0] = 3;
  MinimalEnumeratedLiteral uut2;
  uut2.common.value = 1;
  uut2.common.flags = 2;
  uut2.detail.name_hash[0] = 3;
  MinimalEnumeratedLiteral uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonEnumeratedHeader_equal)
{
  CommonEnumeratedHeader uut1;
  uut1.bit_bound = 1;
  CommonEnumeratedHeader uut2;
  uut2.bit_bound = 1;
  CommonEnumeratedHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteEnumeratedHeader_equal)
{
  CompleteEnumeratedHeader uut1;
  uut1.common.bit_bound = 1;
  uut1.detail.type_name = "name";
  CompleteEnumeratedHeader uut2;
  uut2.common.bit_bound = 1;
  uut2.detail.type_name = "name";
  CompleteEnumeratedHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalEnumeratedHeader_equal)
{
  MinimalEnumeratedHeader uut1;
  uut1.common.bit_bound = 1;
  MinimalEnumeratedHeader uut2;
  uut2.common.bit_bound = 1;
  MinimalEnumeratedHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteEnumeratedType_equal)
{
  CompleteEnumeratedType uut1;
  uut1.enum_flags = 1;
  uut1.header.common.bit_bound = 1;
  uut1.header.detail.type_name = "name";
  CompleteEnumeratedType uut2;
  uut2.enum_flags = 1;
  uut2.header.common.bit_bound = 1;
  uut2.header.detail.type_name = "name";
  CompleteEnumeratedType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalEnumeratedType_equal)
{
  MinimalEnumeratedType uut1;
  uut1.enum_flags = 1;
  uut1.header.common.bit_bound = 1;
  MinimalEnumeratedType uut2;
  uut2.enum_flags = 1;
  uut2.header.common.bit_bound = 1;
  MinimalEnumeratedType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonBitflag_equal)
{
  CommonBitflag uut1;
  uut1.position = 1;
  uut1.flags = 2;
  CommonBitflag uut2;
  uut2.position = 1;
  uut2.flags = 2;
  CommonBitflag uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteBitflag_equal)
{
  CompleteBitflag uut1;
  uut1.common.position = 1;
  uut1.common.flags = 2;
  uut1.detail.name = "name";
  CompleteBitflag uut2;
  uut2.common.position = 1;
  uut2.common.flags = 2;
  uut2.detail.name = "name";
  CompleteBitflag uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalBitflag_equal)
{
  MinimalBitflag uut1;
  uut1.common.position = 1;
  uut1.common.flags = 2;
  uut1.detail.name_hash[0] = 1;
  MinimalBitflag uut2;
  uut2.common.position = 1;
  uut2.common.flags = 2;
  uut2.detail.name_hash[0] = 1;
  MinimalBitflag uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteBitmaskType_equal)
{
  CompleteBitmaskType uut1;
  uut1.bitmask_flags = 1;
  uut1.header.common.bit_bound = 1;
  uut1.header.detail.type_name = "name";
  CompleteBitmaskType uut2;
  uut2.bitmask_flags = 1;
  uut2.header.common.bit_bound = 1;
  uut2.header.detail.type_name = "name";
  CompleteBitmaskType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalBitmaskType_equal)
{
  MinimalBitmaskType uut1;
  uut1.bitmask_flags = 1;
  uut1.header.common.bit_bound = 1;
  MinimalBitmaskType uut2;
  uut2.bitmask_flags = 1;
  uut2.header.common.bit_bound = 1;
  MinimalBitmaskType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CommonBitfield_equal)
{
  CommonBitfield uut1;
  uut1.position = 1;
  uut1.flags = 2;
  uut1.bitcount = 3;
  uut1.holder_type = TK_UINT8;
  CommonBitfield uut2;
  uut2.position = 1;
  uut2.flags = 2;
  uut2.bitcount = 3;
  uut2.holder_type = TK_UINT8;
  CommonBitfield uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteBitfield_equal)
{
  CompleteBitfield uut1;
  uut1.common.position = 1;
  uut1.common.flags = 2;
  uut1.common.bitcount = 3;
  uut1.common.holder_type = TK_UINT8;
  uut1.detail.name = "name";
  CompleteBitfield uut2;
  uut2.common.position = 1;
  uut2.common.flags = 2;
  uut2.common.bitcount = 3;
  uut2.common.holder_type = TK_UINT8;
  uut2.detail.name = "name";
  CompleteBitfield uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalBitfield_equal)
{
  MinimalBitfield uut1;
  uut1.common.position = 1;
  uut1.common.flags = 2;
  uut1.common.bitcount = 3;
  uut1.common.holder_type = TK_UINT8;
  uut1.name_hash[0] = 4;
  MinimalBitfield uut2;
  uut2.common.position = 1;
  uut2.common.flags = 2;
  uut2.common.bitcount = 3;
  uut2.common.holder_type = TK_UINT8;
  uut2.name_hash[0] = 4;
  MinimalBitfield uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteBitsetHeader_equal)
{
  CompleteBitsetHeader uut1;
  uut1.detail.type_name = "name";
  CompleteBitsetHeader uut2;
  uut2.detail.type_name = "name";
  CompleteBitsetHeader uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalBitsetHeader_equal)
{
  MinimalBitsetHeader uut1;
  MinimalBitsetHeader uut2;

  EXPECT_EQ(uut1, uut2);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteBitsetType_equal)
{
  CompleteBitsetType uut1;
  uut1.bitset_flags = 1;
  uut1.header.detail.type_name = "name";
  CompleteBitsetType uut2;
  uut2.bitset_flags = 1;
  uut2.header.detail.type_name = "name";
  CompleteBitsetType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalBitsetType_equal)
{
  MinimalBitsetType uut1;
  uut1.bitset_flags = 1;
  MinimalBitsetType uut2;
  uut2.bitset_flags = 1;
  MinimalBitsetType uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut1, uut3);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteExtendedType_equal)
{
  CompleteExtendedType uut1;
  CompleteExtendedType uut2;

  EXPECT_EQ(uut1, uut2);
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalExtendedType_equal)
{
  MinimalExtendedType uut1;
  MinimalExtendedType uut2;

  EXPECT_EQ(uut1, uut2);
}

TEST(dds_DCPS_XTypes_TypeObject, CompleteTypeObject_equal)
{
  {
    CompleteTypeObject uut1;
    uut1.kind = TK_ALIAS;
    CompleteTypeObject uut2;
    uut2.kind = TK_ALIAS;
    CompleteTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    CompleteTypeObject uut1;
    uut1.kind = TK_ANNOTATION;
    CompleteTypeObject uut2;
    uut2.kind = TK_ANNOTATION;
    CompleteTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    CompleteTypeObject uut1;
    uut1.kind = TK_STRUCTURE;
    CompleteTypeObject uut2;
    uut2.kind = TK_STRUCTURE;
    CompleteTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    CompleteTypeObject uut1;
    uut1.kind = TK_UNION;
    CompleteTypeObject uut2;
    uut2.kind = TK_UNION;
    CompleteTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    CompleteTypeObject uut1;
    uut1.kind = TK_BITSET;
    CompleteTypeObject uut2;
    uut2.kind = TK_BITSET;
    CompleteTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    CompleteTypeObject uut1;
    uut1.kind = TK_SEQUENCE;
    CompleteTypeObject uut2;
    uut2.kind = TK_SEQUENCE;
    CompleteTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    CompleteTypeObject uut1;
    uut1.kind = TK_ARRAY;
    CompleteTypeObject uut2;
    uut2.kind = TK_ARRAY;
    CompleteTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    CompleteTypeObject uut1;
    uut1.kind = TK_MAP;
    CompleteTypeObject uut2;
    uut2.kind = TK_MAP;
    CompleteTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    CompleteTypeObject uut1;
    uut1.kind = TK_ENUM;
    CompleteTypeObject uut2;
    uut2.kind = TK_ENUM;
    CompleteTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    CompleteTypeObject uut1;
    uut1.kind = TK_BITMASK;
    CompleteTypeObject uut2;
    uut2.kind = TK_BITMASK;
    CompleteTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    CompleteTypeObject uut1;
    uut1.kind = TK_ENUM;
    CompleteTypeObject uut2;
    uut2.kind = TK_BITMASK;

    EXPECT_NE(uut1, uut2);
  }
}

TEST(dds_DCPS_XTypes_TypeObject, MinimalTypeObject_equal)
{
  {
    MinimalTypeObject uut1;
    uut1.kind = TK_ALIAS;
    MinimalTypeObject uut2;
    uut2.kind = TK_ALIAS;
    MinimalTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    MinimalTypeObject uut1;
    uut1.kind = TK_ANNOTATION;
    MinimalTypeObject uut2;
    uut2.kind = TK_ANNOTATION;
    MinimalTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    MinimalTypeObject uut1;
    uut1.kind = TK_STRUCTURE;
    MinimalTypeObject uut2;
    uut2.kind = TK_STRUCTURE;
    MinimalTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    MinimalTypeObject uut1;
    uut1.kind = TK_UNION;
    MinimalTypeObject uut2;
    uut2.kind = TK_UNION;
    MinimalTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    MinimalTypeObject uut1;
    uut1.kind = TK_BITSET;
    MinimalTypeObject uut2;
    uut2.kind = TK_BITSET;
    MinimalTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    MinimalTypeObject uut1;
    uut1.kind = TK_SEQUENCE;
    MinimalTypeObject uut2;
    uut2.kind = TK_SEQUENCE;
    MinimalTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    MinimalTypeObject uut1;
    uut1.kind = TK_ARRAY;
    MinimalTypeObject uut2;
    uut2.kind = TK_ARRAY;
    MinimalTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    MinimalTypeObject uut1;
    uut1.kind = TK_MAP;
    MinimalTypeObject uut2;
    uut2.kind = TK_MAP;
    MinimalTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    MinimalTypeObject uut1;
    uut1.kind = TK_ENUM;
    MinimalTypeObject uut2;
    uut2.kind = TK_ENUM;
    MinimalTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    MinimalTypeObject uut1;
    uut1.kind = TK_BITMASK;
    MinimalTypeObject uut2;
    uut2.kind = TK_BITMASK;
    MinimalTypeObject uut3;

    EXPECT_EQ(uut1, uut2);
    EXPECT_NE(uut1, uut3);
  }

  {
    MinimalTypeObject uut1;
    uut1.kind = TK_ENUM;
    MinimalTypeObject uut2;
    uut2.kind = TK_BITMASK;

    EXPECT_NE(uut1, uut2);
  }
}

TEST(dds_DCPS_XTypes_TypeObject, TypeObject_equal)
{
  TypeObject uut1;
  uut1.kind = EK_COMPLETE;
  TypeObject uut2;
  uut2.kind = EK_COMPLETE;
  TypeObject uut3;
  uut3.kind = EK_MINIMAL;
  TypeObject uut4;
  uut4.kind = EK_MINIMAL;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut2, uut3);
  EXPECT_EQ(uut3, uut4);
}

TEST(dds_DCPS_XTypes_TypeIdentifierTypeObjectPair, TypeIdentifierTypeObjectPair_equal)
{
  TypeIdentifierTypeObjectPair uut1;
  uut1.type_identifier = TypeIdentifier(TK_BOOLEAN);
  TypeIdentifierTypeObjectPair uut2;
  uut2.type_identifier = TypeIdentifier(TK_BOOLEAN);
  TypeIdentifierTypeObjectPair uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut2, uut3);
}

TEST(dds_DCPS_XTypes_TypeIdentifierPair, TypeIdentifierPair_equal)
{
  TypeIdentifierPair uut1;
  uut1.type_identifier1 = TypeIdentifier(TK_BOOLEAN);
  uut1.type_identifier2 = TypeIdentifier(TK_CHAR8);
  TypeIdentifierPair uut2;
  uut2.type_identifier1 = TypeIdentifier(TK_BOOLEAN);
  uut2.type_identifier2 = TypeIdentifier(TK_CHAR8);
  TypeIdentifierPair uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut2, uut3);
}

TEST(dds_DCPS_XTypes_TypeIdentifierWithSize, TypeIdentifierWithSize_equal)
{
  TypeIdentifierWithSize uut1;
  uut1.type_id = TypeIdentifier(TK_BOOLEAN);
  uut1.typeobject_serialized_size = 38;
  TypeIdentifierWithSize uut2;
  uut2.type_id = TypeIdentifier(TK_BOOLEAN);
  uut2.typeobject_serialized_size = 38;
  TypeIdentifierWithSize uut3;

  EXPECT_EQ(uut1, uut2);
  EXPECT_NE(uut2, uut3);
}
