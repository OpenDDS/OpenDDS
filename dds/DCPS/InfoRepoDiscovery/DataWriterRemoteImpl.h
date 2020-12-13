/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INFOREPODISCOVERY_DATAWRITERREMOTEIMPL_H
#define OPENDDS_DCPS_INFOREPODISCOVERY_DATAWRITERREMOTEIMPL_H

#include "DataWriterRemoteS.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/DataWriterCallbacks.h"
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


/**
* @class DataWriterRemoteImpl
*
* @brief Implements the OpenDDS::DCPS::DataWriterRemote interface.
*
*/
class DataWriterRemoteImpl
  : public virtual POA_OpenDDS::DCPS::DataWriterRemote {
public:
  explicit DataWriterRemoteImpl(DataWriterCallbacks& parent);

  virtual ~DataWriterRemoteImpl();

  virtual void add_association(const RepoId& yourId,
                               const ReaderAssociation& readers,
                               bool active);

  virtual void remove_associations(const ReaderIdSeq& readers,
                                   CORBA::Boolean callback);

  virtual void update_incompatible_qos(const IncompatibleQosStatus& status);

  virtual void update_subscription_params(const RepoId& readerId,
                                          const DDS::StringSeq& exprParams);

  void detach_parent();

private:
  WeakRcHandle<DataWriterCallbacks> parent_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
