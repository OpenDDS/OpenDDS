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

TEST_F(dds_DCPS_XTypes_DynamicDataAdapter, direct_simple_struct)
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

#endif // OPENDDS_SAFETY_PROFILE
