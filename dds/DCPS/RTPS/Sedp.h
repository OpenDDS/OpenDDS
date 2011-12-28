/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_SEDP_H
#define OPENDDS_RTPS_SEDP_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsInfoC.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"

#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/Definitions.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"

#include "Spdp.h"

#include <map>
#include <set>
#include <string>


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace RTPS {

class SedpTransportClient : public DCPS::TransportClient {
public:
  SedpTransportClient(const DCPS::RepoId& repo_id, DCPS::RcHandle<Spdp>& owner)
    : repo_id_(repo_id)
    , owner_(owner)
  {}

  virtual ~SedpTransportClient();

  // Implementing TransportClient
  bool check_transport_qos(const DCPS::TransportInst&)
    { return true; }
  const DCPS::RepoId& get_repo_id() const
    { return repo_id_; }
  CORBA::Long get_priority_value(const DCPS::AssociationData&) const
    { return 0; }

protected:
  using DCPS::TransportClient::enable_transport;
  using DCPS::TransportClient::disassociate;
  using DCPS::TransportClient::send;
  using DCPS::TransportClient::send_control;

  DCPS::RepoId repo_id_;
  DCPS::RcHandle<Spdp> owner_;
};

class SedpWriter : public DCPS::TransportSendListener,
                   public SedpTransportClient {
public:
  SedpWriter(const DCPS::RepoId& pub_id, DCPS::RcHandle<Spdp>& owner)
    : SedpTransportClient(pub_id, owner)
  {}

  virtual ~SedpWriter();

  bool assoc(const DCPS::AssociationData& subscription);

  // Implementing TransportSendListener
  void data_delivered(const DCPS::DataSampleListElement*);

  void data_dropped(const DCPS::DataSampleListElement*, bool by_transport);

  void control_delivered(ACE_Message_Block* /*sample*/);

  void control_dropped(ACE_Message_Block* /*sample*/,
                       bool /*dropped_by_transport*/);

  void notify_publication_disconnected(const DCPS::ReaderIdSeq&) {}
  void notify_publication_reconnected(const DCPS::ReaderIdSeq&) {}
  void notify_publication_lost(const DCPS::ReaderIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const DCPS::ReaderIdSeq&, bool) {}
  void retrieve_inline_qos_data(InlineQosData&) const {}
};

class SedpReader : public DCPS::TransportReceiveListener,
                   public SedpTransportClient {
public:
  SedpReader(const DCPS::RepoId& sub_id, DCPS::RcHandle<Spdp>& owner)
    : SedpTransportClient(sub_id, owner)
  {}

  virtual ~SedpReader();

  bool assoc(const DCPS::AssociationData& publication);

  // Implementing TransportReceiveListener

  void data_received(const DCPS::ReceivedDataSample& sample);

  void notify_subscription_disconnected(const DCPS::WriterIdSeq&) {}
  void notify_subscription_reconnected(const DCPS::WriterIdSeq&) {}
  void notify_subscription_lost(const DCPS::WriterIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const DCPS::WriterIdSeq&, bool) {}
};

}
}

#endif // OPENDDS_RTPS_SEDP_H

