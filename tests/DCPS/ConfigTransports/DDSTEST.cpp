/*
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
::DDS_TEST::supports(const OpenDDS::DCPS::TransportClient* tc, const OPENDDS_STRING& name)
{
  if (tc == 0)
    {
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("(%P|%t) Null transport client.\n")));
      return false;
    }

  OPENDDS_STRING supported;
  for (OpenDDS::DCPS::TransportClient::ImplsType::const_iterator it = tc->impls_.begin(),
          end = tc->impls_.end();
          it != end;)
    {
      OpenDDS::DCPS::TransportImpl_rch impl = it->lock();
      if (impl)
        {
          supported += impl->config().name();
          if (++it != end)
            {
              supported += ", ";
            }
        }
    }

  ACE_DEBUG((LM_INFO,
             ACE_TEXT("(%P|%t) Check if '%C' is among supported [%C]?\n"),
             name.c_str(),
             supported.c_str()));

  for (OpenDDS::DCPS::TransportClient::ImplsType::const_iterator it = tc->impls_.begin();
          it != tc->impls_.end(); ++it)
    {

      OpenDDS::DCPS::TransportImpl_rch impl = it->lock();
      if (impl && impl->config().name() == name)
        {
//          ACE_DEBUG((LM_DEBUG,
//                     ACE_TEXT("(%P|%t) Yes. Transport '%C' is supported.\n"),
//                     name.c_str()));
          return true;
        }
    }

//  ACE_DEBUG((LM_DEBUG,
//             ACE_TEXT("(%P|%t) No. Transport '%C' is not supported.\n"),
//             name.c_str()));
  return false;
}

bool
::DDS_TEST::negotiated(const OpenDDS::DCPS::TransportClient* ctc, const OPENDDS_STRING& name)
{
  if (ctc == 0)
    {
      ACE_ERROR_RETURN((LM_WARNING,
                        ACE_TEXT("(%P|%t) WARNING: Null transport client.\n")),
                       false);
    }

  OpenDDS::DCPS::TransportClient* tc = const_cast<OpenDDS::DCPS::TransportClient*> (ctc);
  if (tc == 0)
    {
      ACE_ERROR_RETURN((LM_WARNING,
                        ACE_TEXT("(%P|%t) WARNING: Null transport client.\n")),
                       false);
    }

  if (tc->links_.map().size() == 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: No negotiated protocols?!.\n")),
                       false);
    }


  OPENDDS_STRING negotiated;
  for (OpenDDS::DCPS::DataLinkSet::MapType::iterator iter = tc->links_.map().begin(),
          end = tc->links_.map().end(); iter != end;)
    {
      const OpenDDS::DCPS::DataLink_rch& datalink = iter->second;
      negotiated += datalink->impl().config().name();
      if (++iter != end)
        {
          negotiated += ", ";
        }
    }

  ACE_DEBUG((LM_INFO,
             ACE_TEXT("(%P|%t) Check if '%C' is among negotiated [%C]?\n"),
             name.c_str(),
             negotiated.c_str()));

  for (OpenDDS::DCPS::DataLinkSet::MapType::iterator iter = tc->links_.map().begin(),
          end = tc->links_.map().end(); iter != end; ++iter)
    {

      const OpenDDS::DCPS::DataLink_rch& datalink = iter->second;

      if (datalink->impl().config().name() == name)
        {
//          ACE_DEBUG((LM_DEBUG,
//                     ACE_TEXT("(%P|%t) Yes. Transport '%C' was negotiated.\n"),
//                     name.c_str()));
          return true;
        }
    }

//  ACE_DEBUG((LM_DEBUG,
//             ACE_TEXT("(%P|%t) No. Transport '%C' was not negotiated.\n"),
//             name.c_str()));
  return false;
}

