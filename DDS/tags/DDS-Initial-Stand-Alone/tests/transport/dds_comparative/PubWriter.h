// -*- C++ -*-
//
// $Id$
#ifndef PUBWRITER_H
#define PUBWRITER_H

#include  "dds/DCPS/transport/framework/TransportSendListener.h"
#include  "dds/DCPS/DataSampleHeader.h"
#include  "dds/DCPS/Definitions.h"
#include  "ace/Synch.h"
#include  "ace/Condition_T.h"

class Pub;


class PubWriter : public TAO::DCPS::TransportSendListener
{
  public:

    /// Default Ctor
    PubWriter();

    /// Virtual Dtor
    virtual ~PubWriter();

    /// The number of messages to be sent
    void set_num_to_send(unsigned num_to_send);

    /// The size of the data portion of each message.
    void set_data_size(char data_size);

    /// The RepoId (publication id) for this DataWriter.
    void set_id(TAO::DCPS::RepoId pub_id);

    /// Send all of the messages.
    void run(Pub* publisher);

    /// Returns true if the writer has received confirmation of delivery
    /// for each message sent to the transport via the TransportInterface.
    /// Returns false otherwise.
    bool is_done() const;

    /// Invoked when the Pub object that "owns" this PubWriter object has
    /// been informed that the transport has detached from the Pub object.
    /// This happens when the TransportImpl is shutdown() (due to a release()
    /// call on TheTransportFactory) and the Pub object is still attached
    /// to the TransportImpl object.
    void transport_lost();

    /// TransportSendListener method that confirms delivery of a message
    /// that we had previously sent to the TransportInterface.
    virtual void data_delivered(TAO::DCPS::DataSampleListElement* sample);

    /// TransportSendListener method that confirms a message has been
    /// been dropped from the transport, at our request.  We make the
    /// request by asking the TransportInterface to remove_sample().
    /// Thus, this should only be invoked if the remove_sample() method
    /// is actually used by the particular test.
    virtual void data_dropped(TAO::DCPS::DataSampleListElement* sample);


  private:

    TAO::DCPS::DataSampleListElement* get_element
                                       (TAO::DCPS::DataSampleHeader& header);

    typedef ACE_SYNCH_MUTEX         LockType;
    typedef ACE_Guard<LockType>     GuardType;
    typedef ACE_Condition<LockType> ConditionType;


    TAO::DCPS::RepoId pub_id_;

    char data_size_;

    unsigned num_to_send_;

    // Lock used to guard the num_delivered_ and num_dropped_.
    mutable LockType lock_;
    ConditionType condition_;

    unsigned num_sent_;
    unsigned num_delivered_;
    unsigned num_dropped_;
};

#endif  /* PUBWRITER_H */
