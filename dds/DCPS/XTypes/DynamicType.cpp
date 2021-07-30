#include "DynamicType.h"

#include "MemberDescriptor.h"
#include "TypeDescriptor.h"

namespace OpenDDS {
namespace XTypes {

  DDS::ReturnCode_t DynamicType::get_descriptor(TypeDescriptor& descriptor)
  {
    descriptor = *descriptor_;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_member_by_name(DynamicTypeMember_rch& member, const OPENDDS_STRING& name)
  {
    //We have a map of members in member_by_name which takes a string and gives 0 or 1 type DTMs
    //this should use the string as the key and put the returned DTM into member.
    member = member_by_name.find(name)->second;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_all_members_by_name(DynamicTypeMembersByName& member)
  {
    member = member_by_name;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_member(DynamicTypeMember_rch& member, const MemberId& id)
  {
    member = member_by_id.find(id)->second;
    return DDS::RETCODE_OK;
  }
  DDS::ReturnCode_t DynamicType::get_all_members(DynamicTypeMembersById& member)
  {
    member = member_by_id;
    return DDS::RETCODE_OK;
  }
  unsigned long DynamicType::get_member_count()
  {
    return member_by_name.size();
  }
  DDS::ReturnCode_t DynamicType::get_member_by_index(DynamicTypeMember_rch& member, unsigned long index)
  {
    member = member_by_index[index];
  }

  bool DynamicType::equals(const DynamicType& other)
  {
    return (*this == other);
  }

  bool operator==(const DynamicType& lhs, const DynamicType& rhs)
  {
    if (lhs.descriptor_->kind == TK_ENUM && rhs.descriptor_->kind == TK_ENUM) {
      bool a = lhs.descriptor_->kind == rhs.descriptor_->kind;
      bool b = lhs.descriptor_->name == rhs.descriptor_->name;
      bool c = is_equivalent(lhs.descriptor_->base_type, rhs.descriptor_->base_type);
      bool d = is_equivalent(lhs.descriptor_->discriminator_type, rhs.descriptor_->discriminator_type);
      bool e = lhs.descriptor_->bound == rhs.descriptor_->bound;
      bool f = is_equivalent(lhs.descriptor_->element_type, rhs.descriptor_->element_type);
      bool g = is_equivalent(lhs.descriptor_->key_element_type, rhs.descriptor_->key_element_type);
      bool h = lhs.descriptor_->extensibility_kind == rhs.descriptor_->extensibility_kind;
      bool i = lhs.descriptor_->is_nested == rhs.descriptor_->is_nested;
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("TypeDescriptor: %b %b %b %b %b %b %b %b %b\n"),
      a, b, c, d, e, f, g, h, i));
      if (lhs.member_by_name.size() == rhs.member_by_name.size()) {
        if (lhs.member_by_name.size() > 0) {
          for (DynamicTypeMembersByName::const_iterator it = lhs.member_by_name.begin(); it != lhs.member_by_name.end(); ++it) {
            bool f =  (lhs.member_by_name.find(it->first)->second->descriptor_->name == rhs.member_by_name.find(it->first)->second->descriptor_->name);
            bool g =  (lhs.member_by_name.find(it->first)->second->descriptor_->id == rhs.member_by_name.find(it->first)->second->descriptor_->id);
            bool h =  (lhs.member_by_name.find(it->first)->second->descriptor_->default_value == rhs.member_by_name.find(it->first)->second->descriptor_->default_value);
            bool i =  (lhs.member_by_name.find(it->first)->second->descriptor_->index == rhs.member_by_name.find(it->first)->second->descriptor_->index);
            bool j =  (lhs.member_by_name.find(it->first)->second->descriptor_->label == rhs.member_by_name.find(it->first)->second->descriptor_->label);
            bool k =  (lhs.member_by_name.find(it->first)->second->descriptor_->try_construct_kind == rhs.member_by_name.find(it->first)->second->descriptor_->try_construct_kind);
            bool l =  (lhs.member_by_name.find(it->first)->second->descriptor_->is_key == rhs.member_by_name.find(it->first)->second->descriptor_->is_key);
            bool m =  (lhs.member_by_name.find(it->first)->second->descriptor_->is_optional == rhs.member_by_name.find(it->first)->second->descriptor_->is_optional);
            bool n =  (lhs.member_by_name.find(it->first)->second->descriptor_->is_must_understand == rhs.member_by_name.find(it->first)->second->descriptor_->is_must_understand);
            bool o =  (lhs.member_by_name.find(it->first)->second->descriptor_->is_shared == rhs.member_by_name.find(it->first)->second->descriptor_->is_shared);
            bool p =  (lhs.member_by_name.find(it->first)->second->descriptor_->is_default_label == rhs.member_by_name.find(it->first)->second->descriptor_->is_default_label);
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("MemberDescriptor: %b %b %b %b %b %b %b %b %b %b %b\n"), f, g, h, i, j, k, l, m, n, o, p));
            if (!(f && g && h && i && j && k && l && m && n && o && p)) {
              return false;
            }
          }
        }
      } else {
        return false;
      }
    } else {
      return
        *lhs.descriptor_ == *rhs.descriptor_ &&
        lhs.member_by_name == rhs.member_by_name  &&
        lhs.member_by_id == rhs.member_by_id;
    }
    return true;
  }

  bool is_equivalent(const DynamicType_rch& lhs, const DynamicType_rch& rhs)
  {
    if (lhs.in() == rhs.in()) {
      return true;
    } else if (lhs.in() == 0 || rhs.in() == 0) {
      return false;
    } else {
      return *lhs.in() == *rhs.in();
    }
  }

} // namespace XTypes
} // namespace OpenDDS
