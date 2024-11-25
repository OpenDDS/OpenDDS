/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAREADERCALLBACKS_H
#define OPENDDS_DCPS_DATAREADERCALLBACKS_H

#include "EndpointCallbacks.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DiscoveryListener;
class WriterIdSeq;
struct WriterAssociation;

/**
* @class DataReaderCallbacks
*
* @brief Defines the interface for Discovery callbacks into the DataReader.
*
*/
class OpenDDS_Dcps_Export DataReaderCallbacks
  : public EndpointCallbacks {
public:

  virtual void set_subscription_id(const GUID_t& guid) = 0;

  virtual void add_association(const WriterAssociation& writer,
                               bool active) = 0;

  virtual void remove_associations(const WriterIdSeq& writers,
                                   bool callback) = 0;

  virtual void signal_liveliness(const GUID_t& remote_participant) = 0;

  virtual void register_for_writer(const GUID_t& /*participant*/,
                                   const GUID_t& /*readerid*/,
                                   const GUID_t& /*writerid*/,
                                   const TransportLocatorSeq& /*locators*/,
                                   DiscoveryListener* /*listener*/) { }

  virtual void unregister_for_writer(const GUID_t& /*participant*/,
                                     const GUID_t& /*readerid*/,
                                     const GUID_t& /*writerid*/) { }
};

typedef RcHandle<DataReaderCallbacks> DataReaderCallbacks_rch;
typedef WeakRcHandle<DataReaderCallbacks> DataReaderCallbacks_wrch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DATAREADERCALLBACKS_H  */
