/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/XTypes/TypeObject.h"

#include "../common/TestSupport.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using namespace OpenDDS::XTypes;
using namespace OpenDDS::DCPS;

const Encoding xcdr2_be(Encoding::KIND_XCDR2, ENDIAN_BIG);
const Encoding xcdr2_le(Encoding::KIND_XCDR2, ENDIAN_LITTLE);

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
  const size_t end_of_stru = strm.pos() + total_size;
  if (!(strm >> stru.member)) {
    return false;
  }
  return strm.skip(end_of_stru - strm.pos());
}

template<typename T, size_t N>
void check_encode(const T& object, const Encoding& enc, const unsigned char (&expected)[N])
{
  const size_t size = serialized_size(enc, object);
  TEST_CHECK(size == N);
  ACE_Message_Block buffer(size);
  {
    Serializer ser(&buffer, enc);
    TEST_CHECK(ser << object);
    TEST_CHECK(buffer.length() == N);
    TEST_CHECK(0 == std::memcmp(expected, buffer.rd_ptr(), N));
  }
  Serializer ser(&buffer, enc);
  T object2;
  TEST_CHECK(ser >> object2);
}

template<typename T, size_t N>
void check_decode(T& object, const Encoding& enc, const char (&expected)[N])
{
  ACE_Message_Block buffer(expected, N);
  buffer.wr_ptr(N);
  Serializer ser(&buffer, enc);
  TEST_CHECK(ser >> object);
  TEST_CHECK(buffer.length() == 0);
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

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  try {
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
    TEST_CHECK(seqs.length() == 1);
    TEST_CHECK(seqs[0].member == 2);

    const char invalid_encoding[] = {0, 0, 0, 0};
    check_decode(seqs, xcdr2_be, invalid_encoding);
    TEST_CHECK(seqs.length() == 0);


    const TypeObject testObject = getTypeObject();
    const TypeIdentifier goodIdentifier = makeTypeIdentifier(testObject);
    std::cout << goodIdentifier << std::endl;
    using namespace OpenDDS;
    const XTypes::TypeIdentifier good(XTypes::EK_MINIMAL, XTypes::EquivalenceHashWrapper(47, 43, 14, 5, 138, 206, 143, 33, 189, 131, 116, 89, 176, 60));
    TEST_CHECK(goodIdentifier == good);

    Encoding encoding(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
    encoding.skip_sequence_dheader(true);
    const TypeIdentifier badIdentifier = makeTypeIdentifier(testObject, &encoding);
    std::cout << badIdentifier << std::endl;
    const XTypes::TypeIdentifier bad(XTypes::EK_MINIMAL,XTypes::EquivalenceHashWrapper(100, 92, 123, 199, 182, 88, 54, 251, 172, 100, 66, 123, 137, 217));
    TEST_CHECK(badIdentifier == bad);

  } catch (const std::runtime_error& err) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %C\n"),
      err.what()), EXIT_FAILURE);
  }
  return EXIT_SUCCESS;
}
