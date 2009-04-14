#include "test_helper.h"

#include <dds/DCPS/DataWriterImpl.h>
#include <dds/DCPS/DataReaderImpl.h>

OpenDDS::DCPS::DataWriterRemote_ptr
DDS_TEST::getRemoteInterface(const OpenDDS::DCPS::DataWriterImpl &impl)
{
  return OpenDDS::DCPS::DataWriterRemote::_duplicate (impl.dw_remote_objref_);
}

OpenDDS::DCPS::DataReaderRemote_ptr
DDS_TEST::getRemoteInterface(const OpenDDS::DCPS::DataReaderImpl &impl)
{
  return OpenDDS::DCPS::DataReaderRemote::_duplicate (impl.dr_remote_objref_);
}
