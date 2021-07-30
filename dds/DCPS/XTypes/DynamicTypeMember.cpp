#include "DynamicTypeMember.h"
#include "MemberDescriptor.h"

namespace OpenDDS {
namespace XTypes {

  DynamicTypeMember::DynamicTypeMember()
  : descriptor_(new MemberDescriptor())
  {
  }

  DynamicTypeMember::~DynamicTypeMember()
  {
    delete descriptor_;
  }

  DDS::ReturnCode_t DynamicTypeMember::get_descriptor(MemberDescriptor& descriptor)
  {
    descriptor = *descriptor_;
    return DDS::RETCODE_OK;
  }
  bool DynamicTypeMember::equals(const DynamicTypeMember& other)
  {
    return
      //TODO CLAYTON: I feel this is an incomplete implementation despite what the spec says
      this->descriptor_->type.in() == other.descriptor_->type.in();
  }
  MemberId DynamicTypeMember::get_id()
  {
    return descriptor_->id;
  }
  OPENDDS_STRING DynamicTypeMember::get_name()
  {
    return descriptor_->name;
  }

  bool operator==(const DynamicTypeMembersByName& lhs, const DynamicTypeMembersByName rhs)
  {
    if (lhs.size() == rhs.size()) {
      if (lhs.size() > 0) {
        for (DynamicTypeMembersByName::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
          if(rhs.find(it->first) != rhs.end()) {
            if (!(*lhs.find(it->first)->second->descriptor_ == *rhs.find(it->first)->second->descriptor_)) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersByName: Members are not the same \n")));
              return false;
            }
          } else {
            return false;
          }
        }
      }
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersByName: Member count is different \n")));
      return false;
    }
    return true;
  }

  bool operator==(const DynamicTypeMembersById& lhs, const DynamicTypeMembersById rhs)
  {
    if (lhs.size() == rhs.size()) {
      for (DynamicTypeMembersById::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
        if(rhs.find(it->first) != rhs.end()) {
          if (!(*lhs.find(it->first)->second->descriptor_ == *rhs.find(it->first)->second->descriptor_)) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersById: Members are not the same \n")));
            return false;
          }
        } else {
          return false;
        }
      }
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersById: Member count is different \n")));
      return false;
    }
    return true;
  }

} // namespace XTypes
} // namespace OpenDDS
