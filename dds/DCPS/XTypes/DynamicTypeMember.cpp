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
    return true;
    //TODO CLAYTON
    //  this->descriptor_->type.in() == other.descriptor_->type.in();
  }
  MemberId DynamicTypeMember::get_id()
  {
    return descriptor_->id;
  }
  OPENDDS_STRING DynamicTypeMember::get_name()
  {
    return descriptor_->name;
  }

  bool test_equality(const DynamicTypeMembersByName& lhs, const DynamicTypeMembersByName& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
  {
    if (lhs.size() == rhs.size()) {
      if (lhs.size() > 0) {
        for (DynamicTypeMembersByName::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
          if(rhs.find(it->first) != rhs.end()) {
            if (!(test_equality(*lhs.find(it->first)->second->descriptor_, *rhs.find(it->first)->second->descriptor_, dt_ptr_pair))) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersByName: Members are not the same\n")));
              return false;
            }
          } else {
            return false;
          }
        }
      }
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersByName: Member count is different\n")));
      return false;
    }
    return true;
  }

  bool test_equality(const DynamicTypeMembersById& lhs, const DynamicTypeMembersById& rhs, DynamicTypePtrPairSeen& dt_ptr_pair)
  {
    if (lhs.size() == rhs.size()) {
      for (DynamicTypeMembersById::const_iterator it = lhs.begin(); it != lhs.end(); ++it) {
        if(rhs.find(it->first) != rhs.end()) {
          if (!(test_equality(*lhs.find(it->first)->second->descriptor_, *rhs.find(it->first)->second->descriptor_, dt_ptr_pair))) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersById: Members are not the same\n")));
            return false;
          }
        } else {
          return false;
        }
      }
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("DynamicTypeMembersById: Member count is different\n")));
      return false;
    }
    return true;
  }

} // namespace XTypes
} // namespace OpenDDS
