#ifndef OPENDDS_SAFETY_PROFILE
#  include <XTypesUtilsTypeSupportImpl.h>

#  include <dds/DCPS/XTypes/Utils.h>
#  include <dds/DCPS/XTypes/TypeLookupService.h>

#  include <gtest/gtest.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;
using namespace XTypesUtils;

class dds_DCPS_XTypes_Utils : public testing::Test {

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

    // Types for extensibility tests
    add_type<FinalStruct>();
    add_type<AppendableUnion>();
    add_type<MutableStruct>();
    add_type<FinalMaxFinalStruct>();
    add_type<FinalMaxAppendableStruct>();
    add_type<FinalMaxMutableStruct>();
    add_type<AppendableMaxAppendableStruct>();
    add_type<AppendableMaxMutableUnion>();
    add_type<MutableMaxMutableStruct>();
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

public:
  template<typename TopicType>
  void expect_ext(Extensibility expected)
  {
    DDS::DynamicType_var dy = get_dynamic_type<TopicType>();
    Extensibility actual;
    EXPECT_EQ(DDS::RETCODE_OK, extensibility(dy.in(), actual));
    EXPECT_EQ(expected, actual);
  }

  template<typename TopicType>
  void expect_maxext(Extensibility expected)
  {
    DDS::DynamicType_var dy = get_dynamic_type<TopicType>();
    CORBA::String_var name = dy->get_name();
    Extensibility actual;
    EXPECT_EQ(DDS::RETCODE_OK, max_extensibility(dy.in(), actual)) << name.in();
    EXPECT_EQ(expected, actual) << name.in();
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
#endif // OPENDDS_SAFETY_PROFILE
