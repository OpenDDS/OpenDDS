#ifndef OPENDDS_SAFETY_PROFILE
#  include <XTypesUtilsTypeSupportImpl.h>
#  include <key_annotationTypeSupportImpl.h>

#  include <dds/DCPS/XTypes/Utils.h>
#  include <dds/DCPS/XTypes/TypeLookupService.h>
#  include <dds/DCPS/XTypes/DynamicDataAdapter.h>
#  include <dds/DCPS/DCPS_Utils.h>

#  include <gtest/gtest.h>

::testing::AssertionResult retcodes_equal(
  const char* a_expr, const char* b_expr,
  DDS::ReturnCode_t a, DDS::ReturnCode_t b)
{
  if (a == b) {
    return ::testing::AssertionSuccess();
  }
  return ::testing::AssertionFailure() <<
    "Expected equality of these values:\n"
    "  " << a_expr << "\n"
    "    Which is: " << OpenDDS::DCPS::retcode_to_string(a) << "\n"
    "  " << b_expr << "\n"
    "    Which is: " << OpenDDS::DCPS::retcode_to_string(b) << "\n";
}

#define EXPECT_RC_EQ(A, B) EXPECT_PRED_FORMAT2(retcodes_equal, (A), (B))
#define ASSERT_RC_EQ(A, B) ASSERT_PRED_FORMAT2(retcodes_equal, (A), (B))
#define EXPECT_RC_OK(VALUE) EXPECT_RC_EQ(::DDS::RETCODE_OK, (VALUE))
#define ASSERT_RC_OK(VALUE) ASSERT_RC_EQ(::DDS::RETCODE_OK, (VALUE))

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;
using namespace XTypesUtils;
using namespace key_annotation;

class dds_DCPS_XTypes_Utils : public testing::Test {
public:
  TypeLookupService_rch tls_;

  template<typename TopicType>
  void add_type()
  {
    TypeIdentifierPairSeq tid_pairs;
    TypeIdentifierPair tid_pair;
    typedef typename DDSTraits<TopicType>::XtagType Xtag;
    tid_pair.type_identifier1 = getCompleteTypeIdentifier<Xtag>();
    tid_pair.type_identifier2 = getMinimalTypeIdentifier<Xtag>();
    tid_pairs.append(tid_pair);
    tls_->update_type_identifier_map(tid_pairs);

    typename DDSTraits<TopicType>::TypeSupportImplType tsi;
    tsi.add_types(tls_);
  }

  void SetUp()
  {
    tls_ = make_rch<TypeLookupService>();
  }

  template<typename TopicType>
  DDS::DynamicType_var get_dynamic_type()
  {
    typedef typename DDSTraits<TopicType>::XtagType Xtag;
    const TypeIdentifier& com_ti = getCompleteTypeIdentifier<Xtag>();
    const TypeMap& com_map = getCompleteTypeMap<Xtag>();
    TypeMap::const_iterator pos = com_map.find(com_ti);
    EXPECT_TRUE(pos != com_map.end());
    const TypeObject& com_to = pos->second;
    return tls_->complete_to_dynamic(com_to.complete, GUID_UNKNOWN);
  }

  template<typename TopicType>
  void expect_ext(Extensibility expected)
  {
    add_type<TopicType>();
    DDS::DynamicType_var dt = get_dynamic_type<TopicType>();
    CORBA::String_var name = dt->get_name();
    Extensibility actual;
    EXPECT_RC_OK(extensibility(dt.in(), actual)) << "Happend in type " << name.in();
    EXPECT_EQ(expected, actual) << "Happend in type " << name.in();
  }

  template<typename TopicType>
  void expect_maxext(Extensibility expected)
  {
    add_type<TopicType>();
    DDS::DynamicType_var dt = get_dynamic_type<TopicType>();
    CORBA::String_var name = dt->get_name();
    Extensibility actual;
    EXPECT_RC_OK(max_extensibility(dt.in(), actual))
      << "Happend in type " << name.in();
    EXPECT_EQ(expected, actual) << "Happend in type " << name.in();
  }
};

TEST_F(dds_DCPS_XTypes_Utils, extensibility)
{
  expect_ext<FinalStruct>(FINAL);
  expect_ext<AppendableUnion>(APPENDABLE);
  expect_ext<MutableStruct>(MUTABLE);
  expect_ext<FinalMaxFinalStruct>(FINAL);
  expect_ext<FinalMaxAppendableStruct>(FINAL);
  expect_ext<FinalMaxMutableStruct>(FINAL);
  expect_ext<AppendableMaxAppendableStruct>(APPENDABLE);
  expect_ext<AppendableMaxMutableUnion>(APPENDABLE);
  expect_ext<MutableMaxMutableStruct>(MUTABLE);
}

TEST_F(dds_DCPS_XTypes_Utils, max_extensibility)
{
  expect_maxext<FinalStruct>(FINAL);
  expect_maxext<AppendableUnion>(APPENDABLE);
  expect_maxext<MutableStruct>(MUTABLE);
  expect_maxext<FinalMaxFinalStruct>(FINAL);
  expect_maxext<FinalMaxAppendableStruct>(APPENDABLE);
  expect_maxext<FinalMaxMutableStruct>(MUTABLE);
  expect_maxext<AppendableMaxAppendableStruct>(APPENDABLE);
  expect_maxext<AppendableMaxMutableUnion>(MUTABLE);
  expect_maxext<MutableMaxMutableStruct>(MUTABLE);
}

namespace {
  struct GetKeysCheck{
    dds_DCPS_XTypes_Utils& t;
    MemberPathVec expected;

    GetKeysCheck(dds_DCPS_XTypes_Utils& t)
    : t(t)
    {
    }

    MemberPath& new_key()
    {
      expected.push_back(MemberPath());
      return expected.back();
    }

    template <typename TopicType>
    void check()
    {
      t.add_type<TopicType>();
      DDS::DynamicType_var dt = t.get_dynamic_type<TopicType>();
      CORBA::String_var name = dt->get_name();
      MemberPathVec actual;
      EXPECT_RC_OK(get_keys(dt, actual))
        << "Happend in type " << name.in();
      const size_t count = std::min(expected.size(), actual.size());
      for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(expected[i].ids, actual[i].ids)
          << "both with i == " << i << " for type "  << name.in();
      }
      EXPECT_EQ(expected.size(), actual.size()) << "Happend in type " << name.in();
    }
  };
}

TEST_F(dds_DCPS_XTypes_Utils, get_keys)
{
  GetKeysCheck(*this).check<UnkeyedStruct>();

  {
    GetKeysCheck c(*this);
    c.new_key().id(10);
    c.check<SimpleKeyStruct>();
  }

  {
    GetKeysCheck c(*this);
    c.new_key().id(20);
    c.new_key().id(21).id(10);
    c.check<NestedKeyStruct>();
  }

  {
    GetKeysCheck c(*this);
    c.new_key().id(30);
    // TODO: Using typedef seems to result in a broken DynamicType that doesn't
    // have any members, so this key doesn't appear in the results.
    /*
    c.new_key().id(31).id(10);
    */
    c.check<TypedefStructKeyStruct>();
  }

  // NOTE: Array counts are different from the ones in the key_annotation test
  // because we're only counting arrays as 1 key field.
  {
    GetKeysCheck c(*this);
    c.new_key().id(40);
    c.check<LongArrayStruct>();
  }

  {
    GetKeysCheck c(*this);
    c.new_key().id(50);
    c.check<SimpleKeyArray>();
  }

  GetKeysCheck(*this).check<UnkeyedUnion>();

  {
    GetKeysCheck c(*this);
    c.new_key().id(DISCRIMINATOR_ID);
    c.check<KeyedUnion>();
  }

  {
    GetKeysCheck c(*this);
    c.new_key().id(60).id(DISCRIMINATOR_ID);
    c.new_key().id(62).id(DISCRIMINATOR_ID);
    c.new_key().id(63);
    c.check<KeyedUnionStruct>();
  }

  {
    GetKeysCheck c(*this);
    c.new_key().id(70);
    c.new_key().id(71);
    c.check<MultidimensionalArrayStruct>();
  }

  {
    GetKeysCheck c(*this);
    c.new_key().id(100).id(80);
    c.new_key().id(100).id(81);
    c.new_key().id(100).id(82);
    c.new_key().id(101).id(90);
    c.new_key().id(102);
    c.check<ImpliedKeys::StructA>();
  }

  {
    GetKeysCheck c(*this);
    c.new_key().id(110).id(100).id(80);
    c.new_key().id(110).id(100).id(81);
    c.new_key().id(110).id(100).id(82);
    c.new_key().id(110).id(101).id(90);
    c.new_key().id(110).id(102);
    c.new_key().id(111);
    c.check<ImpliedKeys::StructB>();
  }

  /* TODO: Need to support @key(FALSE) to do this
  {
    GetKeysCheck c(*this);
    c.new_key().id(130).id(120);
    c.new_key().id(130).id(121);
    c.check<ImpliedKeys::StructC>();
  }
  */
}

TEST_F(dds_DCPS_XTypes_Utils, member_path_get_member_from_type)
{
  add_type<KeyedUnionStruct>();
  DDS::DynamicType_var dt = get_dynamic_type<KeyedUnionStruct>();
  MemberPathVec keys;
  EXPECT_RC_OK(get_keys(dt, keys));
  std::vector<std::string> expected_names;
  expected_names.push_back("_d");
  expected_names.push_back("_d");
  expected_names.push_back("another_key");
  std::vector<std::string> actual_names;
  for (MemberPathVec::iterator it = keys.begin(); it != keys.end(); ++it) {
    DDS::DynamicTypeMember_var member;
    EXPECT_RC_OK(it->get_member_from_type(dt, member));
    CORBA::String_var name = member->get_name();
    actual_names.push_back(name.in());
  }
  EXPECT_EQ(expected_names, actual_names);
}

#  if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
TEST_F(dds_DCPS_XTypes_Utils, member_path_get_member_from_data)
{
  // TODO: More complex test when DynamicDataAdapter can handle it
  add_type<SimpleKeyStruct>();
  DDS::DynamicType_var dt = get_dynamic_type<SimpleKeyStruct>();
  MemberPathVec keys;
  ASSERT_RC_OK(get_keys(dt, keys));

  SimpleKeyStruct sample;
  sample.key = 10;
  sample.value = 20;
  DynamicDataAdapter<SimpleKeyStruct> dda(dt, getMetaStruct<SimpleKeyStruct>(), sample);

  std::vector<ACE_CDR::Long> expected_values;
  expected_values.push_back(10);
  std::vector<ACE_CDR::Long> actual_values;
  for (MemberPathVec::iterator it = keys.begin(); it != keys.end(); ++it) {
    DDS::DynamicData_var container;
    DDS::MemberId id;
    ASSERT_RC_OK(it->get_member_from_data(&dda, container, id));
    ACE_CDR::Long value;
    ASSERT_RC_OK(container->get_int32_value(value, id));
    actual_values.push_back(value);
  }
  ASSERT_EQ(expected_values, actual_values);
}
#  endif

#endif // OPENDDS_SAFETY_PROFILE
