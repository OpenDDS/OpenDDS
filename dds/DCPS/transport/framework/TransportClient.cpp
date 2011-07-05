/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportClient.h"
#include "TransportConfig.h"
#include "TransportRegistry.h"
#include "TransportExceptions.h"
#include "dds/DCPS/EntityImpl.h"

namespace OpenDDS {
namespace DCPS {

TransportClient::TransportClient()
{}

TransportClient::~TransportClient()
{
  for (std::vector<TransportImpl_rch>::iterator it = impls_.begin();
       it != impls_.end(); ++it) {
    (*it)->detach_client(this);
  }
}

void
TransportClient::enable_transport()
{
  EntityImpl& ent = dynamic_cast<EntityImpl&>(*this);
  TransportConfig_rch tc = ent.transport_config();
  for (EntityImpl* p = ent.parent(); p && tc.is_nil(); p = p->parent()) {
    tc = p->transport_config();
  }
  if (tc.is_nil()) {
    tc = TransportRegistry::instance()->global_config();
  }
  if (tc.is_nil()) {
    throw Transport::NotConfigured();
  }

  const size_t n = tc->instances_.size();
  for (size_t i = 0; i < n; ++i) {
    TransportInst_rch inst = tc->instances_[i];
    if (check_transport_qos(*inst.in())) {
      TransportImpl_rch impl = inst->impl();
      impl->attach_client(this);
      impls_.push_back(impl);
    }
  }
}

void
TransportClient::transport_detached(TransportImpl* which)
{
  for (std::vector<TransportImpl_rch>::iterator it = impls_.begin();
       it != impls_.end(); ++it) {
    if (it->in() == which) {
      impls_.erase(it);
      return;
    }
  }
}

}
}
