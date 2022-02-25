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
DataReaderRemoteImpl::add_association(const RepoId& yourId,
                                      const WriterAssociation& writer,
                                      bool active)
{
  if (DCPS_debug_level) {
    GuidConverter writer_converter(yourId);
    GuidConverter reader_converter(writer.writerId);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderRemoteImpl::add_association - ")
               ACE_TEXT("local %C remote %C\n"),
               std::string(writer_converter).c_str(),
               std::string(reader_converter).c_str()));
  }

  // the local copy of parent_ is necessary to prevent race condition
  RcHandle<DataReaderCallbacks> parent = parent_.lock();
  if (parent.in()) {
    parent->add_association(yourId, writer, active);
  }
}

void
DataReaderRemoteImpl::remove_associations(const WriterIdSeq& writers,
                                          CORBA::Boolean notify_lost)
{
  // the local copy of parent_ is necessary to prevent race condition
  RcHandle<DataReaderCallbacks> parent = parent_.lock();
  if (parent.in()) {
    parent->remove_associations(writers, notify_lost);
  }
}

void
DataReaderRemoteImpl::update_incompatible_qos(
  const IncompatibleQosStatus& status)
{
  // the local copy of parent_ is necessary to prevent race condition
  RcHandle<DataReaderCallbacks> parent = parent_.lock();
  if (parent.in()) {
    parent->update_incompatible_qos(status);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
