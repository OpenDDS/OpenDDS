#include "test_helper.h"

#include <dds/DCPS/DataWriterImpl.h>

OpenDDS::DCPS::DataWriterRemote_ptr
DDS_TEST::getRemoteInterface(const OpenDDS::DCPS::DataWriterImpl &impl)
{
  return OpenDDS::DCPS::DataWriterRemote::_duplicate (impl.dw_remote_objref_);
}
