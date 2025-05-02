/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAWRITERCALLBACKS_H
#define OPENDDS_DCPS_DATAWRITERCALLBACKS_H

#include "EndpointCallbacks.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {
  class StringSeq;
}

namespace OpenDDS {
namespace DCPS {

class DiscoveryListener;
class ReaderIdSeq;
struct ReaderAssociation;

/**
* @class DataWriterCallbacks
*
* @brief Defines the interface for Discovery callbacks into the DataWriter.
*
*/
class OpenDDS_Dcps_Export DataWriterCallbacks
  : public EndpointCallbacks {
public:

  virtual void set_publication_id(const GUID_t& guid) = 0;

  virtual void add_association(const ReaderAssociation& reader,
                               bool active) = 0;

  virtual void remove_associations(const ReaderIdSeq& readers,
                                   bool callback) = 0;

  virtual void update_subscription_params(const GUID_t& readerId,
                                          const DDS::StringSeq& exprParams) = 0;

  virtual void register_for_reader(const GUID_t& /*participant*/,
                                   const GUID_t& /*writerid*/,
                                   const GUID_t& /*readerid*/,
                                   const TransportLocatorSeq& /*locators*/,
                                   DiscoveryListener* /*listener*/) { }

  virtual void unregister_for_reader(const GUID_t& /*participant*/,
                                     const GUID_t& /*writerid*/,
                                     const GUID_t& /*readerid*/) { }
};

typedef RcHandle<DataWriterCallbacks> DataWriterCallbacks_rch;
typedef WeakRcHandle<DataWriterCallbacks> DataWriterCallbacks_wrch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DATAWRITERCALLBACKS_H  */
