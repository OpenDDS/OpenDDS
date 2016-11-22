/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "dds/DCPS/Service_Participant.h"
#include "TransportSendListener.h"
#include "EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TransportSendListener::TransportSendListener()
{
  DBG_ENTRY_LVL("TransportSendListener","TransportSendListener",6);
}

TransportSendListener::~TransportSendListener()
{
  DBG_ENTRY_LVL("TransportSendListener","~TransportSendListener",6);
}

void
TransportSendListener::data_delivered(const DataSampleElement* sample)
{
  ACE_UNUSED_ARG(sample);
  ACE_ERROR((LM_ERROR,
             "(%P|%t) ERROR: Subclass should override if sending data samples.\n"));
}

void
TransportSendListener::data_dropped(const DataSampleElement* sample,
                                                   bool dropped_by_transport)
{
  ACE_UNUSED_ARG(sample);
  ACE_UNUSED_ARG(dropped_by_transport);
  ACE_ERROR((LM_ERROR,
             "(%P|%t) ERROR: Subclass should override if sending data samples.\n"));
}

void
TransportSendListener::control_delivered(ACE_Message_Block* sample)
{
  ACE_UNUSED_ARG(sample);
  ACE_ERROR((LM_ERROR,
             "(%P|%t) ERROR: Subclass should override if sending control samples.\n"));
}

void
TransportSendListener::control_dropped(ACE_Message_Block* sample,
                                                      bool dropped_by_transport)
{
  ACE_UNUSED_ARG(sample);
  ACE_UNUSED_ARG(dropped_by_transport);
  ACE_ERROR((LM_ERROR,
             "(%P|%t) ERROR: Subclass should override if sending control samples.\n"));
}

SendControlStatus
TransportSendListener::send_control_customized(const DataLinkSet_rch&,
                                               const DataSampleHeader&,
                                               ACE_Message_Block*, void*)
{
  return SEND_CONTROL_OK;
}

void
TransportSendListener::retrieve_inline_qos_data(InlineQosData& qos_data) const
{
  qos_data.dw_qos     = TheServiceParticipant->initial_DataWriterQos();
  qos_data.pub_qos    = TheServiceParticipant->initial_PublisherQos();
  qos_data.topic_name = "";
}

} }

OPENDDS_END_VERSIONED_NAMESPACE_DECL
