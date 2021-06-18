/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/XTypes/TypeObject.h"

#include "../common/TestSupport.h"

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
  size_t size = 0;
  serialized_size(enc, size, object);
  TEST_CHECK(size == N);
  ACE_Message_Block buffer(size);
  {
    Serializer ser(&buffer, enc);
    TEST_CHECK(ser << object);
    TEST_CHECK(buffer.length() == N);
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

  } catch (const std::runtime_error& err) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %C\n"),
      err.what()), -1);
  }
  return 0;
}
