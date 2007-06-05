#include "test_helper.h"

#include <dds/DCPS/DataWriterImpl.h>
#include <dds/DCPS/DataReaderImpl.h>

TAO::DCPS::DataWriterRemote_ptr
DDS_TEST::getRemoteInterface(const TAO::DCPS::DataWriterImpl &impl)
{
  return TAO::DCPS::DataWriterRemote::_duplicate (impl.dw_remote_objref_);
}

TAO::DCPS::DataReaderRemote_ptr
DDS_TEST::getRemoteInterface(const TAO::DCPS::DataReaderImpl &impl)
{
  return TAO::DCPS::DataReaderRemote::_duplicate (impl.dr_remote_objref_);
}
