#ifndef OPENDDS_SAFETY_PROFILE
#  include <XTypesUtilsTypeSupportImpl.h>
#  include <key_annotationTypeSupportImpl.h>
#  include <DynamicDataImplTypeSupportImpl.h>

#  include <tests/Utils/GtestRc.h>
#  include <tests/Utils/DataView.h>

#  include <dds/DCPS/XTypes/Utils.h>
#  include <dds/DCPS/XTypes/TypeLookupService.h>
#  include <dds/DCPS/XTypes/DynamicDataAdapter.h>
#  include <dds/DCPS/XTypes/DynamicDataFactory.h>
#  include <dds/DCPS/XTypes/DynamicDataImpl.h>

#  include <gtest/gtest.h>

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
  expected_names.push_back("discriminator");
  expected_names.push_back("discriminator");
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
  DDS::DynamicData_var dda = get_dynamic_data_adapter<SimpleKeyStruct>(dt, sample);

  std::vector<ACE_CDR::Long> expected_values;
  expected_values.push_back(10);
  std::vector<ACE_CDR::Long> actual_values;
  for (MemberPathVec::iterator it = keys.begin(); it != keys.end(); ++it) {
    DDS::DynamicData_var container;
    DDS::MemberId id;
    ASSERT_RC_OK(it->get_member_from_data(dda, container, id));
    ACE_CDR::Long value;
    ASSERT_RC_OK(container->get_int32_value(value, id));
    actual_values.push_back(value);
  }
  ASSERT_EQ(expected_values, actual_values);
}
#  endif

TEST_F(dds_DCPS_XTypes_Utils, less_than)
{
  using namespace DynamicDataImpl;
  add_type<LessThanStruct>();
  DDS::DynamicType_var dt = get_dynamic_type<LessThanStruct>();
  DDS::DynamicData_var a = DDS::DynamicDataFactory::get_instance()->create_data(dt);
  DDS::DynamicData_var b = DDS::DynamicDataFactory::get_instance()->create_data(dt);

  bool is_less_than = false;
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  DDS::MemberId id = 0;

  // byte
  ASSERT_RC_OK(b->set_byte_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_byte_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // bool
  ++id;
  ASSERT_RC_OK(b->set_boolean_value(id, true));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_boolean_value(id, true));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // uint_8
  ++id;
  ASSERT_RC_OK(b->set_uint8_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_uint8_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // uint_16
  ++id;
  ASSERT_RC_OK(b->set_uint16_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_uint16_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // uint_32
  ++id;
  ASSERT_RC_OK(b->set_uint32_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_uint32_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // uint_64
  ++id;
  ASSERT_RC_OK(b->set_uint64_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_uint64_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // int_8
  ++id;
  ASSERT_RC_OK(b->set_int8_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_int8_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // int_16
  ++id;
  ASSERT_RC_OK(b->set_int16_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_int16_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // int_32
  ++id;
  ASSERT_RC_OK(b->set_int32_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_int32_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // int_64
  ++id;
  ASSERT_RC_OK(b->set_int64_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_int64_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // float_32
  ++id;
  ASSERT_RC_OK(b->set_float32_value(id, 1.0f));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_float32_value(id, 1.0f));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // float_64
  ++id;
  ASSERT_RC_OK(b->set_float64_value(id, 1.0));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_float64_value(id, 1.0));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // float_128
  ++id;
  ACE_CDR::LongDouble ld1;
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(ld1, 1.0);
  ASSERT_RC_OK(b->set_float128_value(id, ld1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_float128_value(id, ld1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // char_8
  ++id;
  ASSERT_RC_OK(b->set_char8_value(id, '1'));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_char8_value(id, '1'));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // char_16
  ++id;
  ASSERT_RC_OK(b->set_char16_value(id, L'1'));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_char16_value(id, L'1'));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // str_8
  ++id;
  ASSERT_RC_OK(b->set_string_value(id, "1"));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_string_value(id, "1"));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // str_16
  ++id;
  ASSERT_RC_OK(b->set_wstring_value(id, L"1"));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_wstring_value(id, L"1"));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // enu
  ++id;
  ASSERT_RC_OK(b->set_int32_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a->set_int32_value(id, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // nested_struct.value
  ++id;
  DDS::DynamicData_var a_nested_struct;
  ASSERT_RC_OK(a->get_complex_value(a_nested_struct, id));
  DDS::DynamicData_var b_nested_struct;
  ASSERT_RC_OK(b->get_complex_value(b_nested_struct, id));
  ASSERT_RC_OK(b_nested_struct->set_int32_value(0, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a_nested_struct->set_int32_value(0, 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // nested_union
  ++id;
  DDS::DynamicData_var a_nested_union;
  ASSERT_RC_OK(a->get_complex_value(a_nested_union, id));
  DDS::DynamicData_var b_nested_union;
  ASSERT_RC_OK(b->get_complex_value(b_nested_union, id));
  ASSERT_RC_OK(b_nested_union->set_char8_value(1, 'x'));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a_nested_union->set_char8_value(1, 'x'));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // uint32_array
  ++id;
  DDS::DynamicData_var a_uint32_array;
  ASSERT_RC_OK(a->get_complex_value(a_uint32_array, id));
  DDS::DynamicData_var b_uint32_array;
  ASSERT_RC_OK(b->get_complex_value(b_uint32_array, id));
  ASSERT_RC_OK(b_uint32_array->set_uint32_value(b_uint32_array->get_member_id_at_index(0), 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  ASSERT_RC_OK(a_uint32_array->set_uint32_value(a_uint32_array->get_member_id_at_index(0), 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);

  // uint32_seq
  ++id;
  DDS::DynamicData_var a_uint32_seq;
  ASSERT_RC_OK(a->get_complex_value(a_uint32_seq, id));
  DDS::DynamicData_var b_uint32_seq;
  ASSERT_RC_OK(b->get_complex_value(b_uint32_seq, id));
  // a = {} b = {1}
  ASSERT_RC_OK(b_uint32_seq->set_uint32_value(b_uint32_seq->get_member_id_at_index(0), 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  // a = {0} b = {1}
  ASSERT_RC_OK(a_uint32_seq->set_uint32_value(a_uint32_seq->get_member_id_at_index(0), 0));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_TRUE(is_less_than);
  // a = {1} b = {1}
  ASSERT_RC_OK(a_uint32_seq->set_uint32_value(a_uint32_seq->get_member_id_at_index(0), 1));
  ASSERT_RC_OK(less_than(is_less_than, a, b, Filter_All));
  ASSERT_FALSE(is_less_than);
}

TEST_F(dds_DCPS_XTypes_Utils, MemberPathParser)
{

  {
    MemberPathParser mpp("");
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_TRUE(mpp.error);
  }

  // Invalid paths at start
  {
    MemberPathParser mpp(".");
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_TRUE(mpp.error);
  }

  {
    MemberPathParser mpp("[.");
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_TRUE(mpp.error);
  }

  {
    MemberPathParser mpp("[[");
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_TRUE(mpp.error);
  }

  {
    MemberPathParser mpp("[]");
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_TRUE(mpp.error);
  }

  // Invalid paths after member name
  {
    MemberPathParser mpp("m.");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("m", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_TRUE(mpp.error);
  }

  {
    MemberPathParser mpp("m[.");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("m", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_TRUE(mpp.error);
  }

  {
    MemberPathParser mpp("m[[");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("m", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_TRUE(mpp.error);
  }

  {
    MemberPathParser mpp("m[]");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("m", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_TRUE(mpp.error);
  }

  // Valid paths
  {
    MemberPathParser mpp("eins");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("eins", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_FALSE(mpp.error);
  }

  {
    MemberPathParser mpp("eins.zwei");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("eins", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("zwei", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_FALSE(mpp.error);
  }

  {
    MemberPathParser mpp("eins.zwei.drei");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("eins", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("zwei", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("drei", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_FALSE(mpp.error);
  }

  {
    MemberPathParser mpp("eins[1]");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("eins", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("1", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_FALSE(mpp.error);
  }

  {
    MemberPathParser mpp("eins[1].zwei[2][2]");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("eins", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("1", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("zwei", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("2", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("2", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_FALSE(mpp.error);
  }

  {
    MemberPathParser mpp("eins[1].zwei[2][2].drei[3][3][3]");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("eins", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("1", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("zwei", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("2", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("2", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("drei", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("3", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("3", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("3", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_FALSE(mpp.error);
  }

  {
    MemberPathParser mpp("[1]");
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("1", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_FALSE(mpp.error);
  }

  {
    MemberPathParser mpp("[1].nested[22].more_nested[-14]");
    CORBA::UInt32 index;
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("1", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_index(index));
    ASSERT_EQ(CORBA::UInt32(1), index);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("nested", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("22", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_index(index));
    ASSERT_EQ(CORBA::UInt32(22), index);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("more_nested", mpp.subpath.c_str());
    ASSERT_FALSE(mpp.in_subscript);
    ASSERT_TRUE(mpp.get_next_subpath());
    ASSERT_STREQ("-14", mpp.subpath.c_str());
    ASSERT_TRUE(mpp.in_subscript);
    ASSERT_FALSE(mpp.get_index(index)); // Could be a valid key to a map, but isn't a valid index.
    ASSERT_FALSE(mpp.get_next_subpath());
    ASSERT_FALSE(mpp.error);
  }
}

TEST_F(dds_DCPS_XTypes_Utils, MultidimArray)
{
  add_type<MultidimArrayStruct>();
  DDS::DynamicType_var dt = get_dynamic_type<MultidimArrayStruct>();
  OpenDDS::XTypes::DynamicDataImpl data(dt);
  DDS::DynamicData_var arr_dd;
  EXPECT_EQ(DDS::RETCODE_OK, data.get_complex_value(arr_dd, 0));
  DDS::DynamicType_var arr_type = arr_dd->type();
  DDS::TypeDescriptor_var arr_td;
  EXPECT_EQ(DDS::RETCODE_OK, arr_type->get_descriptor(arr_td));

  MultidimArrayStruct mdim_struct;
  mdim_struct.arr[0][0] = 10;
  mdim_struct.arr[0][1] = 20;
  mdim_struct.arr[0][2] = 30;
  mdim_struct.arr[1][0] = 40;
  mdim_struct.arr[1][1] = 50;
  mdim_struct.arr[1][2] = 60;
  DDS::BoundSeq idx_vec;
  idx_vec.length(2);
  for (CORBA::ULong i = 0; i < arr_td->bound()[0]; ++i) {
    idx_vec[0] = i;
    for (CORBA::ULong j = 0; j < arr_td->bound()[1]; ++j) {
      idx_vec[1] = j;
      CORBA::ULong flat_idx;
      EXPECT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::flat_index(flat_idx, idx_vec, arr_td->bound()));
      EXPECT_EQ(i * arr_td->bound()[1] + j, flat_idx);
      EXPECT_EQ(DDS::RETCODE_OK, arr_dd->set_int32_value(arr_dd->get_member_id_at_index(flat_idx),
                                                         mdim_struct.arr[i][j]));
    }
  }

  const unsigned char expected_cdr[] = {
    0x00,0x00,0x00,0x18,
    0x00,0x00,0x00,10, 0x00,0x00,0x00,20, 0x00,0x00,0x00,30,
    0x00,0x00,0x00,40, 0x00,0x00,0x00,50, 0x00,0x00,0x00,60
  };
  ACE_Message_Block buffer(64);
  OpenDDS::DCPS::Encoding xcdr2(OpenDDS::DCPS::Encoding::KIND_XCDR2, OpenDDS::DCPS::ENDIAN_BIG);
  OpenDDS::DCPS::Serializer ser(&buffer, xcdr2);
  ASSERT_TRUE(ser << &data);
  EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
}

#endif // OPENDDS_SAFETY_PROFILE
