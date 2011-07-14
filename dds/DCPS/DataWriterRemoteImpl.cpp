/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataWriterRemoteImpl.h"
#include "DataWriterImpl.h"

namespace OpenDDS {
namespace DCPS {

DataWriterRemoteImpl::DataWriterRemoteImpl(DataWriterImpl* parent)
  : parent_(parent)
{
}

// This method is called when there are no longer any reference to the
// the servant.
DataWriterRemoteImpl::~DataWriterRemoteImpl()
{
}

void
DataWriterRemoteImpl::detach_parent()
{
  ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
  this->parent_ = 0;
}

void
DataWriterRemoteImpl::add_association(const RepoId& yourId,
                                      const ReaderAssociation& reader)
ACE_THROW_SPEC((CORBA::SystemException))
{
  DataWriterImpl* parent = 0;
  DDS::DataWriter_var dwv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    dwv = DDS::DataWriter::_duplicate(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->add_association(yourId, reader);
  }
}

void
DataWriterRemoteImpl::remove_associations(const ReaderIdSeq& readers,
                                          CORBA::Boolean notify_lost)
ACE_THROW_SPEC((CORBA::SystemException))
{
  DataWriterImpl* parent = 0;
  DDS::DataWriter_var dwv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    dwv = DDS::DataWriter::_duplicate(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->remove_associations(readers, notify_lost);
  }
}

void
DataWriterRemoteImpl::update_incompatible_qos(
  const IncompatibleQosStatus& status)
ACE_THROW_SPEC((CORBA::SystemException))
{
  DataWriterImpl* parent = 0;
  DDS::DataWriter_var dwv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    dwv = DDS::DataWriter::_duplicate(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->update_incompatible_qos(status);
  }
}

void
DataWriterRemoteImpl::update_subscription_params(const RepoId& readerId,
                                                 const DDS::StringSeq& params)
ACE_THROW_SPEC((CORBA::SystemException))
{
  DataWriterImpl* parent = 0;
  DDS::DataWriter_var dwv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    dwv = DDS::DataWriter::_duplicate(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->update_subscription_params(readerId, params);
  }
}

} // namespace DCPS
} // namespace OpenDDS
