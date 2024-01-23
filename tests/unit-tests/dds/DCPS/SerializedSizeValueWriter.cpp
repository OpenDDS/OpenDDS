#include <SerializedSizeValueWriterTypeSupportImpl.h>

#include <dds/DCPS/SerializedSizeValueWriter.h>
#include <dds/DCPS/XTypes/DynamicDataImpl.h>
#include <dds/DCPS/XTypes/DynamicDataAdapter.h>

#include <gtest/gtest.h>

using namespace OpenDDS;

DCPS::Encoding xcdr2(DCPS::Encoding::KIND_XCDR2, DCPS::ENDIAN_BIG);

void dump_sizes(const std::vector<size_t>& sizes)
{
  std::cout << "[ ";
  for (size_t i = 0; i < sizes.size(); ++i) {
    std::cout << sizes[i] << " ";
  }
  std::cout << "]" << std::endl;
}

template <typename StructType>
void init(StructType& st)
{
  st.s_field = 10;
  st.l_field = 20;
  st.nested_field.b_field = true;
  st.nested_field.f_field = 1.0f;
  st.nested_field.o_field = 0x01;
  st.str_field = "hello";
  st.ull_field = 30;
}

template <typename Xtag, typename StructType>
void check_total_size(const StructType& sample)
{
  // Serialized size returned from serialized_size.
  size_t expected_size = 0;
  DCPS::serialized_size(xcdr2, expected_size, sample);

  // Serialized size computed by vwrite with the C++ object.
  DCPS::SerializedSizeValueWriter value_writer(xcdr2);
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

TEST(dds_DCPS_SerializedSizeValueWriter, FinalFinalStruct)
{
  ValueWriter::FinalFinalStruct ffs;
  init(ffs);
  check_total_size<DCPS::ValueWriter_FinalFinalStruct_xtag>(ffs);
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalAppendableStruct)
{
  ValueWriter::FinalAppendableStruct fas;
  init(fas);
  check_total_size<DCPS::ValueWriter_FinalAppendableStruct_xtag>(fas);
}

TEST(dds_DCPS_SerializedSizeValueWriter, FinalMutableStruct)
{
  ValueWriter::FinalMutableStruct fms;
  init(fms);
  check_total_size<DCPS::ValueWriter_FinalMutableStruct_xtag>(fms);
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableFinalStruct)
{
  ValueWriter::AppendableFinalStruct afs;
  init(afs);
  check_total_size<DCPS::ValueWriter_AppendableFinalStruct_xtag>(afs);
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableAppendableStruct)
{
  ValueWriter::AppendableAppendableStruct aas;
  init(aas);
  check_total_size<DCPS::ValueWriter_AppendableAppendableStruct_xtag>(aas);
}

TEST(dds_DCPS_SerializedSizeValueWriter, AppendableMutableStruct)
{
  ValueWriter::AppendableMutableStruct ams;
  init(ams);
  check_total_size<DCPS::ValueWriter_AppendableMutableStruct_xtag>(ams);
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableFinalStruct)
{
  ValueWriter::MutableFinalStruct mfs;
  init(mfs);
  check_total_size<DCPS::ValueWriter_MutableFinalStruct_xtag>(mfs);
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableAppendableStruct)
{
  ValueWriter::MutableAppendableStruct mas;
  init(mas);
  check_total_size<DCPS::ValueWriter_MutableAppendableStruct_xtag>(mas);
}

TEST(dds_DCPS_SerializedSizeValueWriter, MutableMutableStruct)
{
  ValueWriter::MutableMutableStruct mms;
  init(mms);
  check_total_size<DCPS::ValueWriter_MutableMutableStruct_xtag>(mms);
}
