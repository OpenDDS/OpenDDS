#ifndef OPENDDS_SAFETY_PROFILE

#include <DynamicDataXcdrReadImplTypeSupportImpl.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/XTypes/TypeLookupService.h>

#include <gtest/gtest.h>

#include <cstring>

using namespace OpenDDS;
using namespace OpenDDS::XTypes;

namespace {
  // Members of the generated SCC for Node and NodeSeq, in scc_index order
  TypeIdentifierTypeObjectPairSeq node_scc(EquivalenceKind ek)
  {
    const TypeMap& type_map = ek == EK_MINIMAL ?
      DCPS::getMinimalTypeMap<DCPS::Node_xtag>() : DCPS::getCompleteTypeMap<DCPS::Node_xtag>();
    const TypeIdentifier& node = ek == EK_MINIMAL ?
      DCPS::getMinimalTypeIdentifier<DCPS::Node_xtag>() : DCPS::getCompleteTypeIdentifier<DCPS::Node_xtag>();
    TypeIdentifierTypeObjectPairSeq pairs;
    for (TypeMap::const_iterator pos = type_map.begin(); pos != type_map.end(); ++pos) {
      if (pos->first.kind() == TI_STRONGLY_CONNECTED_COMPONENT &&
          make_scc_id_or_default(pos->first) == make_scc_id_or_default(node)) {
        pairs.append(TypeIdentifierTypeObjectPair(pos->first, pos->second));
      }
    }
    return pairs;
  }

  TypeIdentifierTypeObjectPair struct_pair(EquivalenceKind ek)
  {
    const TypeMap& type_map = ek == EK_MINIMAL ?
      DCPS::getMinimalTypeMap<DCPS::MutableArrayStruct_xtag>() :
      DCPS::getCompleteTypeMap<DCPS::MutableArrayStruct_xtag>();
    const TypeIdentifier& ti = ek == EK_MINIMAL ?
      DCPS::getMinimalTypeIdentifier<DCPS::MutableArrayStruct_xtag>() :
      DCPS::getCompleteTypeIdentifier<DCPS::MutableArrayStruct_xtag>();
    return TypeIdentifierTypeObjectPair(ti, type_map.find(ti)->second);
  }

  void add_pair(TypeIdentifierTypeObjectPairSeq& seq, const TypeIdentifierTypeObjectPair& pair)
  {
    seq.append(pair);
  }

  struct ValidateTypeObjects {
    explicit ValidateTypeObjects(bool flag)
      : prev(TheServiceParticipant->validate_type_objects())
    {
      TheServiceParticipant->validate_type_objects(flag);
    }
    ~ValidateTypeObjects()
    {
      TheServiceParticipant->validate_type_objects(prev);
    }
    const bool prev;
  };
}

TEST(dds_DCPS_XTypes_TypeLookupService, SccHashMatchesGenerated)
{
  const EquivalenceKind kinds[] = {EK_MINIMAL, EK_COMPLETE};
  for (size_t k = 0; k < 2; ++k) {
    const TypeIdentifierTypeObjectPairSeq pairs = node_scc(kinds[k]);
    ASSERT_GT(pairs.length(), 1u);
    TypeObjectSeq members;
    for (ACE_CDR::ULong i = 0; i < pairs.length(); ++i) {
      EXPECT_EQ(static_cast<ACE_CDR::Long>(i + 1), pairs[i].type_identifier.sc_component_id().scc_index);
      members.append(pairs[i].type_object);
    }
    const StronglyConnectedComponentId& scc = pairs[0].type_identifier.sc_component_id();
    EquivalenceHash hash;
    make_scc_hash(hash, scc, members);
    EXPECT_EQ(0, std::memcmp(hash, scc.sc_component_id.hash, sizeof hash));
  }
}

TEST(dds_DCPS_XTypes_TypeLookupService, AddTypeObjectsValidatesHash)
{
  const ValidateTypeObjects validate(true);
  const TypeIdentifierTypeObjectPair good = struct_pair(EK_MINIMAL);

  TypeIdentifierTypeObjectPair bad = good;
  bad.type_identifier.equivalence_hash()[0] ^= 1;
  TypeIdentifierTypeObjectPair wrong_kind(struct_pair(EK_COMPLETE).type_identifier, good.type_object);

  TypeIdentifierTypeObjectPairSeq types;
  add_pair(types, good);
  add_pair(types, bad);
  add_pair(types, wrong_kind);

  TypeLookupService tls;
  tls.add_type_objects_to_cache(types);
  EXPECT_TRUE(tls.type_object_in_cache(good.type_identifier));
  EXPECT_FALSE(tls.type_object_in_cache(bad.type_identifier));
  EXPECT_FALSE(tls.type_object_in_cache(wrong_kind.type_identifier));
}

TEST(dds_DCPS_XTypes_TypeLookupService, AddTypeObjectsWithoutValidation)
{
  const ValidateTypeObjects validate(false);
  TypeIdentifierTypeObjectPair bad = struct_pair(EK_MINIMAL);
  bad.type_identifier.equivalence_hash()[0] ^= 1;
  TypeIdentifierTypeObjectPairSeq types;
  add_pair(types, bad);

  TypeLookupService tls;
  tls.add_type_objects_to_cache(types);
  EXPECT_TRUE(tls.type_object_in_cache(bad.type_identifier));
}

TEST(dds_DCPS_XTypes_TypeLookupService, AddTypeObjectsDoesNotReplace)
{
  const ValidateTypeObjects validate(false);
  const TypeIdentifierTypeObjectPair good = struct_pair(EK_MINIMAL);
  TypeIdentifierTypeObjectPairSeq types;
  add_pair(types, TypeIdentifierTypeObjectPair(good.type_identifier, struct_pair(EK_COMPLETE).type_object));

  TypeLookupService tls;
  tls.add(good.type_identifier, good.type_object);
  tls.add_type_objects_to_cache(types);
  EXPECT_EQ(EK_MINIMAL, tls.get_type_object(good.type_identifier).kind);
}

TEST(dds_DCPS_XTypes_TypeLookupService, AddSccOnlyWhenComplete)
{
  const ValidateTypeObjects validate(true);
  const TypeIdentifierTypeObjectPairSeq scc = node_scc(EK_MINIMAL);

  {
    TypeLookupService tls;
    tls.add_type_objects_to_cache(scc);
    for (ACE_CDR::ULong i = 0; i < scc.length(); ++i) {
      EXPECT_TRUE(tls.type_object_in_cache(scc[i].type_identifier));
    }
  }

  {
    // Missing a member
    TypeIdentifierTypeObjectPairSeq partial;
    for (ACE_CDR::ULong i = 1; i < scc.length(); ++i) {
      add_pair(partial, scc[i]);
    }
    TypeLookupService tls;
    tls.add_type_objects_to_cache(partial);
    for (ACE_CDR::ULong i = 0; i < scc.length(); ++i) {
      EXPECT_FALSE(tls.type_object_in_cache(scc[i].type_identifier));
    }
  }

  {
    // Members swapped
    TypeIdentifierTypeObjectPairSeq swapped(scc);
    swapped[0].type_object = scc[1].type_object;
    swapped[1].type_object = scc[0].type_object;
    TypeLookupService tls;
    tls.add_type_objects_to_cache(swapped);
    for (ACE_CDR::ULong i = 0; i < scc.length(); ++i) {
      EXPECT_FALSE(tls.type_object_in_cache(scc[i].type_identifier));
    }
  }

  {
    // Index out of range
    TypeIdentifierTypeObjectPairSeq extra(scc);
    TypeIdentifierTypeObjectPair beyond = scc[0];
    beyond.type_identifier.sc_component_id().scc_index = scc[0].type_identifier.sc_component_id().scc_length + 1;
    add_pair(extra, beyond);
    TypeLookupService tls;
    tls.add_type_objects_to_cache(extra);
    for (ACE_CDR::ULong i = 0; i < scc.length(); ++i) {
      EXPECT_FALSE(tls.type_object_in_cache(scc[i].type_identifier));
    }
  }
}

TEST(dds_DCPS_XTypes_TypeLookupService, GetTypeObjectsScc)
{
  const TypeIdentifierTypeObjectPairSeq scc = node_scc(EK_MINIMAL);
  const TypeIdentifier scc_id = make_scc_id_or_default(scc[0].type_identifier);

  TypeLookupService tls;
  for (ACE_CDR::ULong i = 0; i < scc.length(); ++i) {
    tls.add(scc[i].type_identifier, scc[i].type_object);
  }

  TypeIdentifierSeq request;
  request.append(scc_id);
  TypeIdentifierTypeObjectPairSeq reply;
  tls.get_type_objects(request, reply);
  EXPECT_EQ(scc.length(), reply.length());

  // A huge scc_length doesn't match the cached component
  TypeIdentifier huge = scc_id;
  huge.sc_component_id().scc_length = 0x7fffffff;
  request[0] = huge;
  reply.length(0);
  tls.get_type_objects(request, reply);
  EXPECT_EQ(0u, reply.length());

  // Incomplete components are not sent
  TypeLookupService partial;
  partial.add(scc[0].type_identifier, scc[0].type_object);
  request[0] = scc_id;
  partial.get_type_objects(request, reply);
  EXPECT_EQ(0u, reply.length());
}

TEST(dds_DCPS_XTypes_TypeLookupService, RemoteTypeIdentifierMap)
{
  const ValidateTypeObjects validate(true);
  const TypeIdentifierTypeObjectPair minimal = struct_pair(EK_MINIMAL);
  const TypeIdentifierTypeObjectPair complete = struct_pair(EK_COMPLETE);

  TypeLookupService tls;
  tls.add(complete.type_identifier, complete.type_object);

  TypeIdentifierPairSeq pairs;
  TypeIdentifier wrong = minimal.type_identifier;
  wrong.equivalence_hash()[0] ^= 1;
  pairs.append(TypeIdentifierPair(complete.type_identifier, wrong));
  tls.add_remote_type_identifier_map(pairs);
  TypeIdentifier result;
  EXPECT_FALSE(tls.get_minimal_type_identifier(complete.type_identifier, result));

  pairs[0].type_identifier2 = minimal.type_identifier;
  tls.add_remote_type_identifier_map(pairs);
  EXPECT_TRUE(tls.get_minimal_type_identifier(complete.type_identifier, result));
  EXPECT_EQ(minimal.type_identifier, result);
}

#endif // OPENDDS_SAFETY_PROFILE
