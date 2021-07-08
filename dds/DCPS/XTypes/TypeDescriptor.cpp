#include <dds/DCPS/XTypes/TypeDescriptor.h>

namespace OpenDDS {
namespace XTypes {

  bool TypeDescriptor::equals(const TypeDescriptor& other) {
    return (*this == other);
  }

} // namespace XTypes
} // namespace OpenDDS
