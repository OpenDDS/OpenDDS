#ifndef OPENDDS_SAFETY_PROFILE
#  include "XTypesUtilsTypeSupportImpl.h"

#  include <dds/DCPS/XTypes/Utils.h>
#  include <dds/DCPS/XTypes/TypeLookupService.h>

#  include <gtest/gtest.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;

void nameMatches(const MinimalMemberDetail& mmd, const char* name)
{
  NameHash nh;
  hash_member_name(nh, name);
  EXPECT_TRUE(name_hash_equal(mmd.name_hash, nh));
}

void nameMatches(const CompleteMemberDetail& cmd, const char* name)
{
  EXPECT_EQ(cmd.name, name);
}

template <typename MCTO> // Minimal/Complete TypeObject
void checkTO(const MCTO& mcto)
{
  EXPECT_EQ(mcto.kind, TK_ENUM);
  EXPECT_EQ(mcto.enumerated_type.literal_seq.length(), 2);
  EXPECT_EQ(mcto.enumerated_type.literal_seq[0].common.value, 0);
  nameMatches(mcto.enumerated_type.literal_seq[0].detail, "X");
  EXPECT_EQ(mcto.enumerated_type.literal_seq[1].common.value, 1);
  nameMatches(mcto.enumerated_type.literal_seq[1].detail, "Z");
}

void checkTI(const TypeIdentifier& ti, const TypeMap& map)
{
  const TypeMap::const_iterator it = map.find(ti);
  EXPECT_NE(it, map.end());
  switch (it->second.kind) {
  case EK_MINIMAL:
    checkTO(it->second.minimal);
    break;
  case EK_COMPLETE:
    checkTO(it->second.complete);
    break;
  }
}

template <typename CMSMS> // Complete/Minimal StructMemberSeq
void checkStructMembers(const CMSMS& cmsms, const TypeMap& map)
{
  if (cmsms.length() == 1) {
    return; // nested struct UsesEnumS
  }
  EXPECT_EQ(8, cmsms.length());
  for (DDS::UInt32 i = 0; i < cmsms.length(); ++i) {
    const TypeIdentifier& ti = cmsms[i].common.member_type_id;
    switch (ti.kind()) {
    case EK_MINIMAL:
    case EK_COMPLETE:
      EXPECT_EQ(1, map.count(ti));
      break;
    case TI_PLAIN_SEQUENCE_SMALL:
      EXPECT_EQ(1, map.count(*ti.seq_sdefn().element_identifier));
      break;
    case TI_PLAIN_ARRAY_SMALL:
      EXPECT_EQ(1, map.count(*ti.array_sdefn().element_identifier));
      break;
    default:
      EXPECT_TRUE(false);
    }
  }
}

void checkStruct(const TypeObject& to, const TypeMap& map)
{
  switch (to.kind) {
  case EK_MINIMAL:
    checkStructMembers(to.minimal.struct_type.member_seq, map);
    break;
  case EK_COMPLETE:
    checkStructMembers(to.complete.struct_type.member_seq, map);
    break;
  }
}

void checkAdded(const TypeMap& map)
{
  size_t enums = 0, aliases = 0, structs = 0, unions = 0;
  for (TypeMap::const_iterator it = map.begin(); it != map.end(); ++it) {
    TypeKind kind = TK_NONE;
    switch (it->second.kind) {
    case EK_MINIMAL:
      kind = it->second.minimal.kind;
      break;
    case EK_COMPLETE:
      kind = it->second.complete.kind;
      break;
    }
    switch (kind) {
    case TK_ENUM: ++enums; break;
    case TK_ALIAS: ++aliases; break;
    case TK_UNION: ++unions; break;
    case TK_STRUCTURE:
      ++structs;
      checkStruct(it->second, map);
      break;
    }
  }
  EXPECT_EQ(1, enums); // Enu_t
  EXPECT_EQ(2, aliases); // UsesEnumA, EnuSeq
  EXPECT_EQ(2, structs); // UsesEnumS, TestEnums
  EXPECT_EQ(1, unions); // UsesEnumU
}

TEST(dds_DCPS_XTypes_Utils, remove_enumerators)
{
  TypeLookupService_rch tls_ = make_rch<TypeLookupService>();
  const TypeMap& minimalTM = getMinimalTypeMap<XTypesUtils_TestEnums_xtag>();
  const TypeMap& completeTM = getCompleteTypeMap<XTypesUtils_TestEnums_xtag>();
  tls_->add(minimalTM.begin(), minimalTM.end());
  tls_->add(completeTM.begin(), completeTM.end());

  Sequence<DDS::Int32> values_to_remove;
  values_to_remove.append(1);
  // Original: enum Enu_t { X, Y, Z };
  // Modified: enum Enu_t { X, Z };

  TypeMap miniAdded, compAdded;
  TypeIdentifier newEnumM, newEnumC;
  const TypeIdentifier newStructM = remove_enumerators(getMinimalTypeIdentifier<XTypesUtils_TestEnums_xtag>(),
                                                       getMinimalTypeIdentifier<XTypesUtils_Enu_t_xtag>(),
                                                       values_to_remove, *tls_, miniAdded, &newEnumM);
  EXPECT_NE(newStructM, TypeIdentifier::None);
  checkTI(newEnumM, miniAdded);
  EXPECT_EQ(miniAdded.size(), 6);
  checkAdded(miniAdded);
  const TypeIdentifier newStructC = remove_enumerators(getCompleteTypeIdentifier<XTypesUtils_TestEnums_xtag>(),
                                                       getCompleteTypeIdentifier<XTypesUtils_Enu_t_xtag>(),
                                                       values_to_remove, *tls_, compAdded, &newEnumC);
  EXPECT_NE(newStructC, TypeIdentifier::None);
  checkTI(newEnumC, compAdded);
  EXPECT_EQ(compAdded.size(), 6);
  checkAdded(compAdded);
}

#endif // OPENDDS_SAFETY_PROFILE
