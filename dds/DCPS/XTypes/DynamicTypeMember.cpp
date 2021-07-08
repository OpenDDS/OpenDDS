#include "DynamicTypeMember.h"
#include "MemberDescriptor.h"

namespace OpenDDS {
namespace XTypes {

  DynamicTypeMember::DynamicTypeMember()
  : descriptor_(new MemberDescriptor())
  {
  }

  DynamicTypeMember::~DynamicTypeMember() {
    delete descriptor_;
  }

  DDS::ReturnCode_t DynamicTypeMember::get_descriptor(MemberDescriptor& descriptor) {
    descriptor = *descriptor_;
    return DDS::RETCODE_OK;
  }
  bool DynamicTypeMember::equals(const DynamicTypeMember& other) {
    return
      //TODO CLAYTON: I feel this is an incomplete implementation despite what the spec says
      this->descriptor_->type.in() == other.descriptor_->type.in();
  }
  MemberId DynamicTypeMember::get_id() {
    return descriptor_->id;
  }
  OPENDDS_STRING DynamicTypeMember::get_name() {
    return descriptor_->name;
  }

} // namespace XTypes
} // namespace OpenDDS
