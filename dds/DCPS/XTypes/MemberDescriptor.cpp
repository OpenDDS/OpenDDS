#include <dds/DCPS/XTypes/MemberDescriptor.h>

namespace OpenDDS {
namespace XTypes {
  bool MemberDescriptor::equals(const MemberDescriptor& descriptor) {
    return (*this == descriptor);
  }
} // namespace XTypes
} // namespace OpenDDS