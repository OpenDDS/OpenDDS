/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAREADERCALLBACKS_H
#define OPENDDS_DCPS_DATAREADERCALLBACKS_H

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/DiscoveryListener.h"
#include "dds/DCPS/RcObject.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace ICE {
  class Endpoint;
}

namespace DCPS {

/**
* @class DataReaderCallbacks
*
* @brief Defines the interface for Discovery callbacks into the DataReader.
*
*/
class DataReaderCallbacks
  : public virtual RcObject {
public:

  DataReaderCallbacks() {}

  virtual ~DataReaderCallbacks() {}

  virtual void add_association(const RepoId& yourId,
                               const WriterAssociation& writer,
                               bool active) = 0;

  virtual void association_complete(const RepoId& remote_id) = 0;

  virtual void remove_associations(const WriterIdSeq& writers,
                                   CORBA::Boolean callback) = 0;

  virtual void update_incompatible_qos(const IncompatibleQosStatus& status) = 0;

  virtual void signal_liveliness(const RepoId& remote_participant) = 0;

  virtual void register_for_writer(const RepoId& /*participant*/,
                                   const RepoId& /*readerid*/,
                                   const RepoId& /*writerid*/,
                                   const TransportLocatorSeq& /*locators*/,
                                   DiscoveryListener* /*listener*/) { }

  virtual void unregister_for_writer(const RepoId& /*participant*/,
                                     const RepoId& /*readerid*/,
                                     const RepoId& /*writerid*/) { }

  virtual void update_locators(const RepoId& /*remote*/,
                               const TransportLocatorSeq& /*locators*/) { }

  virtual ICE::Endpoint* get_ice_endpoint() = 0;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DATAREADERCALLBACKS_H  */
