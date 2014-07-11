/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataReaderRemoteImpl.h"
#include "dds/DCPS/DataReaderCallbacks.h"
#include "dds/DCPS/async_debug.h"

namespace OpenDDS {
namespace DCPS {

DataReaderRemoteImpl::DataReaderRemoteImpl(DataReaderCallbacks* parent) :
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

namespace
{
  DDS::DataReader_var getDataReader(DataReaderCallbacks* callbacks)
  {
    // the DataReaderCallbacks will always be a DataReader
    DDS::DataReader_var var =
      DDS::DataReader::_duplicate(dynamic_cast<DDS::DataReader*>(callbacks));
    return var;
  }
}

void
DataReaderRemoteImpl::add_association(const RepoId& yourId,
                                      const WriterAssociation& writer,
                                      bool active)
{
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:DataReaderRemoteImpl::add_association: enter method\n"));
  DataReaderCallbacks* parent = 0;
  DDS::DataReader_var drv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    drv = getDataReader(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->add_association(yourId, writer, active);
  }
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:DataReaderRemoteImpl::add_association: exit method\n"));
}

void
DataReaderRemoteImpl::association_complete(const RepoId& remote_id)
{
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:DataReaderRemoteImpl::association_complete: enter method\n"));
  DataReaderCallbacks* parent = 0;
  DDS::DataReader_var drv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    drv = getDataReader(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->association_complete(remote_id);
  }
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:DataReaderRemoteImpl::association_complete: exit method\n"));
}

void
DataReaderRemoteImpl::remove_associations(const WriterIdSeq& writers,
                                          CORBA::Boolean notify_lost)
{
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:DataReaderRemoteImpl::remove_associations: enter method\n"));
  DataReaderCallbacks* parent = 0;
  DDS::DataReader_var drv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    drv = getDataReader(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->remove_associations(writers, notify_lost);
  }
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:DataReaderRemoteImpl::remove_associations: exit method\n"));
}

void
DataReaderRemoteImpl::update_incompatible_qos(
  const IncompatibleQosStatus& status)
{
  DataReaderCallbacks* parent = 0;
  DDS::DataReader_var drv;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, this->mutex_);
    drv = getDataReader(this->parent_);
    parent = this->parent_;
  }
  if (parent) {
    parent->update_incompatible_qos(status);
  }
}

} // namespace DCPS
} // namespace OpenDDS
