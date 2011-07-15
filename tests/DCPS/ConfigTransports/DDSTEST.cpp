/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/dcps_export.h"

#include "DDSTEST.h"

#include "ace/Log_Msg.h"

#include "dds/DCPS/EntityImpl.h"
#include "dds/DdsDcpsDomainC.h"

#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportConfig.h"

//#include "dds/DCPS/DomainParticipantImpl.h"
//#include "dds/DCPS/transport/framework/TransportImpl.h"
//#include "dds/DCPS/transport/framework/TransportInst.h"
//#include "dds/DCPS/transport/framework/DataLinkSet.h"

#include <vector>

//namespace {
int
::DDS_TEST::supports (const DDS::DataReader* dr, const std::string& protocol_name)
{
  ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("(%P|%t) Validating data reader\n")));

  return supports (dynamic_cast<const OpenDDS::DCPS::TransportClient*> (dr), protocol_name);
}

int
::DDS_TEST::supports (const DDS::DataWriter* dw, const std::string& protocol_name)
{
  ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("(%P|%t) Validating data writer\n")));

  return supports (dynamic_cast<const OpenDDS::DCPS::TransportClient*> (dw), protocol_name);
}

int
::DDS_TEST::supports (const DDS::Entity* pub, const std::string& protocol_name)
{
  ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("(%P|%t) Validating entity\n")));

  return supports (dynamic_cast<const OpenDDS::DCPS::EntityImpl*> (pub), protocol_name);
}

int
::DDS_TEST::supports (const DDS::DomainParticipant* pa, const std::string& protocol_name)
{
  ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("(%P|%t) Validating participant\n")));
  return supports (dynamic_cast<const OpenDDS::DCPS::EntityImpl*> (pa), protocol_name);
}

int
::DDS_TEST::supports (const OpenDDS::DCPS::EntityImpl* entity, const std::string& protocol_name)
{

  ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("(%P|%t) Validating entity: %@\n"), entity));

  const OpenDDS::DCPS::TransportConfig_rch tc = entity->transport_config ();

  if (tc.is_nil ())
    {
      ACE_ERROR_RETURN ((LM_INFO,
                         ACE_TEXT ("(%P|%t) Null transport config for entity %@.\n"),
                         entity),
                        0);
    }

  for (std::vector<OpenDDS::DCPS::TransportInst_rch>::const_iterator it = tc->instances_.begin (); it != tc->instances_.end (); ++it)
    {
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("(%P|%t) Checking '%C' == '%C' ...\n"),
                  protocol_name.c_str (),
                  (*it)->name ().c_str ()));

      if ((*it)->name () == protocol_name)
        {
          ACE_ERROR_RETURN ((LM_INFO,
                             ACE_TEXT ("(%P|%t) Found transport '%C'\n."), protocol_name.c_str ()),
                            1);
        }
    }

  ACE_ERROR_RETURN ((LM_INFO,
                     ACE_TEXT ("(%P|%t) Unable to find transport %C.\n"),
                     protocol_name.c_str ()),
                    0);
}

int
::DDS_TEST::supports (const OpenDDS::DCPS::TransportClient* tc, const std::string& name)
{
  if (tc == 0)
    {
      ACE_ERROR_RETURN ((LM_INFO,
                         ACE_TEXT ("(%P|%t) Null transport client.\n")),
                        0);
    }

  for (std::vector<OpenDDS::DCPS::TransportImpl_rch>::const_iterator it = tc->impls_.begin ();
          it != tc->impls_.end (); ++it)
    {

      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("(%P|%t) Checking '%C' == '%C' ...\n"),
                  name.c_str (),
                  (*it)->config ()->name ().c_str ()));

      if ((*it)->config ()->name () == name)
        {
          ACE_ERROR_RETURN ((LM_INFO,
                             ACE_TEXT ("(%P|%t) Found transport '%C'.\n"), name.c_str ()),
                            1);
        }
    }

  ACE_ERROR_RETURN ((LM_INFO,
                     ACE_TEXT ("(%P|%t) Unable to find transport %C.\n"),
                     name.c_str ()),
                    0);
}

//};
