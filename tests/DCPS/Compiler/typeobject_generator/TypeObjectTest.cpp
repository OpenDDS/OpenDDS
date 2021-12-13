#include "TypeObjectTestTypeSupportImpl.h"

#include <gtest/gtest.h>

#include <set>
#include <map>

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

void extract_scc(const TypeMap& type_map, TypeMap& scc15_map, TypeMap& scc12_map, TypeMap& other_types_map)
{
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
}

void common_check_typemap(const TypeMap& scc15_map, const TypeMap& scc12_map,
                          const TypeMap& other_types_map, EquivalenceKind ek)
{
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

enum TypeName { A, B, C, D, E, F, G, H, I,
  A_SEQ, B_SEQ, C_SEQ, D_SEQ, E_SEQ, F_SEQ, G_SEQ, H_SEQ, I_SEQ,
  A_SEQ_ALIAS, B_SEQ_ALIAS, C_SEQ_ALIAS, D_SEQ_ALIAS, E_SEQ_ALIAS,
  F_SEQ_ALIAS, G_SEQ_ALIAS, H_SEQ_ALIAS, I_SEQ_ALIAS };

// Map from each type to its SCC index.
typedef std::map<ACE_CDR::Long, TypeName> IndexMap;

IndexMap compute_scc15_index_map(const TypeMap& com_scc15)
{
  IndexMap result;

  for (TypeMap::const_iterator it = com_scc15.begin(); it != com_scc15.end(); ++it) {
    const TypeKind tk = it->second.complete.kind;
    const ACE_CDR::Long index = it->first.sc_component_id().scc_index;
    if (tk == TK_ALIAS) {
      const String type_name = it->second.complete.alias_type.header.detail.type_name;
      EXPECT_TRUE(type_name == "::ASequence" || type_name == "::BSeq" ||
                  type_name == "::CSeq" || type_name == "::DSeq" || type_name == "::ESeq");

      if (type_name == "::ASequence") {
        result[index] = A_SEQ_ALIAS;
      } else if (type_name == "::BSeq") {
        result[index] = B_SEQ_ALIAS;
      } else if (type_name == "::CSeq") {
        result[index] = C_SEQ_ALIAS;
      } else if (type_name == "::DSeq") {
        result[index] = D_SEQ_ALIAS;
      } else {
        result[index] = E_SEQ_ALIAS;
      }
    } else if (tk == TK_STRUCTURE) {
      const String type_name = it->second.complete.struct_type.header.detail.type_name;
      EXPECT_TRUE(type_name == "::A" || type_name == "::B" || type_name == "::C" ||
                  type_name == "::D" || type_name == "::E");

      if (type_name == "::A") {
        result[index] = A;
      } else if (type_name == "::B") {
        result[index] = B;
      } else if (type_name == "::C") {
        result[index] = C;
      } else if (type_name == "::D") {
        result[index] = D;
      } else {
        result[index] = E;
      }
    }
  }

  for (TypeMap::const_iterator it = com_scc15.begin(); it != com_scc15.end(); ++it) {
    const TypeKind tk = it->second.complete.kind;
    const ACE_CDR::Long index = it->first.sc_component_id().scc_index;
    if (tk == TK_SEQUENCE) {
      const TypeIdentifier& elem_ti = it->second.complete.sequence_type.element.common.type;
      TypeMap::const_iterator elem_pos = com_scc15.find(elem_ti);
      EXPECT_TRUE(elem_pos != com_scc15.end());
      EXPECT_EQ(elem_pos->second.complete.kind, TK_STRUCTURE);
      const String elem_type_name = elem_pos->second.complete.struct_type.header.detail.type_name;

      if (elem_type_name == "::A") {
        result[index] = A_SEQ;
      } else if (elem_type_name == "::B") {
        result[index] = B_SEQ;
      } else if (elem_type_name == "::C") {
        result[index] = C_SEQ;
      } else if (elem_type_name == "::D") {
        result[index] = D_SEQ;
      } else {
        result[index] = E_SEQ;
      }
    }
  }

  return result;
}

IndexMap compute_scc12_index_map(const TypeMap& com_scc12)
{
  IndexMap result;

  for (TypeMap::const_iterator it = com_scc12.begin(); it != com_scc12.end(); ++it) {
    const TypeKind tk = it->second.complete.kind;
    const ACE_CDR::Long index = it->first.sc_component_id().scc_index;
    if (tk == TK_ALIAS) {
      const String type_name = it->second.complete.alias_type.header.detail.type_name;
      EXPECT_TRUE(type_name == "::FSeq" || type_name == "::GSeq" ||
                  type_name == "::HSeq" || type_name == "::ISeq");

      if (type_name == "::FSeq") {
        result[index] = F_SEQ_ALIAS;
      } else if (type_name == "::GSeq") {
        result[index] = G_SEQ_ALIAS;
      } else if (type_name == "::HSeq") {
        result[index] = H_SEQ_ALIAS;
      } else {
        result[index] = I_SEQ_ALIAS;
      }
    } else if (tk == TK_STRUCTURE) {
      const String type_name = it->second.complete.struct_type.header.detail.type_name;
      EXPECT_TRUE(type_name == "::F" || type_name == "::G" ||
                  type_name == "::H" || type_name == "::I");

      if (type_name == "::F") {
        result[index] = F;
      } else if (type_name == "::G") {
        result[index] = G;
      } else if (type_name == "::H") {
        result[index] = H;
      } else {
        result[index] = I;
      }
    }
  }

  for (TypeMap::const_iterator it = com_scc12.begin(); it != com_scc12.end(); ++it) {
    const TypeKind tk = it->second.complete.kind;
    const ACE_CDR::Long index = it->first.sc_component_id().scc_index;
    if (tk == TK_SEQUENCE) {
      const TypeIdentifier& elem_ti = it->second.complete.sequence_type.element.common.type;
      TypeMap::const_iterator elem_pos = com_scc12.find(elem_ti);
      EXPECT_TRUE(elem_pos != com_scc12.end());
      EXPECT_EQ(elem_pos->second.complete.kind, TK_STRUCTURE);
      const String elem_type_name = elem_pos->second.complete.struct_type.header.detail.type_name;

      if (elem_type_name == "::F") {
        result[index] = F_SEQ;
      } else if (elem_type_name == "::G") {
        result[index] = G_SEQ;
      } else if (elem_type_name == "::H") {
        result[index] = H_SEQ;
      } else {
        result[index] = I_SEQ;
      }
    }
  }

  return result;
}

void check_struct_a_alias_member(const CommonStructMember& common, const TypeMap& scc15, const IndexMap& indexes)
{
  EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(0));
  EXPECT_EQ(common.member_flags, TRY_CONSTRUCT1);
  EXPECT_TRUE(scc15.find(common.member_type_id) != scc15.end());
  const ACE_CDR::Long idx = common.member_type_id.sc_component_id().scc_index;
  const IndexMap::const_iterator pos = indexes.find(idx);
  EXPECT_TRUE(pos != indexes.end() && pos->second == B_SEQ_ALIAS);
}

void check_struct_a_sequence_member(const CommonStructMember& common)
{
  EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(1));
  EXPECT_EQ(common.member_flags, TRY_CONSTRUCT1);
  const PlainSequenceSElemDefn& seq_sdefn = common.member_type_id.seq_sdefn();
  EXPECT_EQ(seq_sdefn.header.equiv_kind, EK_BOTH);
  EXPECT_EQ(seq_sdefn.header.element_flags, TRY_CONSTRUCT1);
  EXPECT_EQ(seq_sdefn.bound, ACE_CDR::Octet(0));
  EXPECT_EQ(seq_sdefn.element_identifier->kind(), TK_BOOLEAN);
}

void check_struct_a(const TypeMap& scc15, const TypeObject& to, const IndexMap& indexes, EquivalenceKind ek)
{
  const StructTypeFlag type_flags = ek == EK_MINIMAL ?
    to.minimal.struct_type.struct_flags : to.complete.struct_type.struct_flags;
  EXPECT_GT(type_flags | IS_APPENDABLE, 0);

  if (ek == EK_MINIMAL) {
    EXPECT_EQ(to.minimal.struct_type.header.base_type, TypeIdentifier(TK_NONE));
    const MinimalStructMemberSeq& member_seq = to.minimal.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(2));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const TypeKind tk = member_seq[i].common.member_type_id.kind();
      EXPECT_TRUE(tk == TI_STRONGLY_CONNECTED_COMPONENT ||
                  tk == TI_PLAIN_SEQUENCE_SMALL);
      NameHash nh;
      if (tk == TI_STRONGLY_CONNECTED_COMPONENT) {
        check_struct_a_alias_member(member_seq[i].common, scc15, indexes);
        hash_member_name(nh, "a_bseq");
      } else {
        check_struct_a_sequence_member(member_seq[i].common);
        hash_member_name(nh, "a_boolseq");
      }
      EXPECT_EQ(std::memcmp(nh, member_seq[i].detail.name_hash, sizeof nh), 0);
    }
  } else {
    EXPECT_EQ(to.complete.struct_type.header.base_type, TypeIdentifier(TK_NONE));
    EXPECT_EQ(to.complete.struct_type.header.detail.type_name, "::A");
    const CompleteStructMemberSeq& member_seq = to.complete.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(2));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const TypeKind tk = member_seq[i].common.member_type_id.kind();
      EXPECT_TRUE(tk == TI_STRONGLY_CONNECTED_COMPONENT ||
                  tk == TI_PLAIN_SEQUENCE_SMALL);
      if (tk == TI_STRONGLY_CONNECTED_COMPONENT) {
        check_struct_a_alias_member(member_seq[i].common, scc15, indexes);
        EXPECT_EQ(member_seq[i].detail.name, "a_bseq");
      } else {
        check_struct_a_sequence_member(member_seq[i].common);
        EXPECT_EQ(member_seq[i].detail.name, "a_boolseq");
      }
    }
  }
}

void check_struct_b_cseq_member(const CommonStructMember& common, const TypeMap& scc15)
{
  EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(0));
  EXPECT_EQ(common.member_flags, TRY_CONSTRUCT1);
  EXPECT_TRUE(scc15.find(common.member_type_id) != scc15.end());
}

void check_struct_b_eseq_member(const CommonStructMember& common, const TypeMap& scc15)
{
  EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(1));
  EXPECT_EQ(common.member_flags, TRY_CONSTRUCT1);
  EXPECT_TRUE(scc15.find(common.member_type_id) != scc15.end());
}

void check_struct_b_union_member(const CommonStructMember& common, const TypeMap& other)
{
  const TypeMap::const_iterator pos = other.find(common.member_type_id);
  EXPECT_TRUE(pos != other.end());
  const TypeKind tk = pos->second.kind == EK_MINIMAL ?
    pos->second.minimal.kind : pos->second.complete.kind;
  EXPECT_EQ(tk, TK_UNION);
}

void check_struct_b(const TypeMap& scc15, const TypeMap& other,
                    const TypeObject& to, const IndexMap& indexes, EquivalenceKind ek)
{
  const StructTypeFlag type_flags = ek == EK_MINIMAL ?
    to.minimal.struct_type.struct_flags : to.complete.struct_type.struct_flags;
  EXPECT_GT(type_flags | IS_FINAL, 0);

  if (ek == EK_MINIMAL) {
    EXPECT_EQ(to.minimal.struct_type.header.base_type, TypeIdentifier(TK_NONE));
    const MinimalStructMemberSeq& member_seq = to.minimal.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(3));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const TypeKind tk = member_seq[i].common.member_type_id.kind();
      EXPECT_TRUE(tk == TI_STRONGLY_CONNECTED_COMPONENT ||
                  tk == EK_MINIMAL);
      if (tk == TI_STRONGLY_CONNECTED_COMPONENT) {
        const ACE_CDR::Long idx = member_seq[i].common.member_type_id.sc_component_id().scc_index;
        const IndexMap::const_iterator pos = indexes.find(idx);
        ASSERT_TRUE(pos != indexes.end() && (pos->second == C_SEQ_ALIAS || pos->second == E_SEQ_ALIAS));
        NameHash nh;
        if (pos->second == C_SEQ_ALIAS) {
          check_struct_b_cseq_member(member_seq[i].common, scc15);
          hash_member_name(nh, "a_cseq");
        } else {
          check_struct_b_eseq_member(member_seq[i].common, scc15);
          hash_member_name(nh, "a_eseq");
        }
        EXPECT_EQ(std::memcmp(nh, member_seq[i].detail.name_hash, sizeof nh), 0);
      } else {
        check_struct_b_union_member(member_seq[i].common, other);
      }
    }
  } else {
    EXPECT_EQ(to.complete.struct_type.header.base_type, TypeIdentifier(TK_NONE));
    EXPECT_EQ(to.complete.struct_type.header.detail.type_name, "::B");
    const CompleteStructMemberSeq& member_seq = to.complete.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(3));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const TypeKind tk = member_seq[i].common.member_type_id.kind();
      EXPECT_TRUE(tk == TI_STRONGLY_CONNECTED_COMPONENT ||
                  tk == EK_COMPLETE);
      if (tk == TI_STRONGLY_CONNECTED_COMPONENT) {
        const ACE_CDR::Long idx = member_seq[i].common.member_type_id.sc_component_id().scc_index;
        const IndexMap::const_iterator pos = indexes.find(idx);
        EXPECT_TRUE(pos != indexes.end() && (pos->second == C_SEQ_ALIAS || pos->second == E_SEQ_ALIAS));
        if (pos->second == C_SEQ_ALIAS) {
          check_struct_b_cseq_member(member_seq[i].common, scc15);
          EXPECT_EQ(member_seq[i].detail.name, "a_cseq");
        } else {
          check_struct_b_eseq_member(member_seq[i].common, scc15);
          EXPECT_EQ(member_seq[i].detail.name, "a_eseq");
        }
      } else {
        check_struct_b_union_member(member_seq[i].common, other);
      }
    }
  }
}

void check_struct_c_dseq_member(const CommonStructMember& common, const TypeMap& scc15)
{
  EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(0));
  EXPECT_EQ(common.member_flags, TRY_CONSTRUCT1);
  EXPECT_TRUE(scc15.find(common.member_type_id) != scc15.end());
}

void check_struct_c_asequence_member(const CommonStructMember& common, const TypeMap& scc15)
{
  EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(1));
  EXPECT_EQ(common.member_flags, TRY_CONSTRUCT1);
  EXPECT_TRUE(scc15.find(common.member_type_id) != scc15.end());
}

void check_struct_c(const TypeMap& scc15, const TypeObject& to, const IndexMap& indexes, EquivalenceKind ek)
{
  const StructTypeFlag type_flags = ek == EK_MINIMAL ?
    to.minimal.struct_type.struct_flags : to.complete.struct_type.struct_flags;
  EXPECT_GT(type_flags | IS_APPENDABLE, 0);

  if (ek == EK_MINIMAL) {
    EXPECT_EQ(to.minimal.struct_type.header.base_type, TypeIdentifier(TK_NONE));
    const MinimalStructMemberSeq& member_seq = to.minimal.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(2));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const TypeKind tk = member_seq[i].common.member_type_id.kind();
      EXPECT_TRUE(tk == TI_STRONGLY_CONNECTED_COMPONENT);
      const ACE_CDR::Long idx = member_seq[i].common.member_type_id.sc_component_id().scc_index;
      const IndexMap::const_iterator pos = indexes.find(idx);
      EXPECT_TRUE(pos != indexes.end() && (pos->second == D_SEQ_ALIAS || pos->second == A_SEQ_ALIAS));
      NameHash nh;
      if (pos->second == D_SEQ_ALIAS) {
        check_struct_c_dseq_member(member_seq[i].common, scc15);
        hash_member_name(nh, "a_dseq");
      } else {
        check_struct_c_asequence_member(member_seq[i].common, scc15);
        hash_member_name(nh, "a_aseq");
      }
      EXPECT_EQ(std::memcmp(nh, member_seq[i].detail.name_hash, sizeof nh), 0);
    }
  } else {
    EXPECT_EQ(to.complete.struct_type.header.base_type, TypeIdentifier(TK_NONE));
    EXPECT_EQ(to.complete.struct_type.header.detail.type_name, "::C");
    const CompleteStructMemberSeq& member_seq = to.complete.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(2));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const TypeKind tk = member_seq[i].common.member_type_id.kind();
      EXPECT_TRUE(tk == TI_STRONGLY_CONNECTED_COMPONENT);
      const ACE_CDR::Long idx = member_seq[i].common.member_type_id.sc_component_id().scc_index;
      const IndexMap::const_iterator pos = indexes.find(idx);
      ASSERT_TRUE(pos != indexes.end() && (pos->second == D_SEQ_ALIAS || pos->second == A_SEQ_ALIAS));
      if (pos->second == D_SEQ_ALIAS) {
        check_struct_c_dseq_member(member_seq[i].common, scc15);
        EXPECT_EQ(member_seq[i].detail.name, "a_dseq");
      } else {
        check_struct_c_asequence_member(member_seq[i].common, scc15);
        EXPECT_EQ(member_seq[i].detail.name, "a_aseq");
      }
    }
  }
}

void check_struct_d_eseq_member(const CommonStructMember& common, const TypeMap& scc15, const IndexMap& indexes)
{
  EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(0));
  EXPECT_EQ(common.member_flags, TRY_CONSTRUCT1);
  EXPECT_TRUE(scc15.find(common.member_type_id) != scc15.end());
  const ACE_CDR::Long idx = common.member_type_id.sc_component_id().scc_index;
  const IndexMap::const_iterator pos = indexes.find(idx);
  EXPECT_TRUE(pos != indexes.end() && pos->second == E_SEQ_ALIAS);
}

void check_struct_d_sequence_member(const CommonStructMember& common)
{
  EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(1));
  EXPECT_EQ(common.member_flags, TRY_CONSTRUCT1);
  const PlainSequenceSElemDefn& seq_sdefn = common.member_type_id.seq_sdefn();
  EXPECT_EQ(seq_sdefn.header.equiv_kind, EK_BOTH);
  EXPECT_EQ(seq_sdefn.header.element_flags, TRY_CONSTRUCT1);
  EXPECT_EQ(seq_sdefn.bound, ACE_CDR::Octet(0));
  EXPECT_EQ(seq_sdefn.element_identifier->kind(), TK_BYTE);
}

void check_struct_d(const TypeMap& scc15, const TypeObject& to, const IndexMap& indexes, EquivalenceKind ek)
{
  const StructTypeFlag type_flags = ek == EK_MINIMAL ?
    to.minimal.struct_type.struct_flags : to.complete.struct_type.struct_flags;
  EXPECT_GT(type_flags | IS_APPENDABLE, 0);

  if (ek == EK_MINIMAL) {
    EXPECT_EQ(to.minimal.struct_type.header.base_type, TypeIdentifier(TK_NONE));
    const MinimalStructMemberSeq& member_seq = to.minimal.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(2));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const TypeKind tk = member_seq[i].common.member_type_id.kind();
      EXPECT_TRUE(tk == TI_STRONGLY_CONNECTED_COMPONENT || tk == TI_PLAIN_SEQUENCE_SMALL);
      NameHash nh;
      if (tk == TI_STRONGLY_CONNECTED_COMPONENT) {
        check_struct_d_eseq_member(member_seq[i].common, scc15, indexes);
        hash_member_name(nh, "a_eseq");
      } else {
        check_struct_d_sequence_member(member_seq[i].common);
        hash_member_name(nh, "a_octetseq");
      }
      EXPECT_EQ(std::memcmp(nh, member_seq[i].detail.name_hash, sizeof nh), 0);
    }
  } else {
    EXPECT_EQ(to.complete.struct_type.header.base_type, TypeIdentifier(TK_NONE));
    EXPECT_EQ(to.complete.struct_type.header.detail.type_name, "::D");
    const CompleteStructMemberSeq& member_seq = to.complete.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(2));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const TypeKind tk = member_seq[i].common.member_type_id.kind();
      EXPECT_TRUE(tk == TI_STRONGLY_CONNECTED_COMPONENT || tk == TI_PLAIN_SEQUENCE_SMALL);
      if (tk == TI_STRONGLY_CONNECTED_COMPONENT) {
        check_struct_d_eseq_member(member_seq[i].common, scc15, indexes);
        EXPECT_EQ(member_seq[i].detail.name, "a_eseq");
      } else {
        check_struct_d_sequence_member(member_seq[i].common);
        EXPECT_EQ(member_seq[i].detail.name, "a_octetseq");
      }
    }
  }
}

void check_struct_e_aseq_member(const CommonStructMember& common, const TypeMap& scc15, const IndexMap& scc15_index_map)
{
  EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(0));
  EXPECT_EQ(common.member_flags, TRY_CONSTRUCT1 | TRY_CONSTRUCT2);
  EXPECT_TRUE(scc15.find(common.member_type_id) != scc15.end());
  const ACE_CDR::Long idx = common.member_type_id.sc_component_id().scc_index;
  const IndexMap::const_iterator pos = scc15_index_map.find(idx);
  EXPECT_TRUE(pos != scc15_index_map.end() && pos->second == A_SEQ_ALIAS);
}

void check_struct_e_struct_member(const CommonStructMember& common, const IndexMap& scc12_index_map)
{
  EXPECT_EQ(common.member_id, static_cast<ACE_CDR::ULong>(1));
  EXPECT_EQ(common.member_flags, TRY_CONSTRUCT1);
  const ACE_CDR::Long idx = common.member_type_id.sc_component_id().scc_index;
  const IndexMap::const_iterator pos = scc12_index_map.find(idx);
  EXPECT_TRUE(pos != scc12_index_map.end() && pos->second == F);
}

void check_struct_e(const TypeMap& scc15, const TypeMap& scc12,
                    const TypeObject& to, const IndexMap& scc15_index_map,
                    const IndexMap& scc12_index_map, EquivalenceKind ek)
{
  const StructTypeFlag type_flags = ek == EK_MINIMAL ?
    to.minimal.struct_type.struct_flags : to.complete.struct_type.struct_flags;
  EXPECT_GT(type_flags | IS_MUTABLE, 0);

  if (ek == EK_MINIMAL) {
    EXPECT_EQ(to.minimal.struct_type.header.base_type, TypeIdentifier(TK_NONE));
    const MinimalStructMemberSeq& member_seq = to.minimal.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(2));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const TypeIdentifier& member_ti = member_seq[i].common.member_type_id;
      EXPECT_EQ(member_ti.kind(), TI_STRONGLY_CONNECTED_COMPONENT);
      EXPECT_TRUE(scc15.find(member_ti) != scc15.end() || scc12.find(member_ti) != scc12.end());
      NameHash nh;
      if (scc15.find(member_ti) != scc15.end()) {
        check_struct_e_aseq_member(member_seq[i].common, scc15, scc15_index_map);
        hash_member_name(nh, "a_aseq");
      } else {
        check_struct_e_struct_member(member_seq[i].common, scc12_index_map);
        hash_member_name(nh, "a_f");
      }
      EXPECT_EQ(std::memcmp(nh, member_seq[i].detail.name_hash, sizeof nh), 0);
    }
  } else {
    EXPECT_EQ(to.complete.struct_type.header.base_type, TypeIdentifier(TK_NONE));
    EXPECT_EQ(to.complete.struct_type.header.detail.type_name, "::E");
    const CompleteStructMemberSeq& member_seq = to.complete.struct_type.member_seq;
    EXPECT_EQ(member_seq.length(), static_cast<ACE_CDR::ULong>(2));

    for (ACE_CDR::ULong i = 0; i < member_seq.length(); ++i) {
      const TypeIdentifier& member_ti = member_seq[i].common.member_type_id;
      EXPECT_EQ(member_ti.kind(), TI_STRONGLY_CONNECTED_COMPONENT);
      EXPECT_TRUE(scc15.find(member_ti) != scc15.end() || scc12.find(member_ti) != scc12.end());
      if (scc15.find(member_ti) != scc15.end()) {
        check_struct_e_aseq_member(member_seq[i].common, scc15, scc15_index_map);
        EXPECT_EQ(member_seq[i].detail.name, "a_aseq");
      } else {
        check_struct_e_struct_member(member_seq[i].common, scc12_index_map);
        EXPECT_EQ(member_seq[i].detail.name, "a_f");
      }
    }
  }
}

void check_sequence(const TypeMap& scc15, const TypeObject& to, const IndexMap& scc15_index_map,
                    const TypeName& elem_type_name, EquivalenceKind ek)
{
  const ACE_CDR::ULong bound = ek == EK_MINIMAL ?
    to.minimal.sequence_type.header.common.bound : to.complete.sequence_type.header.common.bound;
  const CollectionElementFlag elem_flags = ek == EK_MINIMAL ?
    to.minimal.sequence_type.element.common.element_flags :
    to.complete.sequence_type.element.common.element_flags;
  const TypeIdentifier& elem_type = ek == EK_MINIMAL ?
    to.minimal.sequence_type.element.common.type : to.complete.sequence_type.element.common.type;

  EXPECT_EQ(bound, ACE_CDR::ULong(0));
  EXPECT_GT(elem_flags | TRY_CONSTRUCT1, 0);
  EXPECT_EQ(elem_type.kind(), TI_STRONGLY_CONNECTED_COMPONENT);
  EXPECT_TRUE(scc15.find(elem_type) != scc15.end());
  const ACE_CDR::Long elem_idx = elem_type.sc_component_id().scc_index;
  const IndexMap::const_iterator pos = scc15_index_map.find(elem_idx);
  EXPECT_TRUE(pos != scc15_index_map.end() && pos->second == elem_type_name);
}

void check_alias(const TypeMap& scc15, const TypeObject& to, const IndexMap& scc15_index_map,
                 const String type_name, const TypeName& base_type_name, EquivalenceKind ek)
{
  const TypeIdentifier& base_type = ek == EK_MINIMAL ?
    to.minimal.alias_type.body.common.related_type : to.complete.alias_type.body.common.related_type;
  EXPECT_EQ(base_type.kind(), TI_STRONGLY_CONNECTED_COMPONENT);
  EXPECT_TRUE(scc15.find(base_type) != scc15.end());
  const ACE_CDR::Long base_idx = base_type.sc_component_id().scc_index;
  const IndexMap::const_iterator pos = scc15_index_map.find(base_idx);
  EXPECT_TRUE(pos != scc15_index_map.end() && pos->second == base_type_name);

  if (ek == EK_COMPLETE) {
    EXPECT_EQ(to.complete.alias_type.header.detail.type_name, type_name);
  }
}

void check_scc15_map(const TypeMap& scc15, const TypeMap& scc12, const TypeMap& other,
                     const IndexMap& scc15_index_map, const IndexMap& scc12_index_map,
                     EquivalenceKind ek)
{
  typedef std::map<TypeName, ACE_CDR::Long> TypeToIndexMap;
  TypeToIndexMap to_index;
  for (IndexMap::const_iterator it = scc15_index_map.begin(); it != scc15_index_map.end(); ++it) {
    to_index[it->second] = it->first;
  }

  for (TypeMap::const_iterator it = scc15.begin(); it != scc15.end(); ++it) {
    const ACE_CDR::Long index = it->first.sc_component_id().scc_index;

    if (index == to_index[A]) {
      check_struct_a(scc15, it->second, scc15_index_map, ek);
    } else if (index == to_index[B]) {
      check_struct_b(scc15, other, it->second, scc15_index_map, ek);
    } else if (index == to_index[C]) {
      check_struct_c(scc15, it->second, scc15_index_map, ek);
    } else if (index == to_index[D]) {
      check_struct_d(scc15, it->second, scc15_index_map, ek);
    } else if (index == to_index[E]) {
      check_struct_e(scc15, scc12, it->second, scc15_index_map, scc12_index_map, ek);
    } else if (index == to_index[A_SEQ]) {
      check_sequence(scc15, it->second, scc15_index_map, A, ek);
    } else if (index == to_index[B_SEQ]) {
      check_sequence(scc15, it->second, scc15_index_map, B, ek);
    } else if (index == to_index[C_SEQ]) {
      check_sequence(scc15, it->second, scc15_index_map, C, ek);
    } else if (index == to_index[D_SEQ]) {
      check_sequence(scc15, it->second, scc15_index_map, D, ek);
    } else if (index == to_index[E_SEQ]) {
      check_sequence(scc15, it->second, scc15_index_map, E, ek);
    } else if (index == to_index[A_SEQ_ALIAS]) {
      check_alias(scc15, it->second, scc15_index_map, "::ASequence", A_SEQ, ek);
    } else if (index == to_index[B_SEQ_ALIAS]) {
      check_alias(scc15, it->second, scc15_index_map, "::BSeq", B_SEQ, ek);
    } else if (index == to_index[C_SEQ_ALIAS]) {
      check_alias(scc15, it->second, scc15_index_map, "::CSeq", C_SEQ, ek);
    } else if (index == to_index[D_SEQ_ALIAS]) {
      check_alias(scc15, it->second, scc15_index_map, "::DSeq", D_SEQ, ek);
    } else if (index == to_index[E_SEQ_ALIAS]) {
      check_alias(scc15, it->second, scc15_index_map, "::ESeq", E_SEQ, ek);
    }
  }
}

void check_scc12_map(const TypeMap& /*scc12*/, const IndexMap& /*indexes*/, EquivalenceKind /*ek*/)
{
  // TODO(sonndinh): Similar to check_scc15_map.
}

void verify_minimal_typemap(const TypeMap& minimal_map, const TypeMap& complete_map)
{
  TypeMap min_scc15, min_scc12, min_other;
  extract_scc(minimal_map, min_scc15, min_scc12, min_other);
  common_check_typemap(min_scc15, min_scc12, min_other, EK_MINIMAL);

  TypeMap com_scc15, com_scc12, com_other;
  extract_scc(complete_map, com_scc15, com_scc12, com_other);

  // It is not straight-forward to determine directly which type each entry in the
  // minimal type map is associated with. This goes an indirect way by first computing
  // the index of each type in its SCC using the complete type map, and then using
  // those indexes to determine the types in the minimal type map.
  IndexMap scc15_index_map = compute_scc15_index_map(com_scc15);
  IndexMap scc12_index_map = compute_scc12_index_map(com_scc12);
  check_scc15_map(min_scc15, min_scc12, min_other, scc15_index_map, scc12_index_map, EK_MINIMAL);
  check_scc12_map(min_scc12, scc12_index_map, EK_MINIMAL);
}

void verify_complete_typemap(const TypeMap& complete_map)
{
  TypeMap scc15, scc12, other;
  extract_scc(complete_map, scc15, scc12, other);
  common_check_typemap(scc15, scc12, other, EK_COMPLETE);

  IndexMap scc15_index_map = compute_scc15_index_map(scc15);
  IndexMap scc12_index_map = compute_scc12_index_map(scc12);
  check_scc15_map(scc15, scc12, other, scc15_index_map, scc12_index_map, EK_COMPLETE);
  check_scc12_map(scc12, scc12_index_map, EK_COMPLETE);
}

TEST(TypeMapTest, Minimal)
{
  const TypeMap& minimal_map = getMinimalTypeMap<A_xtag>();
  const TypeMap& complete_map = getCompleteTypeMap<A_xtag>();
  verify_minimal_typemap(minimal_map, complete_map);
}

TEST(TypeMapTest, Complete)
{
  const TypeMap& complete_map = getCompleteTypeMap<A_xtag>();
  verify_complete_typemap(complete_map);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
