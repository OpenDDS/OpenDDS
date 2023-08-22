/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataWriterRemoteImpl.h"

#include "dds/DCPS/DataWriterCallbacks.h"
#include "dds/DCPS/debug.h"
#include "dds/DCPS/GuidConverter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

DataWriterRemoteImpl::DataWriterRemoteImpl(DataWriterCallbacks& parent)
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
}

void
DataWriterRemoteImpl::add_association(const ReaderAssociation& reader,
                                      bool active)
{
  // the local copy of parent_ is necessary to prevent race condition
  RcHandle<DataWriterCallbacks> parent = parent_.lock();
  if (parent.in()) {
    parent->add_association(reader, active);
  }
}

void
DataWriterRemoteImpl::remove_associations(const ReaderIdSeq& readers,
                                          CORBA::Boolean notify_lost)
{
  // the local copy of parent_ is necessary to prevent race condition
  RcHandle<DataWriterCallbacks> parent = parent_.lock();
  if (parent.in()) {
    parent->remove_associations(readers, notify_lost);
  }
}

void
DataWriterRemoteImpl::update_incompatible_qos(
  const IncompatibleQosStatus& status)
{
  // the local copy of parent_ is necessary to prevent race condition
  RcHandle<DataWriterCallbacks> parent = parent_.lock();
  if (parent.in()) {
    parent->update_incompatible_qos(status);
  }
}

void
DataWriterRemoteImpl::update_subscription_params(const GUID_t& readerId,
                                                 const DDS::StringSeq& params)
{
  // the local copy of parent_ is necessary to prevent race condition
  RcHandle<DataWriterCallbacks> parent = parent_.lock();
  if (parent.in()) {
    parent->update_subscription_params(readerId, params);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
