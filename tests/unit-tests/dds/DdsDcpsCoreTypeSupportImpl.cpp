/*
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DdsDcpsCoreTypeSupportImpl.h>

#include <dds/DCPS/Serializer.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;


TEST(dds_DdsDcpsCoreTypeSupportImpl, InvalidPropertySeq)
{
  static const Encoding enc(Encoding::KIND_XCDR1);
  const size_t size = int32_cdr_size;
  ACE_Message_Block mb(size);
  {
    Serializer ser_out(&mb, enc);
    ASSERT_TRUE(ser_out << 99);
  }
  {
    Serializer ser_in(&mb, enc);
    DDS::PropertySeq seq;
    ASSERT_FALSE(ser_in >> seq);
  }
}
