/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include <dds/DCPS/ResizeSeqNoInit.h>
#include <dds/DCPS/SafetyProfileSequence.h>
#include <dds/DdsDcpsCoreC.h>
#include <FACE/Sequence.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_tao_sequence_grow)
{
  DDS::OctetSeq seq;
  seq.length(3);
  seq[0] = 0xAA;
  seq[1] = 0xBB;
  seq[2] = 0xCC;

  resize_unbounded_seq_no_init(seq, 5);

  EXPECT_EQ(seq.length(), 5u);
  EXPECT_EQ(seq[0], 0xAA);
  EXPECT_EQ(seq[1], 0xBB);
  EXPECT_EQ(seq[2], 0xCC);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_tao_sequence_shrink)
{
  DDS::OctetSeq seq;
  seq.length(5);
  seq[0] = 0x11;
  seq[1] = 0x22;

  resize_unbounded_seq_no_init(seq, 2);

  EXPECT_EQ(seq.length(), 2u);
  EXPECT_EQ(seq[0], 0x11);
  EXPECT_EQ(seq[1], 0x22);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_tao_sequence_same_size)
{
  DDS::OctetSeq seq;
  seq.length(3);
  seq[0] = 0xDE;
  seq[1] = 0xAD;
  seq[2] = 0xBE;

  resize_unbounded_seq_no_init(seq, 3);

  EXPECT_EQ(seq.length(), 3u);
  EXPECT_EQ(seq[0], 0xDE);
  EXPECT_EQ(seq[1], 0xAD);
  EXPECT_EQ(seq[2], 0xBE);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_tao_sequence_from_empty)
{
  DDS::OctetSeq seq;
  EXPECT_EQ(seq.length(), 0u);

  resize_unbounded_seq_no_init(seq, 10);

  EXPECT_EQ(seq.length(), 10u);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_tao_sequence_to_empty)
{
  DDS::OctetSeq seq;
  seq.length(5);
  seq[0] = 0xFF;

  resize_unbounded_seq_no_init(seq, 0);

  EXPECT_EQ(seq.length(), 0u);
}

namespace Face {
  typedef OpenDDS::FaceTypes::Unbounded Unbounded;
  typedef OpenDDS::FaceTypes::Bounded<10> Bounded10;

  class OctetSeq : public OpenDDS::FaceTypes::Sequence<CORBA::Octet, Unbounded> {
    typedef OpenDDS::FaceTypes::Sequence<CORBA::Octet, Unbounded> Base;
  public:
    OctetSeq() : Base() {}
    OctetSeq(const OctetSeq& other) : Base(other) {}
  };

  class BoundedOctetSeq : public OpenDDS::FaceTypes::Sequence<CORBA::Octet, Bounded10> {
    typedef OpenDDS::FaceTypes::Sequence<CORBA::Octet, Bounded10> Base;
  public:
    BoundedOctetSeq() : Base() {}
    BoundedOctetSeq(const BoundedOctetSeq& other) : Base(other) {}
  };
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_face_sequence_grow)
{
  Face::OctetSeq seq;
  seq.length(3);
  seq[0] = 0xAA;
  seq[1] = 0xBB;
  seq[2] = 0xCC;

  resize_unbounded_seq_no_init(seq, 5);

  EXPECT_EQ(seq.length(), 5u);
  EXPECT_EQ(seq[0], 0xAA);
  EXPECT_EQ(seq[1], 0xBB);
  EXPECT_EQ(seq[2], 0xCC);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_face_sequence_shrink)
{
  Face::OctetSeq seq;
  seq.length(5);
  seq[0] = 0x11;
  seq[1] = 0x22;

  resize_unbounded_seq_no_init(seq, 2);

  EXPECT_EQ(seq.length(), 2u);
  EXPECT_EQ(seq[0], 0x11);
  EXPECT_EQ(seq[1], 0x22);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_bounded_seq_no_init_face_sequence_grow)
{
  Face::BoundedOctetSeq seq;
  seq.length(3);
  seq[0] = 0xAA;
  seq[1] = 0xBB;
  seq[2] = 0xCC;

  resize_bounded_seq_no_init(seq, 5);

  EXPECT_EQ(seq.length(), 5u);
  EXPECT_EQ(seq[0], 0xAA);
  EXPECT_EQ(seq[1], 0xBB);
  EXPECT_EQ(seq[2], 0xCC);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_bounded_seq_no_init_face_sequence_shrink)
{
  Face::BoundedOctetSeq seq;
  seq.length(5);
  seq[0] = 0x11;
  seq[1] = 0x22;

  resize_bounded_seq_no_init(seq, 2);

  EXPECT_EQ(seq.length(), 2u);
  EXPECT_EQ(seq[0], 0x11);
  EXPECT_EQ(seq[1], 0x22);
}

namespace Sp {
  typedef OpenDDS::SafetyProfile::Unbounded Unbounded;
  typedef OpenDDS::SafetyProfile::Bounded<10> Bounded10;

  class OctetSeq : public OpenDDS::SafetyProfile::Sequence<CORBA::Octet, Unbounded> {
    typedef OpenDDS::SafetyProfile::Sequence<CORBA::Octet, Unbounded> Base;
  public:
    OctetSeq() : Base() {}
    OctetSeq(const OctetSeq& other) : Base(other) {}
  };

  class BoundedOctetSeq : public OpenDDS::SafetyProfile::Sequence<CORBA::Octet, Bounded10> {
    typedef OpenDDS::SafetyProfile::Sequence<CORBA::Octet, Bounded10> Base;
  public:
    BoundedOctetSeq() : Base() {}
    BoundedOctetSeq(const BoundedOctetSeq& other) : Base(other) {}
  };
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_sp_sequence_grow)
{
  Sp::OctetSeq seq;
  seq.length(3);
  seq[0] = 0xAA;
  seq[1] = 0xBB;
  seq[2] = 0xCC;

  resize_unbounded_seq_no_init(seq, 5);

  EXPECT_EQ(seq.length(), 5u);
  EXPECT_EQ(seq[0], 0xAA);
  EXPECT_EQ(seq[1], 0xBB);
  EXPECT_EQ(seq[2], 0xCC);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_sp_sequence_shrink)
{
  Sp::OctetSeq seq;
  seq.length(5);
  seq[0] = 0x11;
  seq[1] = 0x22;

  resize_unbounded_seq_no_init(seq, 2);

  EXPECT_EQ(seq.length(), 2u);
  EXPECT_EQ(seq[0], 0x11);
  EXPECT_EQ(seq[1], 0x22);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_bounded_seq_no_init_sp_sequence_grow)
{
  Sp::BoundedOctetSeq seq;
  seq.length(3);
  seq[0] = 0xAA;
  seq[1] = 0xBB;
  seq[2] = 0xCC;

  resize_bounded_seq_no_init(seq, 5);

  EXPECT_EQ(seq.length(), 5u);
  EXPECT_EQ(seq[0], 0xAA);
  EXPECT_EQ(seq[1], 0xBB);
  EXPECT_EQ(seq[2], 0xCC);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_bounded_seq_no_init_sp_sequence_shrink)
{
  Sp::BoundedOctetSeq seq;
  seq.length(5);
  seq[0] = 0x11;
  seq[1] = 0x22;

  resize_bounded_seq_no_init(seq, 2);

  EXPECT_EQ(seq.length(), 2u);
  EXPECT_EQ(seq[0], 0x11);
  EXPECT_EQ(seq[1], 0x22);
}

#ifdef ACE_HAS_CPP11
TEST(dds_DCPS_ResizeSeqNoInit, OptionalInitVector_default_construct)
{
  OptionalInitVector<int> vec;
  EXPECT_EQ(vec.size(), 0u);
}

TEST(dds_DCPS_ResizeSeqNoInit, OptionalInitVector_resize_value_initializes)
{
  OptionalInitVector<int> vec;
  vec.resize(5);
  // Default resize should value-initialize (zeros for int)
  for (size_t i = 0; i < vec.size(); ++i) {
    EXPECT_EQ(vec[i], 0);
  }
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_vector_grow)
{
  OptionalInitVector<int> vec;
  vec.resize(3);
  vec[0] = 10;
  vec[1] = 20;
  vec[2] = 30;

  resize_unbounded_seq_no_init(vec, 5);

  EXPECT_EQ(vec.size(), 5u);
  EXPECT_EQ(vec[0], 10);
  EXPECT_EQ(vec[1], 20);
  EXPECT_EQ(vec[2], 30);
}

TEST(dds_DCPS_ResizeSeqNoInit, resize_unbounded_seq_no_init_vector_shrink)
{
  OptionalInitVector<int> vec;
  vec.resize(5);
  vec[0] = 10;
  vec[1] = 20;

  resize_unbounded_seq_no_init(vec, 2);

  EXPECT_EQ(vec.size(), 2u);
  EXPECT_EQ(vec[0], 10);
  EXPECT_EQ(vec[1], 20);
}
#endif // ACE_HAS_CPP11
