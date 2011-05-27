// -*- C++ -*-
//
// $Id$
#ifndef SUBREADER_H
#define SUBREADER_H

#include  "dds/DCPS/transport/framework/TransportReceiveListener.h"
#include  "dds/DCPS/Definitions.h"


class SubReader : public TAO::DCPS::TransportReceiveListener
{
  public:

    SubReader();
    virtual ~SubReader();

    void set_num_to_receive(unsigned num_to_receive);
    void set_data_size(char data_size);
    void set_id(TAO::DCPS::RepoId sub_id);
    bool is_done() const;
    void transport_lost();

    virtual void data_received(const TAO::DCPS::ReceivedDataSample& sample);


  private:

    TAO::DCPS::RepoId sub_id_;

    char data_size_;

    unsigned num_expected_;
    unsigned num_received_;
};

#endif  /* SUBREADER_H */
