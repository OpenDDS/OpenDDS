// -*- C++ -*-
//
// $Id$
#ifndef SUB_H
#define SUB_H

#include "SubReader.h"
#include "dds/DCPS/transport/framework/TransportInterface.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
#include "ace/INET_Addr.h"
#include <vector>


class Sub : public TAO::DCPS::TransportInterface
{
  public:

    Sub();
    virtual ~Sub();

    /// Set the number of messages expected to be received
    void set_num_to_receive(unsigned num_to_receive);

    /// Set the Data Size
    void set_data_size(char data_size);

    /// Set the local subscriber/reader info
    void set_local_subscriber(TAO::DCPS::RepoId sub_id);

    /// Add a remote publisher
    void add_remote_publisher(TAO::DCPS::RepoId    pub_id,
                              const ACE_INET_Addr& pub_addr);

    void init(unsigned impl_id);
    void wait();


  protected:

    virtual void transport_detached_i();


  private:

    void init_attach_transport(unsigned impl_id);
    void init_add_publications();

    struct PubInfo
    {
      TAO::DCPS::RepoId pub_id_;
      ACE_INET_Addr     pub_addr_;

      PubInfo(TAO::DCPS::RepoId pub_id, const ACE_INET_Addr& pub_addr)
        : pub_id_(pub_id), pub_addr_(pub_addr)
        {}

      void as_association(TAO::DCPS::AssociationData& assoc_data)
        {
          assoc_data.remote_id_ = this->pub_id_;
          assoc_data.remote_data_.transport_id = 1;

          TAO::DCPS::NetworkAddress network_order_address(this->pub_addr_);

          assoc_data.remote_data_.data = TAO::DCPS::TransportInterfaceBLOB
                                   (sizeof(TAO::DCPS::NetworkAddress),
                                    sizeof(TAO::DCPS::NetworkAddress),
                                    (CORBA::Octet*)(&network_order_address));
        }
    };


    typedef std::vector<PubInfo> PubInfoList;

    TAO::DCPS::RepoId sub_id_;

    SubReader reader_;

    PubInfoList pubs_;
};

#endif  /* SUB_H */
