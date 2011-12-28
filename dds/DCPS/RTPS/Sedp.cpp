/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Sedp.h"
#include "Spdp.h"

#include "RtpsMessageTypesC.h"
#include "RtpsBaseMessageTypesTypeSupportImpl.h"

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

SedpTransportClient::~SedpTransportClient()
{
}

//---------------------------------------------------------------

bool
SedpWriter::assoc(const DCPS::AssociationData& subscription)
{
  return associate(subscription, true);
}

void
SedpWriter::data_delivered(const DCPS::DataSampleListElement*)
{
}

void
SedpWriter::data_dropped(const DCPS::DataSampleListElement*, bool)
{
}

void
SedpWriter::control_delivered(ACE_Message_Block*)
{
}

void
SedpWriter::control_dropped(ACE_Message_Block*, bool)
{
}

//-------------------------------------------------------------------------

bool
SedpReader::assoc(const DCPS::AssociationData& publication)
{
  return associate(publication, false);
}


// Implementing TransportReceiveListener

void
SedpReader::data_received(const DCPS::ReceivedDataSample& sample)
{
  switch (sample.header_.message_id_) {
  case DCPS::SAMPLE_DATA: {
    DCPS::Serializer ser(sample.sample_,
                         sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                         DCPS::Serializer::ALIGN_CDR);
    bool ok = true;
    ACE_CDR::ULong encap;
    ok &= (ser >> encap); // read and ignore 32-bit CDR Encapsulation header
    ParameterList data;
    ok &= (ser >> data);
    if (!ok) {
      ACE_DEBUG((LM_ERROR, "ERROR: failed to deserialize data\n"));
      return;
    }

    owner_->add_discovered_endpoint(data);

    break;
  }
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

    //TODO: process dispose/unregister
    break;
  }
  default:
    break;
  }
}


}
}
