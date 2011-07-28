/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataReaderRemoteImpl.h"
#include "DataReaderImpl.h"

namespace OpenDDS {
namespace DCPS {

DataReaderRemoteImpl::DataReaderRemoteImpl(DataReaderImpl* parent) :
    parent_(parent)
{
}

// This method is called when there are no longer any reference to the
// the servant.
DataReaderRemoteImpl::~DataReaderRemoteImpl()
{
}

void
DataReaderRemoteImpl::detach_parent()
{
  ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
  this->parent_ = 0;
}

void
DataReaderRemoteImpl::add_association(const RepoId& yourId,
                                      const WriterAssociation& writer,
                                      bool active)
ACE_THROW_SPEC((CORBA::SystemException))
{
  DataReaderImpl* parent = 0;
  DDS::DataReader_var drv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    drv = DDS::DataReader::_duplicate(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->add_association(yourId, writer, active);
  }
}

void
DataReaderRemoteImpl::remove_associations(const WriterIdSeq& writers,
                                          CORBA::Boolean notify_lost)
ACE_THROW_SPEC((CORBA::SystemException))
{
  DataReaderImpl* parent = 0;
  DDS::DataReader_var drv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    drv = DDS::DataReader::_duplicate(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->remove_associations(writers, notify_lost);
  }
}

void
DataReaderRemoteImpl::update_incompatible_qos(
  const IncompatibleQosStatus& status)
ACE_THROW_SPEC((CORBA::SystemException))
{
  DataReaderImpl* parent = 0;
  DDS::DataReader_var drv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    drv = DDS::DataReader::_duplicate(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->update_incompatible_qos(status);
  }
}

} // namespace DCPS
} // namespace OpenDDS
