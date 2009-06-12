// -*- C++ -*-
//
// $Id$

#include "PubWriter.h"
#include "Pub.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/GuidBuilder.h"


PubWriter::PubWriter()
  : pub_id_(OpenDDS::DCPS::GuidBuilder::create ()),
    data_size_(0),
    num_to_send_(0),
    condition_(this->lock_),
    num_sent_(0),
    num_delivered_(0),
    num_dropped_(0),
    trans_allocator(20, sizeof (OpenDDS::DCPS::TransportSendElement))
{
}


PubWriter::~PubWriter()
{
}


void
PubWriter::set_num_to_send(unsigned num_to_send)
{
  this->num_to_send_ = num_to_send;
}


void
PubWriter::set_data_size(char data_size)
{
  this->data_size_ = data_size;
}


void
PubWriter::set_id(OpenDDS::DCPS::RepoId pub_id)
{
  this->pub_id_ = pub_id;
}


void
PubWriter::run(Pub* publisher)
{
  // Make sure we start with our counters initialized to 0.
  this->num_sent_ = 1;
  this->num_delivered_ = 0;
  this->num_dropped_ = 0;

  // We use the same DataSampleList object for each send() call.
  OpenDDS::DCPS::DataSampleList samples;

  // We are only sending one message at a time.
  samples.size_ = 1;

  // We will reuse the same DataSampleHeader object for each message.
  OpenDDS::DCPS::DataSampleHeader header;

  // The publication id only needs to be set once since it will be the same
  // for all of these messages.
  header.publication_id_ = this->pub_id_;
  header.message_id_ = 1;

  header.message_length_ = 1 << this->data_size_;

  // Now we need to send the messages.
  for (unsigned i = 1; i <= this->num_to_send_; i++)
    {
      // Each message gets a unique sequence_ number
      // (incrementing, starting at 1).
      header.sequence_ = i;

      // Adjust the DataSampleList object to contain just the one element.
      samples.head_ = samples.tail_ = this->get_element(header);

      // Tell the transport interface to send the list of messages.
      // (currently, the list always contains exactly one message).
      publisher->send_samples(samples);

      {
        GuardType guard(this->lock_);

        ++this->num_sent_;

        if (i % 500 == 0)
          {
            unsigned num_unconfirmed = this->num_sent_ -
                                                   (this->num_delivered_ +
                                                    this->num_dropped_);

            if (num_unconfirmed >= 500)
              {
                ACE_DEBUG((LM_DEBUG,
                           "(%P|%t) 500 or more unconfirmed. Wait.\n"));
                this->condition_.wait();
                ACE_DEBUG((LM_DEBUG, "(%P|%t) Awake.\n"));
              }
          }
      }
    }
}


bool
PubWriter::is_done() const
{
  unsigned num_confirmed;

  {
    GuardType guard(this->lock_);
    num_confirmed = this->num_delivered_ + this->num_dropped_;
  }

  return (this->num_to_send_ == num_confirmed);
}


void
PubWriter::transport_lost()
{
}


void
PubWriter::data_delivered(OpenDDS::DCPS::DataSampleListElement* sample)
{
  {
    GuardType guard(this->lock_);
    ++this->num_delivered_;

    unsigned num_unconfirmed = this->num_sent_ - (this->num_delivered_ +
                                                  this->num_dropped_);
    if (num_unconfirmed == 200)
      {
        this->condition_.signal();
      }
  }

  // TBD SOON - Release back to allocator
  delete sample;
}


void
PubWriter::data_dropped(OpenDDS::DCPS::DataSampleListElement* sample,
                        bool dropped_by_transport)
{
  ACE_UNUSED_ARG (dropped_by_transport);

  {
    GuardType guard(this->lock_);
    ++this->num_dropped_;

    unsigned num_unconfirmed = this->num_sent_ - (this->num_delivered_ +
                                                  this->num_dropped_);
    if (num_unconfirmed == 200)
      {
        this->condition_.signal();
      }
  }

  // TBD SOON - Release back to allocator
  delete sample;
}


OpenDDS::DCPS::DataSampleListElement*
PubWriter::get_element(OpenDDS::DCPS::DataSampleHeader& header)
{
  // Each message needs to be "wrapped" inside of a DataSampleListElement.
  // TBD SOON - Use an allocator
  OpenDDS::DCPS::DataSampleListElement* elem =
                         new OpenDDS::DCPS::DataSampleListElement(this->pub_id_,
                                                              this,
                                                              0,
                                                              &trans_allocator);

  // TBD SOON - Use an allocator
  elem->sample_ = new ACE_Message_Block(header.max_marshaled_size());
  elem->sample_ << header;

  // TBD SOON - Use an allocator
  ssize_t num_data_bytes = 1 << this->data_size_;
  elem->sample_->cont(new ACE_Message_Block(num_data_bytes));
  elem->sample_->cont()->wr_ptr(num_data_bytes);

  return elem;
}
