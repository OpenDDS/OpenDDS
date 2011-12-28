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

class SedpBuiltinPublicationsWriter : public DCPS::TransportSendListener,
                                      public DCPS::TransportClient
{
public:
  explicit SedpBuiltinPublicationsWriter(const DCPS::RepoId& pub_id,
                                         DCPS::RcHandle<Spdp> &owner)
    : pub_id_(pub_id),
      inline_qos_mode_(DEFAULT_QOS),
      owner_(owner)
  {}

  virtual ~SedpBuiltinPublicationsWriter();

  bool init(const DCPS::AssociationData& publication);

  // Implementing TransportSendListener
  void data_delivered(const DCPS::DataSampleListElement*);

  void data_dropped(const DCPS::DataSampleListElement*, bool by_transport);

  void control_delivered(ACE_Message_Block* /*sample*/);

  void control_dropped(ACE_Message_Block* /*sample*/,
                       bool /*dropped_by_transport*/);

  // Enum to define qos returned by this object when populating inline qos
  // This will determine which qos policies are placed in the submessage.
  enum InlineQosMode {
    DEFAULT_QOS,       // Use the default values for all pub and dw qos
    PARTIAL_MOD_QOS,   // Modify some (but not all) qos values
    FULL_MOD_QOS       // Modify all qos values.
  };

  void notify_publication_disconnected(const DCPS::ReaderIdSeq&) {}
  void notify_publication_reconnected(const DCPS::ReaderIdSeq&) {}
  void notify_publication_lost(const DCPS::ReaderIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const DCPS::ReaderIdSeq&, bool) {}
  virtual void retrieve_inline_qos_data(InlineQosData& qos_data) const;

  // Implementing TransportClient
  bool check_transport_qos(const DCPS::TransportInst&)
    { return true; }
  const DCPS::RepoId& get_repo_id() const
    { return sub_id_; }
  CORBA::Long get_priority_value(const DCPS::AssociationData&) const
    { return 0; }

  using DCPS::TransportClient::enable_transport;
  using DCPS::TransportClient::disassociate;
  using DCPS::TransportClient::send;
  using DCPS::TransportClient::send_control;

  const DCPS::RepoId& pub_id_;
  DCPS::RepoId sub_id_;
  ssize_t callbacks_expected_;
  InlineQosMode inline_qos_mode_;

  DCPS::RcHandle<Spdp> owner_;

};

class SedpBuiltinPublicationsReader : public DCPS::TransportReceiveListener,
                                      public DCPS::TransportClient
{
public:
  explicit SedpBuiltinPublicationsReader(const DCPS::RepoId& sub_id,
                                         DCPS::RcHandle<Spdp> &owner)
    : done_(false)
    , sub_id_(sub_id)
    , control_msg_count_(0)
    , owner_(owner)
  {}

  virtual ~SedpBuiltinPublicationsReader() {}

  bool init(const DCPS::AssociationData& publication);

  // Implementing TransportReceiveListener

  void data_received(const DCPS::ReceivedDataSample& sample);

  void notify_subscription_disconnected(const DCPS::WriterIdSeq&) {}
  void notify_subscription_reconnected(const DCPS::WriterIdSeq&) {}
  void notify_subscription_lost(const DCPS::WriterIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const DCPS::WriterIdSeq&, bool) {}

  // Implementing TransportClient
  bool check_transport_qos(const DCPS::TransportInst&)
    { return true; }
  const DCPS::RepoId& get_repo_id() const
    { return sub_id_; }
  CORBA::Long get_priority_value(const DCPS::AssociationData&) const
    { return 0; }

  using DCPS::TransportClient::enable_transport;
  using DCPS::TransportClient::disassociate;

  bool done_;
  const DCPS::RepoId& sub_id_;
  DCPS::RepoId pub_id_;
  DCPS::SequenceNumber seq_;
  int control_msg_count_;
  DCPS::RcHandle<Spdp> owner_;
};

class SedpBuiltinSubscriptionsWriter : public DCPS::TransportSendListener,
                                       public DCPS::TransportClient
{
public:
  explicit SedpBuiltinSubscriptionsWriter(const DCPS::RepoId& pub_id,
                                          DCPS::RcHandle<Spdp> &owner)
    : pub_id_(pub_id),
      inline_qos_mode_(DEFAULT_QOS),
      owner_(owner)
  {}

  virtual ~SedpBuiltinSubscriptionsWriter();

  bool init(const DCPS::AssociationData& publication);

  // Implementing TransportSendListener
  void data_delivered(const DCPS::DataSampleListElement*);

  void data_dropped(const DCPS::DataSampleListElement*, bool by_transport);

  void control_delivered(ACE_Message_Block* /*sample*/);

  void control_dropped(ACE_Message_Block* /*sample*/,
                       bool /*dropped_by_transport*/);

  // Enum to define qos returned by this object when populating inline qos
  // This will determine which qos policies are placed in the submessage.
  enum InlineQosMode {
    DEFAULT_QOS,       // Use the default values for all pub and dw qos
    PARTIAL_MOD_QOS,   // Modify some (but not all) qos values
    FULL_MOD_QOS       // Modify all qos values.
  };

  void notify_publication_disconnected(const DCPS::ReaderIdSeq&) {}
  void notify_publication_reconnected(const DCPS::ReaderIdSeq&) {}
  void notify_publication_lost(const DCPS::ReaderIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const DCPS::ReaderIdSeq&, bool) {}
  virtual void retrieve_inline_qos_data(InlineQosData& qos_data) const;

  // Implementing TransportClient
  bool check_transport_qos(const DCPS::TransportInst&)
    { return true; }
  const DCPS::RepoId& get_repo_id() const
    { return sub_id_; }
  CORBA::Long get_priority_value(const DCPS::AssociationData&) const
    { return 0; }


  using DCPS::TransportClient::enable_transport;
  using DCPS::TransportClient::disassociate;
  using DCPS::TransportClient::send;
  using DCPS::TransportClient::send_control;

  const DCPS::RepoId& pub_id_;
  DCPS::RepoId sub_id_;
  ssize_t callbacks_expected_;
  InlineQosMode inline_qos_mode_;
  DCPS::RcHandle<Spdp> owner_;
};

class SedpBuiltinSubscriptionsReader : public DCPS::TransportReceiveListener,
                                       public DCPS::TransportClient
{
public:
  explicit SedpBuiltinSubscriptionsReader(const DCPS::RepoId& sub_id,
                                          DCPS::RcHandle<Spdp> &owner)
    : done_(false),
      sub_id_(sub_id),
      control_msg_count_(0),
      owner_(owner)
  {}

  virtual ~SedpBuiltinSubscriptionsReader() {}

  bool init(const DCPS::AssociationData& publication);

  // Implementing TransportReceiveListener

  void data_received(const DCPS::ReceivedDataSample& sample);

  void notify_subscription_disconnected(const DCPS::WriterIdSeq&) {}
  void notify_subscription_reconnected(const DCPS::WriterIdSeq&) {}
  void notify_subscription_lost(const DCPS::WriterIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const DCPS::WriterIdSeq&, bool) {}

  // Implementing TransportClient
  bool check_transport_qos(const DCPS::TransportInst&)
    { return true; }
  const DCPS::RepoId& get_repo_id() const
    { return sub_id_; }
  CORBA::Long get_priority_value(const DCPS::AssociationData&) const
    { return 0; }

  using DCPS::TransportClient::enable_transport;
  using DCPS::TransportClient::disassociate;

  bool done_;
  const DCPS::RepoId& sub_id_;
  DCPS::RepoId pub_id_;
  DCPS::SequenceNumber seq_;
  int control_msg_count_;
  DCPS::RcHandle<Spdp> owner_;
};

}
}

#endif // OPENDDS_RTPS_SEDP_H

