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

#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/Definitions.h"

#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/Marked_Default_Qos.h"


#include "RtpsMessageTypesC.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"


#include <map>
#include <set>
#include <string>


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace RTPS {

class SepdBuiltinPublicationsWriter : public DCPS::TransportSendListener,
                                      public DCPS::TransportClient
{
public:
  explicit SepdBuiltinPublicationsWriter(const DCPS::RepoId& pub_id)
    : pub_id_(pub_id), inline_qos_mode_(DEFAULT_QOS)
  {}

  virtual ~SepdBuiltinPublicationsWriter();

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
  bool check_transport_qos(const DCPS::TransportInst&);
  const DCPS::RepoId& get_repo_id() const;
  CORBA::Long get_priority_value(const DCPS::AssociationData&) const;


  using DCPS::TransportClient::enable_transport;
  using DCPS::TransportClient::disassociate;
  using DCPS::TransportClient::send;
  using DCPS::TransportClient::send_control;

  const DCPS::RepoId& pub_id_;
  DCPS::RepoId sub_id_;
  ssize_t callbacks_expected_;
  InlineQosMode inline_qos_mode_;

};

class SepdBuiltinPublicationsReader : public DCPS::TransportReceiveListener,
                                      public DCPS::TransportClient
{
public:
  explicit SepdBuiltinPublicationsReader(const DCPS::RepoId& sub_id)
    : done_(false)
    , sub_id_(sub_id)
    , control_msg_count_(0)
  {}

  virtual ~SepdBuiltinPublicationsReader() {}

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
};

class SepdBuiltinSubscriptionsWriter : public DCPS::TransportSendListener,
                                       public DCPS::TransportClient
{
public:
  explicit SepdBuiltinSubscriptionsWriter(const DCPS::RepoId& pub_id)
    : pub_id_(pub_id), inline_qos_mode_(DEFAULT_QOS)
  {}

  virtual ~SepdBuiltinSubscriptionsWriter();

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

};

class SepdBuiltinSubscriptionsReader : public DCPS::TransportReceiveListener,
                                       public DCPS::TransportClient
{
public:
  explicit SepdBuiltinSubscriptionsReader(const DCPS::RepoId& sub_id)
    : done_(false)
    , sub_id_(sub_id)
    , control_msg_count_(0)
  {}

  virtual ~SepdBuiltinSubscriptionsReader() {}

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
};

}
}

#endif // OPENDDS_RTPS_SEDP_H

