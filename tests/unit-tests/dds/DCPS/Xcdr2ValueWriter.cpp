#include <Xcdr2ValueWriterTypeSupportImpl.h>

#include "../../../Utils/DataView.h"

#include <dds/DCPS/Xcdr2ValueWriter.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/DCPS/XTypes/DynamicVwrite.h>
#  include <dds/DCPS/XTypes/DynamicDataAdapter.h>
#endif

#include <gtest/gtest.h>

using namespace OpenDDS;
using namespace ::Test;

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

#ifndef OPENDDS_SAFETY_PROFILE
template <typename Xtag>
DDS::DynamicType_var get_dynamic_type(XTypes::TypeLookupService& tls)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<Xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<Xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());
  tls.add(type_map.begin(), type_map.end());
  return tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());
}
#endif

template <typename Xtag, typename Type>
void check_total_size(DCPS::Xcdr2ValueWriter& value_writer, const Type& sample)
{
  // Serialized size returned from serialized_size.
  size_t expected_size = 0;
  DCPS::serialized_size(xcdr2, expected_size, sample);

  // Serialized size computed by vwrite with the C++ object.
  EXPECT_TRUE(vwrite(value_writer, sample));
  EXPECT_EQ(expected_size, value_writer.get_serialized_size());

#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
  // Serialized size computed by vwrite with a dynamic data object.
  XTypes::TypeLookupService tls;
  DDS::DynamicType_var dt = get_dynamic_type<Xtag>(tls);
  DDS::DynamicData_var dd = XTypes::get_dynamic_data_adapter<Type, Type>(dt, sample);
  EXPECT_TRUE(vwrite(value_writer, dd.in()));
  EXPECT_EQ(expected_size, value_writer.get_serialized_size());
#endif
}

void check_component_sizes(DCPS::Xcdr2ValueWriter& value_writer, const size_t* arr, size_t length)
{
  const OPENDDS_VECTOR(size_t) expected_sizes(arr, arr + length);
  const OPENDDS_VECTOR(size_t)& sizes = value_writer.get_serialized_sizes();
  EXPECT_EQ(expected_sizes, sizes);
}

template <typename Xtag, typename Type>
void check_serialized_data(DCPS::Xcdr2ValueWriter& value_writer, const Type& sample)
{
  // Use generated marshal code on C++ object.
  ACE_Message_Block expected_cdr(value_writer.get_serialized_size());
  {
    DCPS::Serializer ser(&expected_cdr, xcdr2);
    ser << sample;
  }

  // Serialize using generated vwrite on C++ object.
  {
    ACE_Message_Block buffer(value_writer.get_serialized_size());
    DCPS::Serializer ser(&buffer, xcdr2);
    value_writer.set_serializer(&ser);
    EXPECT_TRUE(vwrite(value_writer, sample));
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }

#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
  // Serialize using vwrite on a dynamic data object.
  {
    ACE_Message_Block buffer(value_writer.get_serialized_size());
    DCPS::Serializer ser(&buffer, xcdr2);
    XTypes::TypeLookupService tls;
    DDS::DynamicType_var dt = get_dynamic_type<Xtag>(tls);
    DDS::DynamicData_var dd = XTypes::get_dynamic_data_adapter<Type, Type>(dt, sample);
    value_writer.set_serializer(&ser);
    EXPECT_TRUE(vwrite(value_writer, dd.in()));
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
#endif
}

TEST(dds_DCPS_Xcdr2ValueWriter, FinalFinalStruct)
{
  FinalFinalStruct ffs;
  init(ffs);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalFinalStruct_xtag>(value_writer, ffs);

  // Layout        Running size
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

  check_serialized_data<DCPS::Test_FinalFinalStruct_xtag>(value_writer, ffs);
}

TEST(dds_DCPS_Xcdr2ValueWriter, FinalAppendableStruct)
{
  FinalAppendableStruct fas;
  init(fas);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalAppendableStruct_xtag>(value_writer, fas);

  // Layout        Nested    Running size
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

  check_serialized_data<DCPS::Test_FinalAppendableStruct_xtag>(value_writer, fas);
}

TEST(dds_DCPS_Xcdr2ValueWriter, FinalMutableStruct)
{
  FinalMutableStruct fms;
  init(fms);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalMutableStruct_xtag>(value_writer, fms);

  // Layout         Nested    Running size
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

  check_serialized_data<DCPS::Test_FinalMutableStruct_xtag>(value_writer, fms);
}

TEST(dds_DCPS_Xcdr2ValueWriter, AppendableFinalStruct)
{
  AppendableFinalStruct afs;
  init(afs);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableFinalStruct_xtag>(value_writer, afs);

  // Layout         Running size
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

  check_serialized_data<DCPS::Test_AppendableFinalStruct_xtag>(value_writer, afs);
}

TEST(dds_DCPS_Xcdr2ValueWriter, AppendableAppendableStruct)
{
  AppendableAppendableStruct aas;
  init(aas);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableAppendableStruct_xtag>(value_writer, aas);

  // Layout        Nested    Running size
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

  check_serialized_data<DCPS::Test_AppendableAppendableStruct_xtag>(value_writer, aas);
}

TEST(dds_DCPS_Xcdr2ValueWriter, AppendableMutableStruct)
{
  AppendableMutableStruct ams;
  init(ams);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableMutableStruct_xtag>(value_writer, ams);

  // Layout        Nested    Running size
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

  check_serialized_data<DCPS::Test_AppendableMutableStruct_xtag>(value_writer, ams);
}

TEST(dds_DCPS_Xcdr2ValueWriter, MutableFinalStruct)
{
  MutableFinalStruct mfs;
  init(mfs);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableFinalStruct_xtag>(value_writer, mfs);

  // Layout       Nested    Running size
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

  check_serialized_data<DCPS::Test_MutableFinalStruct_xtag>(value_writer, mfs);
}

TEST(dds_DCPS_Xcdr2ValueWriter, MutableAppendableStruct)
{
  MutableAppendableStruct mas;
  init(mas);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableAppendableStruct_xtag>(value_writer, mas);

  // Layout         Nested   Running size
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

  check_serialized_data<DCPS::Test_MutableAppendableStruct_xtag>(value_writer, mas);
}

TEST(dds_DCPS_Xcdr2ValueWriter, MutableMutableStruct)
{
  MutableMutableStruct mms;
  init(mms);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableMutableStruct_xtag>(value_writer, mms);

  // Layout         Nested   Running size
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

  check_serialized_data<DCPS::Test_MutableMutableStruct_xtag>(value_writer, mms);
}

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

TEST(dds_DCPS_Xcdr2ValueWriter, FinalUnion)
{
  FinalUnion fu;
  init(fu);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalUnion_xtag>(value_writer, fu);

  check_serialized_data<DCPS::Test_FinalUnion_xtag>(value_writer, fu);
}


TEST(dds_DCPS_Xcdr2ValueWriter, AppendableUnion)
{
  AppendableUnion au;
  init(au);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableUnion_xtag>(value_writer, au);

  check_serialized_data<DCPS::Test_AppendableUnion_xtag>(value_writer, au);
}

TEST(dds_DCPS_Xcdr2ValueWriter, MutableUnion)
{
  MutableUnion mu;
  init(mu);
  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableUnion_xtag>(value_writer, mu);

  check_serialized_data<DCPS::Test_MutableUnion_xtag>(value_writer, mu);
}

TEST(dds_DCPS_Xcdr2ValueWriter, FinalComplexStruct)
{
  FinalComplexStruct fcs;
  fcs.c_field = 'd';
  init(fcs.nested_union);
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

  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_FinalComplexStruct_xtag>(value_writer, fcs);

  // Layout                   Running size     Component sizes
  // 1: c_field                    1            (276)
  // [= nested_union
  // 3+4: disc                     8
  // [== nested_field                           (60)
  // 4: DHEADER                    12
  // 2: s_field                    14
  // 2+4: l_field                  20
  // [=== nested_field                          (25)
  // 4: DHEADER                    24
  // 4: EMHEADER                   28
  // 1: b_field                    29           (1)
  // 3+4: EMHEADER                 36
  // 4: f_field                    40           (4)
  // 4: EMHEADER                   44
  // 1: o_field                    45           (1)
  // nested_field ===]
  // 3+4+6: str_field              58
  // 2+8: ull_field                68
  // nested_field ==]
  // nested_union =]
  // 8: ll_field                   76
  // [= nested_struct                           (13)
  // 4: DHEADER                    80
  // 1: b_field                    81
  // 3+4: f_field                  88
  // 1: o_field                    89
  // nested_struct =]
  // [= nnested_struct
  // 1+2: s_field                  92
  // 4: l_field                    96
  // [== nested_field                           (13)
  // 4: DHEADER                    100
  // 1: b_field                    101
  // 3+4: f_field                  108
  // 1: o_field                    109
  // nested_field =]
  // 3+4+6: str_field              122
  // 2+8: ull_field                132
  // nnested_struct =]
  // 4+10: str_field               146
  // 2+4+2+2: seq_field            156
  // 4+4: arr_field                164
  // 2+2+2+2: md_arr_field         172
  // [= nested_seq                              (37)
  // 4: DHEADER                    176
  // 4: length                     180
  // [== nested_seq[0]                          (13)
  // 4: DHEADER                    184
  // 1+3+4+1: fields               193
  // nested_seq[0] ==]
  // [== nested_seq[1]                          (13)
  // 3+4: DHEADER                  200
  // 1+3+4+1: fields               209
  // nested_seq[1] ==]
  // nested_seq =]
  // [= nested_arr                              (57)
  // 3+4: DHEADER                  216
  // [== nested_arr[0]                          (25)
  // 4: DHEADER                    220
  // 4+1+3+4+4+4+1=21: fields      241          (1) (4) (1)
  // nested_arr[0] =]
  // [== nested_arr[1]                          (25)
  // 3+4: DHEADER                  248
  // 4+1+3+4+4+4+1=21: fields      269          (1) (4) (1)
  // nested_arr[1] ==]
  // nested_arr =]
  // 3+4: f_field                  276
  const size_t arr[] = {276, 60, 25, 1, 4, 1, 13, 13, 37, 13, 13, 57, 25, 1, 4, 1, 25, 1, 4, 1};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));

  check_serialized_data<DCPS::Test_FinalComplexStruct_xtag>(value_writer, fcs);
}

TEST(dds_DCPS_Xcdr2ValueWriter, AppendableComplexStruct)
{
  AppendableComplexStruct acs;
  acs.s_field = 1;
  init(acs.nested_union);
  acs.f_field = 1.0f;
  init_base_struct(acs.nested_struct);
  init(acs.nnested_struct);
  acs.str_field = "my string";

  acs.seq_field.length(2);
  acs.seq_field[0] = 0;
  acs.seq_field[1] = 1;

  acs.arr_field[0] = ONE;
  acs.arr_field[1] = TWO;

  acs.md_arr_field[0][0] = 0;
  acs.md_arr_field[0][1] = 1;
  acs.md_arr_field[1][0] = 10;
  acs.md_arr_field[1][1] = 11;

  acs.nested_seq.length(2);
  MutableStruct ms;
  init_base_struct(ms);
  acs.nested_seq[0] = ms;
  acs.nested_seq[1] = ms;

  FinalStruct fs;
  init_base_struct(fs);
  acs.nested_arr[0] = fs;
  acs.nested_arr[1] = fs;

  acs.d_field = 1.0;

  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_AppendableComplexStruct_xtag>(value_writer, acs);

  // Layout              Running size   Component sizes
  // 4: DHEADER                          (332)
  // 2: s_field                    2
  // [= nested_union                     (80)
  // 2+4: DHEADER                  8
  // 4: disc                       12
  // [== nested_field                    (72)
  // 4: DHEADER                    16
  // 4: EMHEADER                   20
  // 2: s_field                    22    (2)
  // 2+4: EMHEADER                 28
  // 4: l_field                    32    (4)
  // 4: EMHEADER                   36
  // 4: NEXTINT                    40
  // [=== nested_field                   (9)
  // 1: b_field                    41
  // 3+4: f_field                  48
  // 1: o_field                    49
  // nested_field ===]
  // 3+4: EMHEADER                 56
  // 4: NEXTINT                    60
  // 4+6: str_field                70    (10)
  // 2+4: EMHEADER                 76
  // 8: ull_field                  84    (8)
  // nested_field ==]
  // nested_union =]
  // 4: f_field                    88
  // [= nested_struct                    (25)
  // 4: DHEADER                    92
  // 4: EMHEADER                   96
  // 1: b_field                    97    (1)
  // 3+4: EMHEADER                 104
  // 4: f_field                    108   (4)
  // 4: EMHEADER                   112
  // 1: o_field                    113   (1)
  // nested_struct =]
  // [= nnested_struct                   (60)
  // 3+4: DHEADER                  120
  // 2: s_field                    122
  // 2+4: l_field                  128
  // [== nested_field                    (25)
  // 4: DHEADER                    132
  // 4: EMHEADER                   136
  // 1: b_field                    137   (1)
  // 3+4: EMHEADER                 144
  // 4: f_field                    148   (4)
  // 4: EMHEADER                   152
  // 1: o_field                    153   (1)
  // nested_field ==]
  // 3+4+6: str_field              166
  // 2+8: ull_field                176
  // nnested_struct =]
  // 4+10: str_field               190
  // 2+4+4+4: seq_field            204
  // [= arr_field
  // 4: DHEADER                    208   (12)
  // 4+4: elements                 216
  // arr_field =]
  // 4+4+4+4: md_arr_field         232
  // [= nested_seq                       (61)
  // 4: DHEADER                    236
  // 4: length                     240
  // [== nested_seq[0]
  // 4: DHEADER                    244   (25)
  // 4: EMHEADER                   248
  // 1: b_field                    249   (1)
  // 3+4: EMHEADER                 256
  // 4: f_field                    260   (4)
  // 4: EMHEADER                   264
  // 1: o_field                    265   (1)
  // nested_seq[0] ==]
  // [== nested_seq[1]
  // 3+4: DHEADER                  272   (25)
  // 4: EMHEADER                   276
  // 1: b_field                    277   (1)
  // 3+4: EMHEADER                 284
  // 4: f_field                    288   (4)
  // 4: EMHEADER                   292
  // 1: o_field                    293   (1)
  // nested_seq[1] ==]
  // nested_seq =]
  // [= nested_arr
  // 3+4: DHEADER                  300   (21)
  // [== nested_arr[0]
  // 1: b_field                    301
  // 3+4: f_field                  308
  // 1: o_field                    309
  // nested_arr[0] ==]
  // [== nested_arr[1]
  // 1: b_field                    310
  // 2+4: f_field                  316
  // 1: o_field                    317
  // nested_arr[1] ==]
  // nested_arr =]
  // 3+8: d_field                  328
  const size_t arr[] = {332,80,72,2,4,9,10,8,25,1,4,1,60,25,1,4,1,12,61,25,1,4,1,25,1,4,1,21};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));

  check_serialized_data<DCPS::Test_AppendableComplexStruct_xtag>(value_writer, acs);
}

TEST(dds_DCPS_Xcdr2ValueWriter, MutableComplexStruct)
{
  MutableComplexStruct mcs;
  mcs.str_field = "my string";
  init(mcs.nested_union);
  mcs.s_field = 1;
  init_base_struct(mcs.nested_struct);
  init(mcs.nnested_struct);
  mcs.str2_field = "my string2";

  mcs.seq_field.length(2);
  mcs.seq_field[0] = 0;
  mcs.seq_field[1] = 1;

  mcs.arr_field[0] = 10;
  mcs.arr_field[1] = 20;

  mcs.md_arr_field[0][0] = 0;
  mcs.md_arr_field[0][1] = 1;
  mcs.md_arr_field[1][0] = 10;
  mcs.md_arr_field[1][1] = 11;

  mcs.nested_seq.length(2);
  FinalStruct fs;
  init_base_struct(fs);
  mcs.nested_seq[0] = fs;
  mcs.nested_seq[1] = fs;

  AppendableStruct as;
  init_base_struct(as);
  mcs.nested_arr[0] = as;
  mcs.nested_arr[1] = as;

  mcs.l_field = 11;

  DCPS::Xcdr2ValueWriter value_writer(xcdr2);
  check_total_size<DCPS::Test_MutableComplexStruct_xtag>(value_writer, mcs);

  // Layout              Running size   Component sizes
  // 4: DHEADER             4               (356)
  // 4: EMHEADER            8
  // 4: NEXTINT             12
  // 4+10: str_field        26              (14)
  // 2+4: EMHEADER          32
  // 4: NEXTINT             36
  // [= nested_union                        (64)
  // 4: DHEADER             40
  // 4: EMHEADER            44
  // 4: disc                48              (4)
  // 4: EMHEADER            52
  // 4: NEXTINT             56
  // [== nested_field                       (44)
  // 2: s_field             58
  // 2+4: l_field           64
  // [=== nested_field                      (13)
  // 4: DHEADER             68
  // 1: b_field             69
  // 3+4: f_field           76
  // 1: o_field             77
  // nested_field ===]
  // 3+4+6: str_field       90
  // 2+8: ull_field         100
  // nested_field ==]
  // nested_union =]
  // 4: EMHEADER            104
  // 2: s_field             106             (2)
  // 2+4: EMHEADER          112
  // 4: NEXTINT             116
  // [= nested_struct                       (9)
  // 1: b_field             117
  // 3+4: f_field           124
  // 1: o_field             125
  // nested_struct =]
  // 3+4: EMHEADER          132
  // 4: NEXTINT             136
  // [= nnested_struct                      (72)
  // 4: DHEADER             140
  // 4: EMHEADER            144
  // 2: s_field             146             (2)
  // 2+4: EMHEADER          152
  // 4: l_field             156             (4)
  // 4: EMHEADER            160
  // 4: NEXTINT             164
  // [== nested_field                       (9)
  // 1: b_field             165
  // 3+4: f_field           172
  // 1: o_field             173
  // nested_field ==]
  // 3+4: EMHEADER          180
  // 4: NEXTINT             184
  // 4+6: str_field         194             (10)
  // 2+4: EMHEADER          200
  // 8: ull_field           208             (8)
  // nnested_struct =]
  // 4: EMHEADER            212
  // 4: NEXTINT             216
  // 4+11: str2_field       231             (15)
  // 1+4: EMHEADER          236
  // 4+2+2: seq_field       244             (8)
  // 4: EMHEADER            248
  // 4+4: arr_field         256             (8)
  // 4: EMHEADER            260
  // 2+2+2+2: md_arr_field  268             (8)
  // 4: EMHEADER            272
  // 4: NEXTINT             276
  // [= nested_seq                          (25)
  // 4: DHEADER             280
  // 4: length              284
  // [== nested_seq[0]
  // 1: b_field             285
  // 3+4: f_field           292
  // 1: o_field             293
  // nested_seq[0] ==]
  // [== nested_seq[1]
  // 1: b_field             294
  // 2+4: f_field           300
  // 1: o_field             301
  // nested_seq[1] ==]
  // nested_seq =]
  // 3+4: EMHEADER          308
  // 4: NEXTINT             312
  // [= nested_arr                          (33)
  // 4: DHEADER             316
  // [== nested_arr[0]
  // 4: DHEADER             320             (13)
  // 1: b_field             321
  // 3+4: f_field           328
  // 1: o_field             329
  // nested_arr[0] ==]
  // [== nested_arr[1]
  // 3+4: DHEADER           336             (13)
  // 1: b_field             337
  // 3+4: f_field           344
  // 1: o_field             345
  // nested_arr[1] ==]
  // nested_arr =]
  // 3+4: EMHEADER          352
  // 4: l_field             356             (4)
  const size_t arr[] = {356,14,64,4,44,13,2,9,72,2,4,9,10,8,15,8,8,8,25,33,13,13,4};
  check_component_sizes(value_writer, arr, sizeof(arr) / sizeof(arr[0]));

  check_serialized_data<DCPS::Test_MutableComplexStruct_xtag>(value_writer, mcs);
}
