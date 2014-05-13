/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataWriterRemoteImpl.h"
#include "dds/DCPS/DataWriterCallbacks.h"
#include "dds/DCPS/GuidConverter.h"

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
   //### Debug statements to track where associate is failing
  GuidConverter yourID_converted(yourId);
  GuidConverter remoteID_converted(reader.readerId);
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataWriterRemoteImpl::add_association: enter method yourID: %C remoteID: %C\n", std::string(yourID_converted).c_str(), std::string(remoteID_converted).c_str()));

  DataWriterCallbacks* parent = 0;
  DDS::DataWriter_var dwv;
  {
     //### Debug statements to track where associate is failing
     ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataWriterRemoteImpl::add_association: about to guard mutex_ method\n"));
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    //### Debug statements to track where associate is failing
    ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataWriterRemoteImpl::add_association: mutex_ LOCKED to getDataWriter\n"));
    dwv = getDataWriter(this->parent_);
    //### Debug statements to track where associate is failing
    ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataWriterRemoteImpl::add_association: got dataWriter assigning to parent\n"));
    parent = this->parent_;
    //### Debug statements to track where associate is failing
    ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataWriterRemoteImpl::add_association: mutex_ RELEASED\n"));
  }
  if (parent) {
     //### Debug statements to track where associate is failing
     ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataWriterRemoteImpl::add_association: have parent, add_association\n"));
    parent->add_association(yourId, reader, active);
    //### Debug statements to track where associate is failing
    ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataWriterRemoteImpl::add_association: parent done with add_association\n"));
  }

  //### Debug statements to track where associate is failing
  ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataWriterRemoteImpl::add_association: exit method\n"));
}

void
DataWriterRemoteImpl::association_complete(const RepoId& remote_id)
{

   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataWriterRemoteImpl::association_complete: enter method\n"));
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

  //### Debug statements to track where associate is failing
  ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###DataWriterRemoteImpl::association_complete: exit method\n"));
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
