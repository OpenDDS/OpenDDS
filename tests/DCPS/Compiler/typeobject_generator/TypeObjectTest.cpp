#include "TypeObjectTestTypeSupportImpl.h"

#include <gtest/gtest.h>

#include <set>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;

void verify_some_union(const TypeIdentifier& ti, const TypeObject& to)
{
}

void verify_nested_struct(const TypeIdentifier& ti, const TypeObject& to)
{
}

void verify_common_enum_literal(const CommonEnumeratedLiteral& common, unsigned i)
{
  EXPECT_EQ(common.value, static_cast<ACE_CDR::Long>(i));
  if (i == 0) {
    EXPECT_EQ(common.flags, IS_DEFAULT);
  } else {
    EXPECT_EQ(common.flags, 0);
  }
}

void verify_union_disc(const TypeIdentifier& ti, const TypeObject& to)
{
  const char* literal_names[3] = {"SEQUENCE_MEMBER", "STRUCT_MEMBER", "PRIMITIVE_MEMBER"};

  if (ti.kind() == EK_MINIMAL) {
    const MinimalEnumeratedLiteralSeq& literal_seq = to.minimal.enumerated_type.literal_seq;
    for (unsigned i = 0; i < literal_seq.length(); ++i) {
      verify_common_enum_literal(literal_seq[i].common, i);
      NameHash nh;
      hash_member_name(nh, literal_names[i]);
      EXPECT_EQ(std::memcmp(literal_seq[i].detail.name_hash, nh, sizeof(NameHash)), 0);
    }

  } else {
    EXPECT_EQ(to.complete.enumerated_type.header.detail.type_name, String("::UnionDisc"));
    const CompleteEnumeratedLiteralSeq& literal_seq = to.complete.enumerated_type.literal_seq;
    for (unsigned i = 0; i < literal_seq.length(); ++i) {
      verify_common_enum_literal(literal_seq[i].common, i);
      EXPECT_EQ(literal_seq[i].detail.name, String(literal_names[i]));
    }
  }

  EXPECT_EQ(makeTypeIdentifier(to), ti);
}

void verify_long10seq(const TypeIdentifier& ti, const TypeObject& to)
{
  
}

void verify_typemap(const TypeMap& type_map, EquivalenceKind ek)
{
  const unsigned int num_types = 31;
  EXPECT_EQ(type_map.size(), num_types);

  TypeMap scc15_map, scc12_map, other_types_map;
  for (TypeMap::const_iterator it = type_map.begin(); it != type_map.end(); ++it) {
    if (it->first.kind() == TI_STRONGLY_CONNECTED_COMPONENT &&
        it->first.sc_component_id().scc_length == 15) {
      scc15_map.insert(*it);
    } else if (it->first.kind() == TI_STRONGLY_CONNECTED_COMPONENT &&
               it->first.sc_component_id().scc_length == 12) {
      scc12_map.insert(*it);
    } else {
      other_types_map.insert(*it);
    }
  }

  EXPECT_EQ(scc15_map.size(), static_cast<size_t>(15));
  EXPECT_EQ(scc12_map.size(), static_cast<size_t>(12));
  EXPECT_EQ(other_types_map.size(), static_cast<size_t>(4));

  std::set<ACE_CDR::Long> scc_indexes;
  for (ACE_CDR::Long i = 1; i <= 15; ++i) {
    scc_indexes.insert(i);
  }

  for (TypeMap::const_iterator it = scc15_map.begin(); it != scc15_map.end(); ++it) {
    EXPECT_EQ(it->first.sc_component_id().sc_component_id.kind, ek);
    EXPECT_EQ(it->first.sc_component_id().scc_length, 15);
    ACE_CDR::Long scc_index = it->first.sc_component_id().scc_index;
    EXPECT_TRUE(scc_indexes.find(scc_index) != scc_indexes.end());
    scc_indexes.erase(scc_index);

    // TODO(sonndinh): Verify each type.
    if (ek == EK_MINIMAL) {

    } else {

    }
  }
  EXPECT_TRUE(scc_indexes.empty());

  for (ACE_CDR::Long i = 1; i <= 12; ++i) {
    scc_indexes.insert(i);
  }

  for (TypeMap::const_iterator it = scc12_map.begin(); it != scc12_map.end(); ++it) {
    EXPECT_EQ(it->first.sc_component_id().sc_component_id.kind, ek);
    EXPECT_EQ(it->first.sc_component_id().scc_length, 12);
    ACE_CDR::Long scc_index = it->first.sc_component_id().scc_index;
    EXPECT_TRUE(scc_indexes.find(scc_index) != scc_indexes.end());
    scc_indexes.erase(scc_index);

    // TODO(sonndinh): Verify each type.
  }
  EXPECT_TRUE(scc_indexes.empty());

  for (TypeMap::const_iterator it = other_types_map.begin(); it != other_types_map.end(); ++it) {
    EXPECT_EQ(it->first.kind(), ek);
    EXPECT_EQ(it->second.kind, ek);

    TypeKind tk = ek == EK_MINIMAL ? it->second.minimal.kind : it->second.complete.kind;
    EXPECT_TRUE(tk == TK_UNION || tk == TK_STRUCTURE || tk == TK_ENUM || tk == TK_ALIAS);
    if (tk == TK_UNION) {
      verify_some_union(it->first, it->second);
    } else if (tk == TK_STRUCTURE) {
      verify_nested_struct(it->first, it->second);
    } else if (tk == TK_ENUM) {
      verify_union_disc(it->first, it->second);
    } else {
      verify_long10seq(it->first, it->second);
    }
  }
}

TEST(TypeMapTest, Minimal)
{
  const TypeMap& minimal_map = getMinimalTypeMap<A_xtag>();
  verify_typemap(minimal_map, EK_MINIMAL);
}

TEST(TypeMapTest, Complete)
{
  const TypeMap& complete_map = getCompleteTypeMap<A_xtag>();
  verify_typemap(complete_map, EK_COMPLETE);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
