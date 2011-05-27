// -*- C++ -*-
//
// $Id$
#ifndef PUB_H
#define PUB_H

#include "PubWriter.h"
#include "dds/DCPS/transport/framework/TransportInterface.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
#include "ace/INET_Addr.h"
#include <vector>


class Pub : public OpenDDS::DCPS::TransportInterface
{
  public:

    /// Default Ctor
    Pub();

    /// Virtual Dtor
    virtual ~Pub();

    /// Set the number of messages to be sent
    void set_num_to_send(unsigned num_to_send);

    /// Set the Data Size
    void set_data_size(char data_size);

    /// Set the local publisher/writer info
    void set_local_publisher(OpenDDS::DCPS::RepoId pub_id);

    /// Add a remote subscriber
    void add_remote_subscriber(OpenDDS::DCPS::RepoId    sub_id,
                               const ACE_INET_Addr& sub_addr);

    /// Initialize the publisher.  This causes this publisher to attach itself
    /// to the appropriate TransportImpl object, followed by a call to
    /// add_subscriptions() for all of the remote subscribers that have
    /// been added to this Pub object.
    void init(unsigned impl_id);

    /// Run the publisher.  This causes this publisher to send() all of the
    /// messages to the transport. 
    void run();

    /// Wait until the publisher gets confirmation from the transport that
    /// all of the sent messages have been delivered.
    void wait();


    void send_samples(const OpenDDS::DCPS::DataSampleList& samples);


  protected:

    /// Only called if the TransportImpl is shutdown() when this
    /// TransportInterface object is still attached to the TransportImpl.
    virtual void transport_detached_i();


  private:

    /// Helper method called by our init() method to attach ourselves to
    /// the TransportImpl identified by the supplied impl_id argument.
    /// This method takes care of the error checking that needs to be done
    /// as part of the attachment logic, and will raise a TestException if
    /// things don't go smoothly.  An error message will be logged prior to
    /// raising an exception.
    void init_attach_transport(unsigned impl_id);

    /// Another helper method called by our init() method.  This will invoke
    /// the add_subscriptions() method (we inherited from TransportInterface)
    /// with all of the proper arguments.  A TestException will be raised if
    /// things don't go smoothly.  An error message will be logged prior to
    /// raising an exception.
    void init_add_subscriptions();

    struct SubInfo
    {
      OpenDDS::DCPS::RepoId sub_id_;
      ACE_INET_Addr     sub_addr_;

      SubInfo(OpenDDS::DCPS::RepoId sub_id, const ACE_INET_Addr& sub_addr)
        : sub_id_(sub_id), sub_addr_(sub_addr)
        {}

      void as_association(OpenDDS::DCPS::AssociationData& assoc_data)
        {
          assoc_data.remote_id_ = this->sub_id_;
          assoc_data.remote_data_.transport_id = 1;

          OpenDDS::DCPS::NetworkAddress network_order_address(this->sub_addr_);

          assoc_data.remote_data_.data = OpenDDS::DCPS::TransportInterfaceBLOB
                                   (sizeof(OpenDDS::DCPS::NetworkAddress),
                                    sizeof(OpenDDS::DCPS::NetworkAddress),
                                    (CORBA::Octet*)(&network_order_address));
        }
    };

    typedef std::vector<SubInfo> SubInfoList;

    OpenDDS::DCPS::RepoId pub_id_;

    /// The lone PubWriter object.
    PubWriter writer_;

    /// The list of SubInfo objects (each holds a sub_id and a sub_addr).
    SubInfoList subs_;
};

#endif  /* PUB_H */
