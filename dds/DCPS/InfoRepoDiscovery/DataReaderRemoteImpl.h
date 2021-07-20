/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INFOREPODISCOVERY_DATAREADERREMOTEIMPL_H
#define OPENDDS_DCPS_INFOREPODISCOVERY_DATAREADERREMOTEIMPL_H

#include "InfoRepoDiscovery_Export.h"
#include "DataReaderRemoteS.h"
#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/DataReaderCallbacks.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
* @class DataReaderRemoteImpl
*
* @brief Implements the OpenDDS::DCPS::ReaderRemote interface that
*        is used to add and remove associations.
*
*/
class DataReaderRemoteImpl
  : public virtual POA_OpenDDS::DCPS::DataReaderRemote {
public:

  explicit DataReaderRemoteImpl(DataReaderCallbacks& parent);

  virtual ~DataReaderRemoteImpl();

  virtual void add_association(const RepoId& yourId,
                               const WriterAssociation& writer,
                               bool active);

  virtual void remove_associations(const WriterIdSeq& writers,
                                   CORBA::Boolean callback);

  virtual void update_incompatible_qos(const IncompatibleQosStatus& status);

  void detach_parent();

private:
  WeakRcHandle<DataReaderCallbacks> parent_;
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_DATAREADERREMOTEIMPL_H  */
