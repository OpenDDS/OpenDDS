#include "union_defaultsTypeSupportImpl.h"

#include <dds/DCPS/JsonValueReader.h>
#include <dds/DCPS/JsonValueWriter.h>

#include <gtest/gtest.h>

namespace {
  const OpenDDS::DCPS::Encoding xcdr2(OpenDDS::DCPS::Encoding::KIND_XCDR2, OpenDDS::DCPS::ENDIAN_BIG);

  void set_sequence(Unions::KU& ku)
  {
    ku.dsequence(Unions::dummy_sequence(2));
  }

  bool val_is_empty(const Unions::KU& ku)
  {
    return ku.val().empty();
  }

  Unions::KU& ks_u(Unions::KS& ks)
  {
    return ks.u();
  }
}

TEST(UnionDefault, no_default)
{
  Unions::Z z;
  OpenDDS::DCPS::set_default(z);
  EXPECT_TRUE(z._d() == 0);
}

TEST(UnionDefault, string)
{
  Unions::Y y;
  OpenDDS::DCPS::set_default(y);
  EXPECT_TRUE(y._d() == 0);
}

TEST(UnionDefault, Z)
{
  Unions::X x;
  OpenDDS::DCPS::set_default(x);
  EXPECT_TRUE(x._d() == 0);
}

TEST(UnionDefault, dummy)
{
  Unions::V v;
  OpenDDS::DCPS::set_default(v);
  EXPECT_TRUE(v._d() == 0);
}

TEST(UnionDefault, dummy_sequence)
{
  Unions::U u;
  OpenDDS::DCPS::set_default(u);
  EXPECT_TRUE(u._d() == 0);
}

TEST(UnionDefault, dummy_array)
{
  Unions::T t;
  OpenDDS::DCPS::set_default(t);
  EXPECT_TRUE(t._d() == 0);
}

TEST(UnionDefault, default_branch_from_other_member)
{
  Unions::W w;
  w.val("abc");
  OpenDDS::DCPS::set_default(w);
  EXPECT_TRUE(w._d() == 0);
  EXPECT_EQ(0u, w.dsequence().size());
}

TEST(UnionDefault, implicit_default_from_other_member)
{
  Unions::N n;
  n.val("abc");
  OpenDDS::DCPS::set_default(n);
  EXPECT_TRUE(n._d() == 0);
}

template <typename Union, typename Disc>
void check_implicit_default_read(Disc disc)
{
  using namespace OpenDDS::DCPS;
  Union sent;
  sent._default();
  sent._d(disc);
  Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));
  {
    Serializer serializer(data.get(), xcdr2);
    ASSERT_TRUE(serializer << sent);
  }

  Union received;
  received.val("abc");
  {
    Serializer serializer(data.get(), xcdr2);
    ASSERT_TRUE(serializer >> received);
  }
  EXPECT_TRUE(received._d() == disc);
}

TEST(UnionDefault, deserialize_implicit_default_into_other_member)
{
  check_implicit_default_read<Unions::N>(5);
}

TEST(UnionDefault, deserialize_boolean_implicit_default_into_other_member)
{
  check_implicit_default_read<Unions::BN>(false);
}

TEST(UnionDefault, deserialize_mutable_implicit_default_into_other_member)
{
  check_implicit_default_read<Unions::MN>(5);
}

TEST(UnionDefault, deserialize_mutable_boolean_implicit_default_into_other_member)
{
  check_implicit_default_read<Unions::MBN>(false);
}

TEST(UnionDefault, key_only_read_into_other_member)
{
  using namespace OpenDDS::DCPS;
  Unions::KU sent;
  sent.val("abc");
  const KeyOnly<const Unions::KU> sent_key(sent);
  Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent_key)));
  {
    Serializer serializer(data.get(), xcdr2);
    ASSERT_TRUE(serializer << sent_key);
  }

  Unions::KU received;
  set_sequence(received);
  const KeyOnly<Unions::KU> received_key(received);
  {
    Serializer serializer(data.get(), xcdr2);
    ASSERT_TRUE(serializer >> received_key);
  }
  EXPECT_TRUE(received._d() == 1);
  EXPECT_TRUE(val_is_empty(received));
}

TEST(UnionDefault, nested_key_only_read_into_other_member)
{
  using namespace OpenDDS::DCPS;
  Unions::KS sent;
  ks_u(sent).val("abc");
  const KeyOnly<const Unions::KS> sent_key(sent);
  Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent_key)));
  {
    Serializer serializer(data.get(), xcdr2);
    ASSERT_TRUE(serializer << sent_key);
  }

  Unions::KS received;
  set_sequence(ks_u(received));
  const KeyOnly<Unions::KS> received_key(received);
  {
    Serializer serializer(data.get(), xcdr2);
    ASSERT_TRUE(serializer >> received_key);
  }
  EXPECT_TRUE(ks_u(received)._d() == 1);
  EXPECT_TRUE(val_is_empty(ks_u(received)));
}

#if OPENDDS_HAS_JSON_VALUE_READER && OPENDDS_HAS_JSON_VALUE_WRITER
template <typename T>
std::string to_key_only_json(const T& sample)
{
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  OpenDDS::DCPS::JsonValueWriter<rapidjson::Writer<rapidjson::StringBuffer> > jvw(writer);
  const OpenDDS::DCPS::KeyOnly<const T> key(sample);
  EXPECT_TRUE(vwrite(jvw, key));
  return buffer.GetString();
}

template <typename T>
bool from_key_only_json(const std::string& json, T& sample)
{
  rapidjson::StringStream stream(json.c_str());
  OpenDDS::DCPS::JsonValueReader<> jvr(stream);
  const OpenDDS::DCPS::KeyOnly<T> key(sample);
  return vread(jvr, key);
}

TEST(UnionDefault, key_only_vread_into_other_member)
{
  Unions::KU sent;
  sent.val("abc");
  Unions::KU received;
  set_sequence(received);
  ASSERT_TRUE(from_key_only_json(to_key_only_json(sent), received));
  EXPECT_TRUE(received._d() == 1);
  EXPECT_TRUE(val_is_empty(received));
}

TEST(UnionDefault, nested_key_only_vread_into_other_member)
{
  Unions::KS sent;
  ks_u(sent).val("abc");
  Unions::KS received;
  set_sequence(ks_u(received));
  ASSERT_TRUE(from_key_only_json(to_key_only_json(sent), received));
  EXPECT_TRUE(ks_u(received)._d() == 1);
  EXPECT_TRUE(val_is_empty(ks_u(received)));
}
#endif

#ifndef OPENDDS_SAFETY_PROFILE
TEST(UnionDefault, wstring)
{
  Unions::S s;
  OpenDDS::DCPS::set_default(s);
  EXPECT_TRUE(s._d() == 0);
}

TEST(UnionDefault, wchar)
{
  Unions::R r;
  OpenDDS::DCPS::set_default(r);
  EXPECT_TRUE(r._d() == 0);
}
#endif

TEST(UnionDefault, long_double)
{
  Unions::Q q;
  OpenDDS::DCPS::set_default(q);
  EXPECT_TRUE(q._d() == 0);
}

TEST(UnionDefault, boolean)
{
  Unions::P p;
  OpenDDS::DCPS::set_default(p);
  EXPECT_TRUE(p._d() == 0);
}

TEST(UnionDefault, enum)
{
  Unions::O o;
  OpenDDS::DCPS::set_default(o);
  EXPECT_TRUE(o._d() == Unions::Dog::Mastiff);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
