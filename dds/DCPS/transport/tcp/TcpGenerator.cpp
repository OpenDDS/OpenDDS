/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpGenerator.h"
#include "TcpInst.h"
#include "TcpFactory.h"

OpenDDS::DCPS::TcpGenerator::TcpGenerator()
{
}

OpenDDS::DCPS::TcpGenerator::~TcpGenerator()
{
}

OpenDDS::DCPS::TransportImplFactory*
OpenDDS::DCPS::TcpGenerator::new_factory()
{
  TcpFactory* factory = 0;
  ACE_NEW_RETURN(factory,
                 TcpFactory(),
                 0);
  return factory;
}

OpenDDS::DCPS::TransportInst*
OpenDDS::DCPS::TcpGenerator::new_configuration(const TransportIdType id)
{
  ACE_UNUSED_ARG(id);

  TcpInst* trans_config = 0;
  ACE_NEW_RETURN(trans_config,
                 TcpInst(),
                 0);
  return trans_config;
}

void
OpenDDS::DCPS::TcpGenerator::default_transport_ids(TransportIdList & ids)
{
  ids.clear();
  ids.push_back(OpenDDS::DCPS::DEFAULT_TCP_ID);
}
