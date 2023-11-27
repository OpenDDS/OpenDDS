#ifndef OPENDDS_SAFETY_PROFILE
#  include <DynamicDataAdapterTypeSupportImpl.h>

#  include <tests/Utils/GtestRc.h>

#  include <dds/DCPS/XTypes/Utils.h>
#  include <dds/DCPS/XTypes/TypeLookupService.h>
#  include <dds/DCPS/XTypes/DynamicDataAdapter.h>
#  include <dds/DCPS/XTypes/DynamicDataFactory.h>

#  include <gtest/gtest.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;
using namespace DynamicDataAdapterIdl;

class dds_DCPS_XTypes_DynamicDataAdapter : public testing::Test {
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
};

#  if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
TEST_F(dds_DCPS_XTypes_DynamicDataAdapter, simple_struct)
{
  add_type<SimpleStruct>();
  DDS::DynamicType_var dt = get_dynamic_type<SimpleStruct>();
  SimpleStruct ss;
  ss.value = 1;
  DDS::DynamicData_var dd = get_dynamic_data_adapter<SimpleStruct, SimpleStruct>(dt, ss);
  DDS::UInt32 value;
  ASSERT_RC_OK(dd->get_uint32_value(value, 0));
  ASSERT_EQ(1u, value);
  ASSERT_RC_OK(dd->set_uint32_value(0, value + 1));
  ASSERT_EQ(2u, ss.value);
}

template <typename T>
void set_collection_struct_seq_len(T& col_struct)
{
  col_struct.byte.length(2);
  col_struct.boole.length(2);
  col_struct.i8.length(2);
  col_struct.u8.length(2);
  col_struct.i16.length(2);
  col_struct.u16.length(2);
  col_struct.i32.length(2);
  col_struct.u32.length(2);
  col_struct.i64.length(2);
  col_struct.u64.length(2);
  col_struct.f32.length(2);
  col_struct.f64.length(2);
  col_struct.f128.length(2);
  col_struct.c8.length(2);
  col_struct.c16.length(2);
  col_struct.s8.length(2);
  col_struct.s16.length(2);
  col_struct.enum_type.length(2);
  col_struct.simple_struct.length(2);
  col_struct.simple_union.length(2);
}

template <typename T>
void test_collection_struct_seq_len(T& col_struct)
{
  ASSERT_EQ(col_struct.byte.length(), 2u);
  ASSERT_EQ(col_struct.boole.length(), 2u);
  ASSERT_EQ(col_struct.i8.length(), 2u);
  ASSERT_EQ(col_struct.u8.length(), 2u);
  ASSERT_EQ(col_struct.i16.length(), 2u);
  ASSERT_EQ(col_struct.u16.length(), 2u);
  ASSERT_EQ(col_struct.i32.length(), 2u);
  ASSERT_EQ(col_struct.u32.length(), 2u);
  ASSERT_EQ(col_struct.i64.length(), 2u);
  ASSERT_EQ(col_struct.u64.length(), 2u);
  ASSERT_EQ(col_struct.f32.length(), 2u);
  ASSERT_EQ(col_struct.f64.length(), 2u);
  ASSERT_EQ(col_struct.f128.length(), 2u);
  ASSERT_EQ(col_struct.c8.length(), 2u);
  ASSERT_EQ(col_struct.c16.length(), 2u);
  ASSERT_EQ(col_struct.s8.length(), 2u);
  ASSERT_EQ(col_struct.s16.length(), 2u);
  ASSERT_EQ(col_struct.enum_type.length(), 2u);
  ASSERT_EQ(col_struct.simple_struct.length(), 2u);
  ASSERT_EQ(col_struct.simple_union.length(), 2u);
}

template <typename T>
void set_collection_struct(T& col_struct)
{
  col_struct.byte[0] = 0x01;
  col_struct.byte[1] = 0x02;
  col_struct.boole[0] = false;
  col_struct.boole[1] = true;
  col_struct.i8[0] = 100;
  col_struct.i8[1] = 111;
  col_struct.u8[0] = 200;
  col_struct.u8[1] = 222;
  col_struct.i16[0] = 1000;
  col_struct.i16[1] = 1111;
  col_struct.u16[0] = 2000;
  col_struct.u16[1] = 2222;
  col_struct.i32[0] = 10000;
  col_struct.i32[1] = 11111;
  col_struct.u32[0] = 20000;
  col_struct.u32[1] = 22222;
  col_struct.i64[0] = 100000;
  col_struct.i64[1] = 111111;
  col_struct.u64[0] = 200000;
  col_struct.u64[1] = 222222;
  col_struct.f32[0] = 0.10f;
  col_struct.f32[1] = 0.11f;
  col_struct.f64[0] = 0.20;
  col_struct.f64[1] = 0.22;
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(col_struct.f128[0], 0.30);
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(col_struct.f128[1], 0.33);
  col_struct.c8[0] = '1';
  col_struct.c8[1] = '2';
  col_struct.c16[0] = L'1';
  col_struct.c16[1] = L'2';
  col_struct.s8[0] = "Once";
  col_struct.s8[1] = "Twice";
  col_struct.s16[0] = L"Once";
  col_struct.s16[1] = L"Twice";
  col_struct.enum_type[0] = TypeBool;
  col_struct.enum_type[1] = TypeChar16;
  col_struct.simple_struct[0].value = 10;
  col_struct.simple_struct[1].value = 20;
  col_struct.simple_union[0].value(10);
  col_struct.simple_union[1].value(20);
}

template <typename T>
void test_collection_struct(T& col_struct)
{
  EXPECT_EQ(col_struct.byte[0], 0x01);
  EXPECT_EQ(col_struct.byte[1], 0x02);
  EXPECT_EQ(col_struct.boole[0], false);
  EXPECT_EQ(col_struct.boole[1], true);
  EXPECT_EQ(col_struct.i8[0], 100);
  EXPECT_EQ(col_struct.i8[1], 111);
  EXPECT_EQ(col_struct.u8[0], 200u);
  EXPECT_EQ(col_struct.u8[1], 222u);
  EXPECT_EQ(col_struct.i16[0], 1000);
  EXPECT_EQ(col_struct.i16[1], 1111);
  EXPECT_EQ(col_struct.u16[0], 2000u);
  EXPECT_EQ(col_struct.u16[1], 2222u);
  EXPECT_EQ(col_struct.i32[0], 10000);
  EXPECT_EQ(col_struct.i32[1], 11111);
  EXPECT_EQ(col_struct.u32[0], 20000u);
  EXPECT_EQ(col_struct.u32[1], 22222u);
  EXPECT_EQ(col_struct.i64[0], 100000);
  EXPECT_EQ(col_struct.i64[1], 111111);
  EXPECT_EQ(col_struct.u64[0], 200000u);
  EXPECT_EQ(col_struct.u64[1], 222222u);
  EXPECT_EQ(col_struct.f32[0], 0.10f);
  EXPECT_EQ(col_struct.f32[1], 0.11f);
  EXPECT_EQ(col_struct.f64[0], 0.20);
  EXPECT_EQ(col_struct.f64[1], 0.22);
  EXPECT_EQ(static_cast<double>(col_struct.f128[0]), 0.30);
  EXPECT_EQ(static_cast<double>(col_struct.f128[1]), 0.33);
  EXPECT_EQ(col_struct.c8[0], '1');
  EXPECT_EQ(col_struct.c8[1], '2');
  EXPECT_EQ(col_struct.c16[0], L'1');
  EXPECT_EQ(col_struct.c16[1], L'2');
  EXPECT_STREQ(col_struct.s8[0], "Once");
  EXPECT_STREQ(col_struct.s8[1], "Twice");
  EXPECT_STREQ(col_struct.s16[0], L"Once");
  EXPECT_STREQ(col_struct.s16[1], L"Twice");
  EXPECT_EQ(col_struct.enum_type[0], TypeBool);
  EXPECT_EQ(col_struct.enum_type[1], TypeChar16);
  EXPECT_EQ(col_struct.simple_struct[0].value, 10u);
  EXPECT_EQ(col_struct.simple_struct[1].value, 20u);
  EXPECT_EQ(col_struct.simple_union[0].value(), 10u);
  EXPECT_EQ(col_struct.simple_union[1].value(), 20u);
}

TEST_F(dds_DCPS_XTypes_DynamicDataAdapter, test_struct)
{
  add_type<TestStruct>();
  DDS::DynamicType_var dt = get_dynamic_type<TestStruct>();
  DDS::DynamicData_var ddi = DDS::DynamicDataFactory::get_instance()->create_data(dt);

  {
    TestStruct x;
    x.byte = 0xff;
    x.boole = true;
    x.i8 = 1;
    x.u8 = 2;
    x.i16 = 3;
    x.u16 = 4;
    x.i32 = 5;
    x.u32 = 6;
    x.i64 = 7;
    x.u64 = 8;
    x.f32 = 0.1f;
    x.f64 = 0.2;
    x.f128 = 0.3;
    x.c8 = '!';
    x.c16 = L'@';
    x.s8 = "Hello";
    x.s16 = L"Goodbye";
    x.enum_type = TypeEnum;
    x.simple_struct.value = 10;
    x.simple_union.value(20);
    set_collection_struct_seq_len(x.seqs);
    set_collection_struct(x.seqs);
    set_collection_struct_seq_len(x.anon_seqs);
    set_collection_struct(x.anon_seqs);
    set_collection_struct(x.arrays);
    set_collection_struct(x.anon_arrays);
    x.typedef_of_seq_typedef.length(2);
    x.typedef_of_seq_typedef[0] = 1;
    x.typedef_of_seq_typedef[1] = 2;
    x.typedef_of_array_typedef[0] = 1;
    x.typedef_of_array_typedef[1] = 2;

    DDS::DynamicData_var dda = get_dynamic_data_adapter<TestStruct>(dt, x);
    ASSERT_RC_OK(copy(ddi, dda));
  }

  {
    TestStruct y;
    DDS::DynamicData_var dda = get_dynamic_data_adapter<TestStruct>(dt, y);
    ASSERT_RC_OK(copy(dda, ddi));
    EXPECT_EQ(y.byte, 0xff);
    EXPECT_EQ(y.boole, true);
    EXPECT_EQ(y.i8, 1);
    EXPECT_EQ(y.u8, 2u);
    EXPECT_EQ(y.i16, 3);
    EXPECT_EQ(y.u16, 4u);
    EXPECT_EQ(y.i32, 5);
    EXPECT_EQ(y.u32, 6u);
    EXPECT_EQ(y.i64, 7);
    EXPECT_EQ(y.u64, 8u);
    EXPECT_EQ(y.f32, 0.1f);
    EXPECT_EQ(y.f64, 0.2);
    EXPECT_EQ(static_cast<double>(y.f128), 0.3);
    EXPECT_EQ(y.c8, '!');
    EXPECT_EQ(y.c16, L'@');
    EXPECT_STREQ(y.s8, "Hello");
    EXPECT_STREQ(y.s16, L"Goodbye");
    EXPECT_EQ(y.enum_type, TypeEnum);
    EXPECT_EQ(y.simple_struct.value, 10u);
    EXPECT_EQ(y.simple_union.value(), 20u);
    test_collection_struct_seq_len(y.seqs);
    test_collection_struct(y.seqs);
    test_collection_struct_seq_len(y.anon_seqs);
    test_collection_struct(y.anon_seqs);
    test_collection_struct(y.arrays);
    test_collection_struct(y.anon_arrays);
    ASSERT_EQ(y.typedef_of_seq_typedef.length(), 2u);
    EXPECT_EQ(y.typedef_of_seq_typedef[0], 1);
    EXPECT_EQ(y.typedef_of_seq_typedef[1], 2);
    EXPECT_EQ(y.typedef_of_array_typedef[0], 1);
    EXPECT_EQ(y.typedef_of_array_typedef[1], 2);
  }
}

TEST_F(dds_DCPS_XTypes_DynamicDataAdapter, test_union)
{
  add_type<TestUnion>();
  DDS::DynamicType_var dt = get_dynamic_type<TestUnion>();

  {
    DDS::DynamicData_var ddi = DDS::DynamicDataFactory::get_instance()->create_data(dt);
    {
      TestUnion tu;
      tu.i32(100);
      DDS::DynamicData_var dda = get_dynamic_data_adapter(dt, tu);
      ASSERT_RC_OK(copy(ddi, dda));
    }
    {
      TestUnion tu;
      DDS::DynamicData_var dda = get_dynamic_data_adapter(dt, tu);
      ASSERT_RC_OK(copy(dda, ddi));
      ASSERT_EQ(tu.i32(), 100);
    }
  }
  {
    DDS::DynamicData_var ddi = DDS::DynamicDataFactory::get_instance()->create_data(dt);
    const char* const s = "A string";
    {
      TestUnion tu;
      tu.s8(s);
      DDS::DynamicData_var dda = get_dynamic_data_adapter(dt, tu);
      ASSERT_RC_OK(copy(ddi, dda));
    }
    {
      TestUnion tu;
      DDS::DynamicData_var dda = get_dynamic_data_adapter(dt, tu);
      ASSERT_RC_OK(copy(dda, ddi));
      ASSERT_STREQ(tu.s8(), s);
    }
  }
  {
    DDS::DynamicData_var ddi = DDS::DynamicDataFactory::get_instance()->create_data(dt);
    {
      SimpleStruct ss;
      ss.value = 1;
      TestUnion tu;
      tu.simple_struct(ss);
      DDS::DynamicData_var dda = get_dynamic_data_adapter(dt, tu);
      ASSERT_RC_OK(copy(ddi, dda));
    }
    {
      TestUnion tu;
      DDS::DynamicData_var dda = get_dynamic_data_adapter(dt, tu);
      ASSERT_RC_OK(copy(dda, ddi));
      ASSERT_EQ(tu.simple_struct().value, 1u);
    }
  }
  {
    DDS::DynamicData_var ddi = DDS::DynamicDataFactory::get_instance()->create_data(dt);
    {
      SimpleUnion su;
      su.value(1);
      TestUnion tu;
      tu.simple_union(su);
      DDS::DynamicData_var dda = get_dynamic_data_adapter(dt, tu);
      ASSERT_RC_OK(copy(ddi, dda));
    }
    {
      TestUnion tu;
      DDS::DynamicData_var dda = get_dynamic_data_adapter(dt, tu);
      ASSERT_RC_OK(copy(dda, ddi));
      ASSERT_EQ(tu.simple_union().value(), 1u);
    }
  }
}

#  else // (No DynamicDataAdapter)
TEST_F(dds_DCPS_XTypes_DynamicDataAdapter, null_get_dynamic_data_adapter)
{
  add_type<SimpleStruct>();
  DDS::DynamicType_var dt = get_dynamic_type<SimpleStruct>();
  SimpleStruct ss;
  DDS::DynamicData_var dd = get_dynamic_data_adapter<SimpleStruct, SimpleStruct>(dt, ss);
  DDS::DynamicData* const null = 0;
  ASSERT_EQ(dd.in(), null);
}
#  endif

#endif // OPENDDS_SAFETY_PROFILE
