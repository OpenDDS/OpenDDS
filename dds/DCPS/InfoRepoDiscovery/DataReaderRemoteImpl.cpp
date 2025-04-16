/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataReaderRemoteImpl.h"

#include "dds/DCPS/DataReaderCallbacks.h"
#include "dds/DCPS/debug.h"
#include "dds/DCPS/GuidConverter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

DataReaderRemoteImpl::DataReaderRemoteImpl(DataReaderCallbacks& parent)
  : parent_(parent)
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
}

void
DataReaderRemoteImpl::add_association(const WriterAssociation& writer,
                                      bool active)
{
  // the local copy of parent_ is necessary to prevent race condition
  RcHandle<DataReaderCallbacks> parent = parent_.lock();
  if (parent) {
    parent->add_association(writer, active);
  }
}

void
DataReaderRemoteImpl::remove_associations(const WriterIdSeq& writers,
                                          CORBA::Boolean notify_lost)
{
  // the local copy of parent_ is necessary to prevent race condition
  RcHandle<DataReaderCallbacks> parent = parent_.lock();
  if (parent) {
    parent->remove_associations(writers, notify_lost);
  }
}

void
DataReaderRemoteImpl::update_incompatible_qos(
  const IncompatibleQosStatus& status)
{
  // the local copy of parent_ is necessary to prevent race condition
  RcHandle<DataReaderCallbacks> parent = parent_.lock();
  if (parent) {
    parent->update_incompatible_qos(status);
  }
}

#if OPENDDS_CONFIG_OWNERSHIP_KIND_EXCLUSIVE
void
DataReaderRemoteImpl::update_ownership_strength(const GUID_t& publication_id,
                                                CORBA::Long strength)
{
  RcHandle<DataReaderCallbacks> parent = parent_.lock();
  if (parent) {
    parent->update_ownership_strength(publication_id, strength);
  }
}
#endif

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
