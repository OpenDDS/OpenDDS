/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataWriterRemoteImpl.h"
#include "dds/DCPS/DataWriterCallbacks.h"

namespace OpenDDS {
namespace DCPS {

DataWriterRemoteImpl::DataWriterRemoteImpl(DataWriterCallbacks* parent)
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

namespace
{
  DDS::DataWriter_var getDataWriter(DataWriterCallbacks* callbacks)
  {
    // the DataWriterCallbacks will always be a DataWriter
    DDS::DataWriter_var var =
      DDS::DataWriter::_duplicate(dynamic_cast<DDS::DataWriter*>(callbacks));
    return var;
  }
}

void
DataWriterRemoteImpl::add_association(const RepoId& yourId,
                                      const ReaderAssociation& reader,
                                      bool active)
{
  DataWriterCallbacks* parent = 0;
  DDS::DataWriter_var dwv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    dwv = getDataWriter(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->add_association(yourId, reader, active);
  }
}

void
DataWriterRemoteImpl::association_complete(const RepoId& remote_id)
{
  DataWriterCallbacks* parent = 0;
  DDS::DataWriter_var dwv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    dwv = getDataWriter(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->association_complete(remote_id);
  }
}

void
DataWriterRemoteImpl::remove_associations(const ReaderIdSeq& readers,
                                          CORBA::Boolean notify_lost)
{
  DataWriterCallbacks* parent = 0;
  DDS::DataWriter_var dwv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    dwv = getDataWriter(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->remove_associations(readers, notify_lost);
  }
}

void
DataWriterRemoteImpl::update_incompatible_qos(
  const IncompatibleQosStatus& status)
{
  DataWriterCallbacks* parent = 0;
  DDS::DataWriter_var dwv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    dwv = getDataWriter(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->update_incompatible_qos(status);
  }
}

void
DataWriterRemoteImpl::update_subscription_params(const RepoId& readerId,
                                                 const DDS::StringSeq& params)
{
  DataWriterCallbacks* parent = 0;
  DDS::DataWriter_var dwv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    dwv = getDataWriter(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->update_subscription_params(readerId, params);
  }
}

} // namespace DCPS
} // namespace OpenDDS
