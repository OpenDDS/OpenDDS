/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Sedp.h"
#include "Spdp.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesC.h"

#include "dds/DCPS/transport/framework/ReceivedDataSample.h"

#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/Marked_Default_Qos.h"

#include <cstdio>
#include <ctime>
#include <iostream>
#include <sstream>


namespace OpenDDS {
namespace RTPS {

//---------------------------------------------------------------

bool
SedpBuiltinPublicationsWriter::init(const DCPS::AssociationData& publication)
{
  sub_id_ = publication.remote_id_;
  return associate (publication, true);
}

void
SedpBuiltinPublicationsWriter::data_delivered(const DCPS::DataSampleListElement*)
{
}

void
SedpBuiltinPublicationsWriter::data_dropped(const DCPS::DataSampleListElement*, bool )
{
}

void
SedpBuiltinPublicationsWriter::control_delivered(ACE_Message_Block* )
{
}

void
SedpBuiltinPublicationsWriter::control_dropped(ACE_Message_Block* ,
                       bool )
{
}

void
SedpBuiltinPublicationsWriter::retrieve_inline_qos_data(InlineQosData& qos_data) const
{
  qos_data.dw_qos     = DATAWRITER_QOS_DEFAULT;
  qos_data.pub_qos    = PUBLISHER_QOS_DEFAULT;
  qos_data.topic_name = "SEDP Builtin Publications";
  switch (this->inline_qos_mode_) {
  case FULL_MOD_QOS:
    qos_data.pub_qos.presentation.access_scope = DDS::GROUP_PRESENTATION_QOS;
    qos_data.dw_qos.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
    qos_data.dw_qos.deadline.period.sec = 10;
    qos_data.dw_qos.latency_budget.duration.sec = 11;
    qos_data.dw_qos.ownership.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;
  case PARTIAL_MOD_QOS:
    qos_data.pub_qos.partition.name.length(1);
    qos_data.pub_qos.partition.name[0] = "Hello";
    qos_data.dw_qos.ownership_strength.value = 12;
    qos_data.dw_qos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    qos_data.dw_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    qos_data.dw_qos.transport_priority.value = 13;
    qos_data.dw_qos.lifespan.duration.sec = 14;
    qos_data.dw_qos.destination_order.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
  case DEFAULT_QOS:
    break;
  }
}

//-------------------------------------------------------------------------

bool
SedpBuiltinPublicationsReader::init(const DCPS::AssociationData& publication)
{
  pub_id_ = publication.remote_id_;
  return associate(publication, false);
}

  // Implementing TransportReceiveListener

void
SedpBuiltinPublicationsReader::data_received(const DCPS::ReceivedDataSample& sample)
{
  switch (sample.header_.message_id_) {
  case DCPS::SAMPLE_DATA: {
    DCPS::Serializer ser(sample.sample_,
                         sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                         DCPS::Serializer::ALIGN_CDR);
    bool ok = true;
    ACE_CDR::ULong encap;
    ok &= (ser >> encap); // read and ignore 32-bit CDR Encapsulation header
    DiscoveredWriterData data;
    ok &= (ser >> data);
    if (!ok) {
      ACE_DEBUG((LM_ERROR, "ERROR: failed to deserialize data\n"));
      return;
    }

    // so we have a new discoveredWriter, pair it up with any readers?
    owner_->add_discovered_writer (data);

    break;
  }
  case DCPS::INSTANCE_REGISTRATION:
  case DCPS::DISPOSE_INSTANCE:
  case DCPS::UNREGISTER_INSTANCE:
  case DCPS::DISPOSE_UNREGISTER_INSTANCE: {
    DCPS::Serializer ser(sample.sample_,
                         sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                         DCPS::Serializer::ALIGN_CDR);
    bool ok = true;
    ACE_CDR::ULong encap;
    ok &= (ser >> encap); // read and ignore 32-bit CDR Encapsulation header
    DiscoveredWriterData data;
    ok &= (ser >> data);

    if (!ok) {
      ACE_DEBUG((LM_ERROR, "ERROR: failed to deserialize key data\n"));
      return;
    }
    break;
  }
  }
}


//--------------------------------------------------------------------------


bool
SedpBuiltinSubscriptionsWriter::init(const DCPS::AssociationData& publication)
{
  sub_id_ = publication.remote_id_;
  return associate (publication, true);
}

void
SedpBuiltinSubscriptionsWriter::data_delivered(const DCPS::DataSampleListElement*)
{
}

void
SedpBuiltinSubscriptionsWriter::data_dropped(const DCPS::DataSampleListElement*, bool )
{
}

void
SedpBuiltinSubscriptionsWriter::control_delivered(ACE_Message_Block* )
{
}

void
SedpBuiltinSubscriptionsWriter::control_dropped(ACE_Message_Block* ,
                       bool )
{
}

void
SedpBuiltinSubscriptionsWriter::retrieve_inline_qos_data(InlineQosData& qos_data) const
{
  qos_data.dw_qos     = DATAWRITER_QOS_DEFAULT;
  qos_data.pub_qos    = PUBLISHER_QOS_DEFAULT;
  qos_data.topic_name = "SEDP Builtin Subscriptions";
  switch (this->inline_qos_mode_) {
  case FULL_MOD_QOS:
    qos_data.pub_qos.presentation.access_scope = DDS::GROUP_PRESENTATION_QOS;
    qos_data.dw_qos.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
    qos_data.dw_qos.deadline.period.sec = 10;
    qos_data.dw_qos.latency_budget.duration.sec = 11;
    qos_data.dw_qos.ownership.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;
  case PARTIAL_MOD_QOS:
    qos_data.pub_qos.partition.name.length(1);
    qos_data.pub_qos.partition.name[0] = "Hello";
    qos_data.dw_qos.ownership_strength.value = 12;
    qos_data.dw_qos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    qos_data.dw_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    qos_data.dw_qos.transport_priority.value = 13;
    qos_data.dw_qos.lifespan.duration.sec = 14;
    qos_data.dw_qos.destination_order.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
  case DEFAULT_QOS:
    break;
  }
}

//--------------------------------------------------------------------------


bool
SedpBuiltinSubscriptionsReader::init(const DCPS::AssociationData& publication)
{
  pub_id_ = publication.remote_id_;
  return associate(publication, false);
}

  // Implementing TransportReceiveListener

void
SedpBuiltinSubscriptionsReader::data_received(const DCPS::ReceivedDataSample& sample)
{
  switch (sample.header_.message_id_) {
  case DCPS::SAMPLE_DATA: {
    DCPS::Serializer ser(sample.sample_,
                         sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                         DCPS::Serializer::ALIGN_CDR);
    bool ok = true;
    ACE_CDR::ULong encap;
    ok &= (ser >> encap); // read and ignore 32-bit CDR Encapsulation header
    DiscoveredReaderData data;
    ok &= (ser >> data);

    if (!ok) {
      ACE_DEBUG((LM_ERROR, "ERROR: failed to deserialize data\n"));
      return;
    }

    // so we have a new discoveredReader, pair it up with any writerss?
    owner_->add_discovered_reader (data);
    break;
  }
  case DCPS::INSTANCE_REGISTRATION:
  case DCPS::DISPOSE_INSTANCE:
  case DCPS::UNREGISTER_INSTANCE:
  case DCPS::DISPOSE_UNREGISTER_INSTANCE: {
    DCPS::Serializer ser(sample.sample_,
                         sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                         DCPS::Serializer::ALIGN_CDR);
    bool ok = true;
    ACE_CDR::ULong encap;
    ok &= (ser >> encap); // read and ignore 32-bit CDR Encapsulation header
    DiscoveredReaderData data;
    ok &= (ser >> data);

    if (!ok) {
      ACE_DEBUG((LM_ERROR, "ERROR: failed to deserialize key data\n"));
      return;
    }
    break;
  }
  }
}


}
}
