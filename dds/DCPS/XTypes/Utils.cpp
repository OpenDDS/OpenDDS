/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#include "Utils.h"

#include "TypeLookupService.h"

#include <dds/DCPS/debug.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

using DCPS::LogLevel;
using DCPS::log_level;

namespace { // helpers for XTypes::remove_enumerators (below)

  template <typename LiteralSeq>
  LiteralSeq subset(const LiteralSeq& in, const OPENDDS_SET(DDS::Int32)& toRemove)
  {
    // Modify the input LiteralSeq to have a subset of the original elements.
    // Elements that are removed result in enumerator names (the "detail" part) being
    // shifted up to earlier indexes, but the "common" part (integer value) doesn't change.
    LiteralSeq subset(in);
    for (DDS::UInt32 i = subset.length(); i > 0; --i) {
      if (toRemove.count(subset[i - 1].common.value)) {
        for (DDS::UInt32 j = i - 1; j < subset.length() - 1; ++j) {
          subset[j].detail = subset[j + 1].detail;
        }
        subset.length(subset.length() - 1);
      }
    }
    return subset;
  }

  TypeObject remove_enumerators(const TypeObject& to_enum_type,
                                const Sequence<DDS::Int32>& values)
  {
    const OPENDDS_SET(DDS::Int32) toRemove(values.begin(), values.end());
    TypeObject removed(to_enum_type);
    switch (removed.kind) {
    case EK_MINIMAL:
      if (removed.minimal.kind != TK_ENUM) {
        return TypeObject();
      }
      removed.minimal.enumerated_type.literal_seq =
        subset(removed.minimal.enumerated_type.literal_seq, toRemove);
      break;
    case EK_COMPLETE:
      if (removed.complete.kind != TK_ENUM) {
        return TypeObject();
      }
      removed.complete.enumerated_type.literal_seq =
        subset(removed.complete.enumerated_type.literal_seq, toRemove);
      break;
    default:
      return TypeObject();
    }
    return removed;
  }

  struct ReplaceEnums {
    const TypeIdentifier& enum_type_;
    const TypeIdentifier& modified_enum_;
    const TypeLookupService& lookup_;
    TypeMap& new_types_;

    ReplaceEnums(const TypeIdentifier& enum_type, const TypeIdentifier& modified_enum,
                 const TypeLookupService& lookup, TypeMap& new_types)
      : enum_type_(enum_type)
      , modified_enum_(modified_enum)
      , lookup_(lookup)
      , new_types_(new_types)
    {}

    bool replace_identifier(TypeIdentifier& id)
    {
      if (id == enum_type_) {
        id = modified_enum_;
        return true;
      }

      switch (id.kind()) {
      case TI_PLAIN_SEQUENCE_SMALL:
        return replace_identifier(*id.seq_sdefn().element_identifier);
      case TI_PLAIN_SEQUENCE_LARGE:
        return replace_identifier(*id.seq_ldefn().element_identifier);
      case TI_PLAIN_ARRAY_SMALL:
        return replace_identifier(*id.array_sdefn().element_identifier);
      case TI_PLAIN_ARRAY_LARGE:
        return replace_identifier(*id.array_ldefn().element_identifier);
      case TI_PLAIN_MAP_SMALL:
        return replace_identifier(*id.map_sdefn().key_identifier)
          | replace_identifier(*id.map_sdefn().element_identifier);
      case TI_PLAIN_MAP_LARGE:
        return replace_identifier(*id.map_ldefn().key_identifier)
          | replace_identifier(*id.map_ldefn().element_identifier);
      case EK_MINIMAL:
      case EK_COMPLETE: {
        const TypeIdentifier newId = replace_object(id);
        if (newId.kind() != TK_NONE) {
          id = newId;
          return true;
        }
        return false;
      }
      default:
        return false;
      }
    }

    template <typename MCST> // Minimal/Complete StructType
    bool replace_struct(MCST& mcst)
    {
      bool replaced = replace_identifier(mcst.header.base_type);
      for (DDS::UInt32 i = 0; i < mcst.member_seq.length(); ++i) {
        replaced |= replace_identifier(mcst.member_seq[i].common.member_type_id);
      }
      return replaced;
    }

    template <typename MCUT> // Minimal/Complete UnionType
    bool replace_union(MCUT& mcut)
    {
      bool replaced = replace_identifier(mcut.discriminator.common.type_id);
      for (DDS::UInt32 i = 0; i < mcut.member_seq.length(); ++i) {
        replaced |= replace_identifier(mcut.member_seq[i].common.type_id);
      }
      return replaced;
    }

    template <typename MCTO> // Minimal/Complete TypeObject
    bool replace_mcto(MCTO& mcto)
    {
      switch (mcto.kind) {
      case TK_ALIAS:
        return replace_identifier(mcto.alias_type.body.common.related_type);
      case TK_STRUCTURE:
        return replace_struct(mcto.struct_type);
      case TK_UNION:
        return replace_union(mcto.union_type);
      case TK_SEQUENCE:
        return replace_identifier(mcto.sequence_type.element.common.type);
      case TK_ARRAY:
        return replace_identifier(mcto.array_type.element.common.type);
      case TK_MAP:
        return replace_identifier(mcto.map_type.key.common.type)
          | replace_identifier(mcto.map_type.element.common.type);
      default:
        return false;
      }
    }

    TypeIdentifier replace_object(const TypeIdentifier& typeId)
    {
      TypeObject copy(lookup_.get_type_object(typeId));
      bool replaced = false;

      switch (copy.kind) {
      case EK_MINIMAL:
        replaced = replace_mcto(copy.minimal);
        break;
      case EK_COMPLETE:
        replaced = replace_mcto(copy.complete);
        break;
      default:
        return TypeIdentifier::None;
      }

      if (!replaced) {
        return TypeIdentifier::None;
      }

      const TypeIdentifier copyId = makeTypeIdentifier(copy);
      new_types_[copyId] = copy;
      return copyId;
    }
  };
}

TypeIdentifier remove_enumerators(const TypeIdentifier& top_level,
                                  const TypeIdentifier& enum_type,
                                  const Sequence<DDS::Int32>& values,
                                  const TypeLookupService& lookup,
                                  TypeMap& type_map,
                                  TypeIdentifier* modified_enum)
{
  const TypeObject& to_enum_type = lookup.get_type_object(enum_type);
  if (to_enum_type.kind == TK_NONE) {
    return TypeIdentifier::None;
  }

  const TypeObject to_modified_enum = remove_enumerators(to_enum_type, values);
  if (to_modified_enum.kind == TK_NONE) {
    return TypeIdentifier::None;
  }

  TypeIdentifier ti_modified_enum = makeTypeIdentifier(to_modified_enum);
  type_map[ti_modified_enum] = to_modified_enum;
  if (modified_enum) {
    *modified_enum = ti_modified_enum;
  }

  ReplaceEnums re(enum_type, ti_modified_enum, lookup, type_map);
  return re.replace_object(top_level);
}

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL
