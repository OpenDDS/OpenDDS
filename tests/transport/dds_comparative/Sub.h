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


class Sub : public OpenDDS::DCPS::TransportInterface
{
  public:

    Sub();
    virtual ~Sub();

    /// Set the number of messages expected to be received
    void set_num_to_receive(unsigned num_to_receive);

    /// Set the Data Size
    void set_data_size(char data_size);

    /// Set the local subscriber/reader info
    void set_local_subscriber(OpenDDS::DCPS::RepoId sub_id);

    /// Add a remote publisher
    void add_remote_publisher(OpenDDS::DCPS::RepoId    pub_id,
                              const ACE_INET_Addr& pub_addr,
                              const ACE_TString&   pub_addr_str);

    void init(unsigned impl_id);
    void wait();


  protected:

    virtual void transport_detached_i();


  private:

    void init_attach_transport(unsigned impl_id);
    void init_add_publications();

    struct PubInfo
    {
      OpenDDS::DCPS::RepoId pub_id_;
      ACE_INET_Addr     pub_addr_;
      ACE_TString       pub_addr_str_;

      PubInfo(OpenDDS::DCPS::RepoId pub_id,
              const ACE_INET_Addr& pub_addr,
              const ACE_TString&   pub_addr_str)
        : pub_id_(pub_id), pub_addr_(pub_addr), pub_addr_str_(pub_addr_str)
        {}

      void as_association(OpenDDS::DCPS::AssociationData& assoc_data)
        {
          assoc_data.remote_id_ = this->pub_id_;
          assoc_data.remote_data_.transport_id = 1;
          assoc_data.remote_data_.publication_transport_priority = 0;

          OpenDDS::DCPS::NetworkAddress network_order_address(this->pub_addr_str_.c_str());

          ACE_OutputCDR cdr;
          cdr << network_order_address;
          size_t len = cdr.total_length ();

          assoc_data.remote_data_.data
            = OpenDDS::DCPS::TransportInterfaceBLOB
            (len,
            len,
            (CORBA::Octet*)(cdr.buffer ()));
        }
    };


    typedef std::vector<PubInfo> PubInfoList;

    OpenDDS::DCPS::RepoId sub_id_;

    SubReader reader_;

    PubInfoList pubs_;
};

#endif  /* SUB_H */
