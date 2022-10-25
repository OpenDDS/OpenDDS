#ifndef OPENDDS_SAFETY_PROFILE
#  include "../../../MaxExtensibilityTypeSupportImpl.h"

#  include <dds/DCPS/XTypes/Utils.h>
#  include <dds/DCPS/XTypes/TypeLookupService.h>

#  include <gtest/gtest.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;
using namespace max_extensibility;

class dds_DCPS_XTypes_Utils : public testing::Test {
  template<typename TopicType, typename Xtag>
  void add_type()
  {
    TypeIdentifierPairSeq tid_pairs;
    TypeIdentifierPair tid_pair;
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
    add_type<FinalStruct, max_extensibility_FinalStruct_xtag>();
    add_type<AppendableUnion, max_extensibility_AppendableUnion_xtag>();
    add_type<MutableStruct, max_extensibility_MutableStruct_xtag>();
    add_type<FinalMaxFinalStruct, max_extensibility_FinalMaxFinalStruct_xtag>();
    add_type<FinalMaxAppendableStruct, max_extensibility_FinalMaxAppendableStruct_xtag>();
    add_type<FinalMaxMutableStruct, max_extensibility_FinalMaxMutableStruct_xtag>();
    add_type<AppendableMaxAppendableStruct, max_extensibility_AppendableMaxAppendableStruct_xtag>();
    add_type<AppendableMaxMutableUnion, max_extensibility_AppendableMaxMutableUnion_xtag>();
    add_type<MutableMaxMutableStruct, max_extensibility_MutableMaxMutableStruct_xtag>();
  }

  TypeLookupService_rch tls_;
  template<typename Xtag>
  DDS::DynamicType_var get_dynamic_type()
  {
    const TypeIdentifier& com_ti = getCompleteTypeIdentifier<Xtag>();
    const TypeMap& com_map = getCompleteTypeMap<Xtag>();
    TypeMap::const_iterator pos = com_map.find(com_ti);
    EXPECT_TRUE(pos != com_map.end());
    const TypeObject& com_to = pos->second;
    return tls_->complete_to_dynamic(com_to.complete, GUID_UNKNOWN);
  }

public:
  template<typename Xtag>
  void expect_ext(Extensibility expected)
  {
    DDS::DynamicType_var dy = get_dynamic_type<Xtag>();
    Extensibility actual;
    EXPECT_EQ(DDS::RETCODE_OK, extensibility(dy.in(), actual));
    EXPECT_EQ(expected, actual);
  }

  template<typename Xtag>
  void expect_maxext(Extensibility expected)
  {
    DDS::DynamicType_var dy = get_dynamic_type<Xtag>();
    CORBA::String_var name = dy->get_name();
    Extensibility actual;
    EXPECT_EQ(DDS::RETCODE_OK, OpenDDS::XTypes::max_extensibility(dy.in(), actual)) << name.in();
    EXPECT_EQ(expected, actual) << name.in();
  }
};

TEST_F(dds_DCPS_XTypes_Utils, extensibility)
{
  expect_ext<max_extensibility_FinalStruct_xtag>(FINAL);
  expect_ext<max_extensibility_AppendableUnion_xtag>(APPENDABLE);
  expect_ext<max_extensibility_MutableStruct_xtag>(MUTABLE);
  expect_ext<max_extensibility_FinalMaxFinalStruct_xtag>(FINAL);
  expect_ext<max_extensibility_FinalMaxAppendableStruct_xtag>(FINAL);
  expect_ext<max_extensibility_FinalMaxMutableStruct_xtag>(FINAL);
  expect_ext<max_extensibility_AppendableMaxAppendableStruct_xtag>(APPENDABLE);
  expect_ext<max_extensibility_AppendableMaxMutableUnion_xtag>(APPENDABLE);
  expect_ext<max_extensibility_MutableMaxMutableStruct_xtag>(MUTABLE);
}

TEST_F(dds_DCPS_XTypes_Utils, max_extensibility)
{
  expect_maxext<max_extensibility_FinalStruct_xtag>(FINAL);
  expect_maxext<max_extensibility_AppendableUnion_xtag>(APPENDABLE);
  expect_maxext<max_extensibility_MutableStruct_xtag>(MUTABLE);
  expect_maxext<max_extensibility_FinalMaxFinalStruct_xtag>(FINAL);
  expect_maxext<max_extensibility_FinalMaxAppendableStruct_xtag>(APPENDABLE);
  expect_maxext<max_extensibility_FinalMaxMutableStruct_xtag>(MUTABLE);
  expect_maxext<max_extensibility_AppendableMaxAppendableStruct_xtag>(APPENDABLE);
  expect_maxext<max_extensibility_AppendableMaxMutableUnion_xtag>(MUTABLE);
  expect_maxext<max_extensibility_MutableMaxMutableStruct_xtag>(MUTABLE);
}
#endif // OPENDDS_SAFETY_PROFILE
