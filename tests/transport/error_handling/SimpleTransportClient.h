// -*- C++ -*-
//
// $Id$
#ifndef SIMPLETRANSPORTCLIENT_H
#define SIMPLETRANSPORTCLIENT_H

#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"

class SimpleTransportClient : public OpenDDS::DCPS::TransportClient
{
 public:

  SimpleTransportClient()
    : exceptionThrown(false)
    {
    }

  void enable()
    {
      try 
        {
          exceptionThrown = false;
          this->enable_transport(false, false);
        } catch (const OpenDDS::DCPS::Transport::Exception&) {
              exceptionThrown = true;
        }
    }

  bool check_transport_qos(const OpenDDS::DCPS::TransportInst&)
    { return true; }
  const OpenDDS::DCPS::RepoId& get_repo_id() const
    { return repoId_; }
  CORBA::Long get_priority_value(const OpenDDS::DCPS::AssociationData&) const
    { return 0; }

  bool exceptionThrown;

  OpenDDS::DCPS::RepoId repoId_;
};

#endif
