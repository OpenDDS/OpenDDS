#include <SerializedSizeValueWriterTypeSupportImpl.h>

#include <dds/DCPS/SerializedSizeValueWriter.h>
#include <dds/DCPS/XTypes/DynamicDataImpl.h>
#include <dds/DCPS/XTypes/DynamicDataAdapter.h>

#include <gtest/gtest.h>

using namespace OpenDDS;
using namespace Test;

DCPS::Encoding xcdr2(DCPS::Encoding::KIND_XCDR2, DCPS::ENDIAN_BIG);

void dump_sizes(const std::vector<size_t>& sizes)
{
  std::cout << "[ ";
  for (size_t i = 0; i < sizes.size(); ++i) {
    std::cout << sizes[i] << " ";
  }
  std::cout << "]" << std::endl;
}

template <typename BaseStructType>
void init_base_struct(BaseStructType& bst)
{
  bst.b_field = true;
  bst.f_field = 1.0f;
  bst.o_field = 0x01;
}

template <typename StructType>
void init(StructType& st)
{
  st.s_field = 10;
  st.l_field = 20;
  init_base_struct(st.nested_field);
  st.str_field = "hello";
  st.ull_field = 30;
}

template <typename Xtag, typename StructType>
void check_total_size(DCPS::SerializedSizeValueWriter& value_writer, const StructType& sample)
{
  // Serialized size returned from serialized_size.
  size_t expected_size = 0;
  DCPS::serialized_size(xcdr2, expected_size, sample);

  // Serialized size computed by vwrite with the C++ object.
  vwrite(value_writer, sample);
  EXPECT_EQ(expected_size, value_writer.get_serialized_size());

  // Serialized size computed by vwrite with a dynamic data object.
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<Xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<Xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());
  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());
  DDS::DynamicData_var dd = XTypes::get_dynamic_data_adapter<StructType, StructType>(dt, sample);

  vwrite(value_writer, dd.in());
  EXPECT_EQ(expected_size, value_writer.get_serialized_size());
}

void check_component_sizes(DCPS::SerializedSizeValueWriter& value_writer, const size_t* arr, size_t length)
{
  std::vector<size_t> expected_sizes(arr, arr + length);
  const std::vector<size_t>& sizes = value_writer.get_serialized_sizes();
  EXPECT_TRUE(expected_sizes == sizes);
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalFinalStruct)
{
  FinalFinalStruct ffs;
  init(ffs);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalFinalStruct_xtag>(value_writer, ffs);

  // Layout           Total size
  // 2: s_field       2
  // 2+4: l_field     8
  // [ nested_field
  // 1: b_field       9
  // 3+4: f_field     16
  // 1: o_field       17
  // nested_field ]
  // 3+4+6: str_field 30
  // 2+8: ull_field   (40)
  const size_t arr[] = {40};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalAppendableStruct)
{
  FinalAppendableStruct fas;
  init(fas);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalAppendableStruct_xtag>(value_writer, fas);

  // Layout           Nested    Total size
  // 2: s_field                 2
  // 2+4: l_field               8
  // [ nested_field
  // 4: Dheader       4         12
  // 1: b_field       5         13
  // 3+4: f_field     12        20
  // 1: o_field       (13)      21
  // nested_field ]
  // 3+4+6: str_field           34
  // 2+8: ull_field             (44)
  const size_t arr[] = {44, 13};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalMutableStruct)
{
  FinalMutableStruct fms;
  init(fms);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalMutableStruct_xtag>(value_writer, fms);

  // Layout           Nested    Total size
  // 2: s_field                 2
  // 2+4: l_field               8
  // [ nested_field
  // 4: Dheader       4         12
  // 4: Emheader      8         16
  // 1: b_field       9         17
  // 3+4: Emheader    16        24
  // 4: f_field       20        28
  // 4: Emheader      24        32
  // 1: o_field       (25)      33
  // nested_field ]
  // 3+4+6: str_field           46
  // 2+8: ull_field             (56)
  const size_t arr[] = {56, 25, 1, 4, 1};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableFinalStruct)
{
  AppendableFinalStruct afs;
  init(afs);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableFinalStruct_xtag>(value_writer, afs);

  // Layout           Total size
  // 4: Dheader       4
  // 2: s_field       6
  // 2+4: l_field     12
  // [ nested_field
  // 1: b_field       13
  // 3+4: f_field     20
  // 1: o_field       21
  // nested_field ]
  // 3+4+6: str_field 34
  // 2+8: ull_field   (44)
  const size_t arr[] = {44};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableAppendableStruct)
{
  AppendableAppendableStruct aas;
  init(aas);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableAppendableStruct_xtag>(value_writer, aas);

  // Layout           Nested    Total size
  // 4: Dheader                 4
  // 2: s_field                 6
  // 2+4: l_field               12
  // [ nested_field
  // 4: Dheader       4         16
  // 1: b_field       5         17
  // 3+4: f_field     12        24
  // 1: o_field       (13)      25
  // nested_field ]
  // 3+4+6: str_field           38
  // 2+8: ull_field             (48)
  const size_t arr[] = {48, 13};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableMutableStruct)
{
  AppendableMutableStruct ams;
  init(ams);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableMutableStruct_xtag>(value_writer, ams);

  // Layout           Nested    Total size
  // 4: Dheader                 4
  // 2: s_field                 6
  // 2+4: l_field               12
  // [ nested_field
  // 4: Dheader       4         16
  // 4: Emheader      8         20
  // 1: b_field       9         21
  // 3+4: Emheader    16        28
  // 4: f_field       20        32
  // 4: Emheader      24        36
  // 1: o_field       (25)      37
  // nested_field ]
  // 3+4+6: str_field           50
  // 2+8: ull_field             (60)
  const size_t arr[] = {60, 25, 1, 4, 1};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableFinalStruct)
{
  MutableFinalStruct mfs;
  init(mfs);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableFinalStruct_xtag>(value_writer, mfs);

  // Layout           Nested    Total size
  // 4: Dheader                 4
  // 4: Emheader                8
  // 2: s_field                 10
  // 2+4: Emheader              16
  // 4: l_field                 20
  // 4: Emheader                24
  // 4: Nextint                 28
  // [ nested_field
  // 1: b_field       1         29
  // 3+4: f_field     8         36
  // 1: o_field       (9)       37
  // nested_field ]
  // 3+4: Emheader              44
  // 4: Nextint                 48
  // 4+6: str_field             58
  // 2+4: Emheader              64
  // 8: ull_field               (72)
  const size_t arr[] = {72, 2, 4, 9, 10, 8};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableAppendableStruct)
{
  MutableAppendableStruct mas;
  init(mas);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableAppendableStruct_xtag>(value_writer, mas);

  // Layout           Nested    Total size
  // 4: Dheader                 4
  // 4: Emheader                8
  // 2: s_field                 10
  // 2+4: Emheader              16
  // 4: l_field                 20
  // 4: Emheader                24
  // 4: Nextint                 28
  // [ nested_field
  // 4: Dheader       4         32
  // 1: b_field       5         33
  // 3+4: f_field     12        40
  // 1: o_field       (13)      41
  // nested_field ]
  // 3+4: Emheader              48
  // 4: Nextint                 52
  // 4+6: str_field             62
  // 2+4: Emheader              68
  // 8: ull_field               (76)
  const size_t arr[] = {76, 2, 4, 13, 10, 8};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableMutableStruct)
{
  MutableMutableStruct mms;
  init(mms);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableMutableStruct_xtag>(value_writer, mms);

  // Layout           Nested    Total size
  // 4: Dheader                 4
  // 4: Emheader                8
  // 2: s_field                 10
  // 2+4: Emheader              16
  // 4: l_field                 20
  // 4: Emheader                24
  // 4: Nextint                 28
  // [ nested_field
  // 4: Dheader       4         32
  // 4: Emheader      8         36
  // 1: b_field       9         37
  // 3+4: Emheader    16        44
  // 4: f_field       20        48
  // 4: Emheader      24        52
  // 1: o_field       (25)      53
  // nested_field ]
  // 3+4: Emheader              60
  // 4: Nextint                 64
  // 4+6: str_field             74
  // 2+4: Emheader              80
  // 8: ull_field               (88)
  const size_t arr[] = {88, 2, 4, 25, 1, 4, 1, 10, 8};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));
}

// TODO(sonndinh): Check component sizes

void init(FinalUnion& fu)
{
  AppendableMutableStruct ams;
  init(ams);
  fu.nested_field(ams);
}

void init(AppendableUnion& au)
{
  MutableFinalStruct mfs;
  init(mfs);
  au.nested_field(mfs);
}

void init(MutableUnion& mu)
{
  FinalAppendableStruct fas;
  init(fas);
  mu.nested_field(fas);
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalUnion)
{
  FinalUnion fu;
  init(fu);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalUnion_xtag>(value_writer, fu);
}


TEST(dds_DCPS_SerializedSizeValueWriter, AppendableUnion)
{
  AppendableUnion au;
  init(au);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableUnion_xtag>(value_writer, au);
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableUnion)
{
  MutableUnion mu;
  init(mu);
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableUnion_xtag>(value_writer, mu);
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalComplexStruct)
{
  FinalComplexStruct fcs;
  fcs.c_field = 'd';

  FinalUnion fu;
  init(fu);
  fcs.nested_union = fu;

  fcs.ll_field = 123;
  init_base_struct(fcs.nested_struct);
  init(fcs.nnested_struct);
  fcs.str_field = "my string";

  fcs.seq_field.length(2);
  fcs.seq_field[0] = 0;
  fcs.seq_field[1] = 1;

  fcs.arr_field[0] = 10;
  fcs.arr_field[1] = 20;

  fcs.md_arr_field[0][0] = 0;
  fcs.md_arr_field[0][1] = 1;
  fcs.md_arr_field[1][0] = 10;
  fcs.md_arr_field[1][1] = 11;

  fcs.nested_seq.length(2);
  AppendableStruct as;
  init_base_struct(as);
  fcs.nested_seq[0] = as;
  fcs.nested_seq[1] = as;

  MutableStruct ms;
  init_base_struct(ms);
  fcs.nested_arr[0] = ms;
  fcs.nested_arr[1] = ms;

  fcs.f_field = 2.0f;

  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalComplexStruct_xtag>(value_writer, fcs);
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableComplexStruct)
{
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableComplexStruct)
{
}
