#include "TypeObjectTestTypeSupportImpl.h"

#include <gtest/gtest.h>

#include <set>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;

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

void verify_some_union_member(const CommonUnionMember& common,
                              const MinimalMemberDetail* minimal_detail,
                              const CompleteMemberDetail* complete_detail,
                              const TypeIdentifier& alias_member,
                              const TypeIdentifier& struct_member,
                              const TypeIdentifier& double_member)
{
  EXPECT_TRUE(common.type_id == alias_member ||
              common.type_id == struct_member ||
              common.type_id == double_member);

  NameHash name_hash;
  EXPECT_GT(common.member_flags | TRY_CONSTRUCT1, 0);
  if (common.type_id == alias_member) {
    EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(0));
    EXPECT_EQ(common.label_seq.length(), static_cast<ACE_CDR::ULong>(1));
    EXPECT_EQ(common.label_seq[0], static_cast<ACE_CDR::Long>(0));
    if (minimal_detail) {
      hash_member_name(name_hash, "a_seq");
      EXPECT_EQ(std::memcmp(minimal_detail->name_hash, name_hash, sizeof name_hash), 0);
    } else if (complete_detail) {
      EXPECT_EQ(complete_detail->name, "a_seq");
    }
  } else if (common.type_id == struct_member) {
    EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(1));
    EXPECT_EQ(common.label_seq.length(), static_cast<ACE_CDR::ULong>(1));
    EXPECT_EQ(common.label_seq[0], static_cast<ACE_CDR::Long>(1));
    if (minimal_detail) {
      hash_member_name(name_hash, "a_struct");
      EXPECT_EQ(std::memcmp(minimal_detail->name_hash, name_hash, sizeof name_hash), 0);
    } else if (complete_detail) {
      EXPECT_EQ(complete_detail->name, "a_struct");
    }
  } else {
    EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(2));
    EXPECT_GT(common.member_flags | IS_DEFAULT, 0);
    EXPECT_EQ(common.label_seq.length(), static_cast<ACE_CDR::ULong>(1));
    EXPECT_EQ(common.label_seq[0], static_cast<ACE_CDR::Long>(2));
    if (minimal_detail) {
      hash_member_name(name_hash, "a_double");
      EXPECT_EQ(std::memcmp(minimal_detail->name_hash, name_hash, sizeof name_hash), 0);
    } else if (complete_detail) {
      EXPECT_EQ(complete_detail->name, "a_double");
    }
  }
}

void verify_some_union(const TypeIdentifier& ti, const TypeObject& to, const TypeMap& type_map)
{
  const UnionTypeFlag& type_flags = ti.kind() == EK_MINIMAL ? to.minimal.union_type.union_flags :
    to.complete.union_type.union_flags;
  EXPECT_GT(type_flags | IS_FINAL, 0);

  TypeIdentifier disc_member, alias_member, struct_member, double_member(TK_FLOAT64);
  for (TypeMap::const_iterator it = type_map.begin(); it != type_map.end(); ++it) {
    const TypeKind tk = it->first.kind() == EK_MINIMAL ?
      it->second.minimal.kind : it->second.complete.kind;
    if (tk == TK_ENUM) {
      disc_member = it->first;
    } else if (tk == TK_ALIAS) {
      alias_member = it->first;
    } else if (tk == TK_STRUCTURE) {
      struct_member = it->first;
    }
  }

  if (ti.kind() == EK_MINIMAL) {
    EXPECT_GT(to.minimal.union_type.discriminator.common.member_flags | IS_KEY, 0);
    EXPECT_TRUE(to.minimal.union_type.discriminator.common.type_id == disc_member);

    const MinimalUnionMemberSeq& member_seq = to.minimal.union_type.member_seq;
    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      verify_some_union_member(member_seq[i].common, &member_seq[i].detail, 0, alias_member, struct_member, double_member);
    }
  } else {
    EXPECT_EQ(to.complete.union_type.header.detail.type_name, "::SomeUnion");
    EXPECT_GT(to.complete.union_type.discriminator.common.member_flags | IS_KEY, 0);
    EXPECT_TRUE(to.complete.union_type.discriminator.common.type_id == disc_member);

    const CompleteUnionMemberSeq& member_seq = to.complete.union_type.member_seq;
    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      verify_some_union_member(member_seq[i].common, 0, &member_seq[i].detail, alias_member, struct_member, double_member);
    }
  }
}

void verify_nested_struct_member_common(const CommonStructMember& common,
                                        String field_name,
                                        String field_hash_name)
{
  EXPECT_EQ(common.member_id, hash_member_name_to_id(field_hash_name));
  if (field_name == "a_short") {
    EXPECT_GT(common.member_flags | IS_MUST_UNDERSTAND, 0);
    EXPECT_EQ(common.member_type_id.kind(), TK_INT16);
  } else if (field_name == "a_long") {
    EXPECT_EQ(common.member_type_id.kind(), TK_INT32);
  } else {
    EXPECT_EQ(common.member_type_id.kind(), TK_FLOAT32);
  }
}

void verify_nested_struct(const TypeIdentifier& ti, const TypeObject& to)
{
  StructTypeFlag type_flags = ti.kind() == EK_MINIMAL ? to.minimal.struct_type.struct_flags :
    to.complete.struct_type.struct_flags;
  EXPECT_GT(type_flags | IS_MUTABLE, 0);

  if (ti.kind() == EK_MINIMAL) {
    EXPECT_EQ(to.minimal.struct_type.header.base_type.kind(), TK_NONE);
    const MinimalStructMemberSeq& member_seq = to.minimal.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(3));

    NameHash name_hashes[3];
    hash_member_name(name_hashes[0], "a_short");
    hash_member_name(name_hashes[1], "a_long");
    hash_member_name(name_hashes[2], "a_float");
    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const MinimalStructMember& member = member_seq[i];
      const NameHash& name_hash = member.detail.name_hash;
      EXPECT_TRUE(std::memcmp(name_hash, name_hashes[0], sizeof(NameHash)) == 0 ||
                  std::memcmp(name_hash, name_hashes[1], sizeof(NameHash)) == 0 ||
                  std::memcmp(name_hash, name_hashes[2], sizeof(NameHash)) == 0);

      if (std::memcmp(name_hash, name_hashes[0], sizeof(NameHash)) == 0) {
        verify_nested_struct_member_common(member.common, "a_short", "short_member");
      } else if (std::memcmp(name_hash, name_hashes[1], sizeof(NameHash)) == 0) {
        verify_nested_struct_member_common(member.common, "a_long", "long_member");
      } else {
        verify_nested_struct_member_common(member.common, "a_float", "float_member");
      }
    }
  } else {
    EXPECT_EQ(to.complete.struct_type.header.base_type.kind(), TK_NONE);
    EXPECT_EQ(to.complete.struct_type.header.detail.type_name, "::NestedStruct");
    const CompleteStructMemberSeq& member_seq = to.complete.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(3));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const CompleteStructMember& member = member_seq[i];
      const MemberName& name = member.detail.name;
      EXPECT_TRUE(name == "a_short" || name == "a_long" || name == "a_float");

      if (name == "a_short") {
        verify_nested_struct_member_common(member.common, "a_short", "short_member");
      } else if (name == "a_long") {
        verify_nested_struct_member_common(member.common, "a_long", "long_member");
      } else {
        verify_nested_struct_member_common(member.common, "a_float", "float_member");
      }
    }
  }
}

void verify_long10seq(const TypeIdentifier& ti, const TypeObject& to)
{
  const TypeIdentifier& base_type = ti.kind() == EK_MINIMAL ?
    to.minimal.alias_type.body.common.related_type :
    to.complete.alias_type.body.common.related_type;
  EXPECT_EQ(base_type.kind(), TI_PLAIN_SEQUENCE_SMALL);
  EXPECT_EQ(base_type.seq_sdefn().header.equiv_kind, EK_BOTH);
  EXPECT_EQ(base_type.seq_sdefn().header.element_flags, TRY_CONSTRUCT1);
  EXPECT_EQ(base_type.seq_sdefn().bound, static_cast<ACE_CDR::Octet>(10));
  EXPECT_EQ(base_type.seq_sdefn().element_identifier->kind(), TK_INT32);

  if (ti.kind() == EK_COMPLETE) {
    EXPECT_EQ(to.complete.alias_type.header.detail.type_name, "::Long10Seq");
  }

  EXPECT_EQ(makeTypeIdentifier(to), ti);
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

    const TypeKind tk = ek == EK_MINIMAL ? it->second.minimal.kind : it->second.complete.kind;
    EXPECT_TRUE(tk == TK_UNION || tk == TK_STRUCTURE || tk == TK_ENUM || tk == TK_ALIAS);
    if (tk == TK_UNION) {
      verify_some_union(it->first, it->second, other_types_map);
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
