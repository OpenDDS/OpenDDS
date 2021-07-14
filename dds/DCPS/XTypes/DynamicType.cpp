#include <dds/DCPS/XTypes/DynamicType.h>
namespace OpenDDS {
namespace XTypes {

  DDS::ReturnCode_t DynamicType::get_descriptor(TypeDescriptor& descriptor) {
    
  }
  DDS::ReturnCode_t DynamicType::get_member_by_name(DynamicTypeMember_rch& member, const OPENDDS_STRING& name) {
    //We have a map of members in member_by_name which takes a string and gives 0 or 1 type DTMs
    //this should use the string as the key and put the returned DTM into member.
    member = member_by_name.find(name)->second;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_all_members_by_name(DynamicTypeMembersByName& member) {
    member = member_by_name;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_member(DynamicTypeMember_rch& member, const MemberId& id) {
    member = member_by_id.find(id)->second;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_all_members(DynamicTypeMembersById& member) {
    member = member_by_id;
    return DDS::RETCODE_OK;
  }
  unsigned long DynamicType::get_member_count() {
    return member_by_name.size();
  }
  DDS::ReturnCode_t DynamicType::get_member_by_index(DynamicTypeMember_rch& member, unsigned long index) {
    member = member_by_index[index];
  }
  bool DynamicType::equals(const DynamicType& other) {
    return (*this == other);
  }

  DDS::ReturnCode_t complete_struct_member_to_member_descriptor(MemberDescriptor*& md,
    const CompleteStructMember& cm) {
    md->name = cm.detail.name;
    md->id = cm.common.member_id;
    md->default_value = ""; //TODO CLAYTON: Where do we get the default value from CompleteStructMember?
    md->label.length(0);
    if (cm.common.member_flags & (1 << 0)) {
      if (cm.common.member_flags & (1 << 1)) {
        md->try_construct_kind = TRIM;
      } else {
        md->try_construct_kind = DISCARD;
      }
    } else {
      if (cm.common.member_flags & (1 << 1)) {
        md->try_construct_kind = USE_DEFAULT;
      } else {
        ACE_ERROR((LM_ERROR, ACE_TEXT("Invalid TryConstruct Kind in complete_struct_member_to_member_descriptor\n")));
      }
    }
    md->is_key = (cm.common.member_flags & (1 << 5));
    md->is_optional = (cm.common.member_flags & (1 << 3));
    md->is_must_understand = (cm.common.member_flags & (1 << 4));
    md->is_shared = (cm.common.member_flags & (1 << 2));
    md->is_default_label = 0;
  }

  DDS::ReturnCode_t complete_struct_member_to_dynamic_type_member(DynamicTypeMember_rch& dtm,
    const CompleteStructMember& cm) {
    complete_struct_member_to_member_descriptor(dtm.in()->descriptor_, cm);
  }

  DDS::ReturnCode_t complete_to_dynamic(DynamicType& dt, const CompleteTypeObject& cto) {
    switch (cto.kind) {
    case TK_STRUCTURE:
      dt.descriptor_->kind = TK_STRUCTURE;
      dt.descriptor_->name = cto.struct_type.header.detail.type_name;
      //TODO:CLAYTON
      //dt.descriptor_->base_type = 
      //cto.struct_type.header.base_type gets us a TypeIdentifier which we need to use to get a CompleteTypeObject
      // We would then call complete_to_dynamic using the resultant CompleteTypeObject to get a DynamicType which
      // is our base_type
      // additionally we need to figure out where we are storing our dynamic types once created so that if we have
      // already created a DynamicType for it, we can just grab it instead.
      //dt.descriptor_->discriminator_type = nil
      dt.descriptor_->bound.length(0);
      //dt.descriptor_->element_type =  nil
      //dt.descriptor_->key_element_type =  nil
      if (cto.struct_type.struct_flags & (1 << 0)) {
        dt.descriptor_->extensibility_kind = FINAL;
      } else if (cto.struct_type.struct_flags & (1 << 1)) {
        dt.descriptor_->extensibility_kind = APPENDABLE;
      } else if (cto.struct_type.struct_flags & (1 << 2)) {
        dt.descriptor_->extensibility_kind = MUTABLE;
      } else {
        ACE_ERROR((LM_ERROR, ACE_TEXT("Invalid extensibility kind in complete_to_dynamic(DynamicType& dt, const CompleteTypeObject& cto)\n")));
      }
      dt.descriptor_->is_nested = (cto.struct_type.struct_flags & (1 << 3));
      for (ulong i = 0; i < cto.struct_type.member_seq.length(); ++i) {
        DynamicTypeMember_rch dtm;
        complete_struct_member_to_dynamic_type_member(dtm, cto.struct_type.member_seq[i]);
        dt.member_by_index.insert(dt.member_by_index.end(), dtm); //insert at end?
      }
      break;
    case TK_UNION:
      break;
    case TK_ALIAS:
      break;
    case TK_ARRAY:
      break;
    case TK_SEQUENCE:
      break;
   }
  }

} // namespace XTypes
} // namespace OpenDDS
