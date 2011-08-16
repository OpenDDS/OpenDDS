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

#include "dds/DCPS/transport/framework/DataLinkSet.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportConfig.h"

#include <vector>

bool
::DDS_TEST::supports(const OpenDDS::DCPS::TransportClient* tc, const std::string& name)
{
  if (tc == 0)
    {
      ACE_ERROR_RETURN((LM_INFO,
                        ACE_TEXT("(%P|%t) Null transport client.\n")),
                       0);
    }

  for (std::vector<OpenDDS::DCPS::TransportImpl_rch>::const_iterator it = tc->impls_.begin();
          it != tc->impls_.end(); ++it)
    {

      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("(%P|%t) Check if supported '%C' matches '%C'?\n"),
                 (*it)->config()->name().c_str(),
                 name.c_str()));

      if ((*it)->config()->name() == name)
        {
          ACE_ERROR_RETURN((LM_INFO,
                            ACE_TEXT("(%P|%t) Yes. Transport '%C' is supported.\n"),
                            name.c_str()),
                           true);
        }
    }

  ACE_ERROR_RETURN((LM_INFO,
                    ACE_TEXT("(%P|%t) No. Transport '%C' is not supported.\n"),
                    name.c_str()),
                   false);
}

bool
::DDS_TEST::negotiated(const OpenDDS::DCPS::TransportClient* ctc, const std::string& name)
{
  if (ctc == 0)
    {
      ACE_ERROR_RETURN((LM_INFO,
                        ACE_TEXT("(%P|%t) Null transport client.\n")),
                       false);
    }

  OpenDDS::DCPS::TransportClient* tc = const_cast<OpenDDS::DCPS::TransportClient*> (ctc);
  if (tc == 0)
    {
      ACE_ERROR_RETURN((LM_INFO,
                        ACE_TEXT("(%P|%t) Null transport client.\n")),
                       false);
    }


  for (OpenDDS::DCPS::DataLinkSet::MapType::iterator iter = tc->links_.map().begin(),
          end = tc->links_.map().end(); iter != end; ++iter)
    {

      const OpenDDS::DCPS::DataLink_rch& datalink = iter->second;


      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) Check if negotiated '%C' matches '%C'?\n"),
                 datalink->impl()->config()->name().c_str(),
                 name.c_str()));

      if (datalink->impl()->config()->name() == name)
        {
          ACE_ERROR_RETURN((LM_INFO,
                            ACE_TEXT("(%P|%t) Yes. Transport '%C' was negotiated.\n"),
                            name.c_str()),
                           true);
        }
    }

  ACE_ERROR_RETURN((LM_INFO,
                    ACE_TEXT("(%P|%t) No. Transport '%C' was not negotiated.\n"),
                    name.c_str()),
                   false);
}

