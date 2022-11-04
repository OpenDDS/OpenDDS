#ifndef OPENDDS_SAFETY_PROFILE
#  include <XTypesUtilsTypeSupportImpl.h>
#  include <key_annotationTypeSupportImpl.h>

#  include <dds/DCPS/XTypes/Utils.h>
#  include <dds/DCPS/XTypes/TypeLookupService.h>

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
    EXPECT_EQ(DDS::RETCODE_OK, extensibility(dt.in(), actual)) << "Happend in type " << name.in();
    EXPECT_EQ(expected, actual) << "Happend in type " << name.in();
  }

  template<typename TopicType>
  void expect_maxext(Extensibility expected)
  {
    add_type<TopicType>();
    DDS::DynamicType_var dt = get_dynamic_type<TopicType>();
    CORBA::String_var name = dt->get_name();
    Extensibility actual;
    EXPECT_EQ(DDS::RETCODE_OK, max_extensibility(dt.in(), actual))
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
      EXPECT_EQ(DDS::RETCODE_OK, get_keys(dt, actual))
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

#endif // OPENDDS_SAFETY_PROFILE
