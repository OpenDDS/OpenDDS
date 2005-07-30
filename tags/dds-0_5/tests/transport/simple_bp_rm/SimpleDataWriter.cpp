// -*- C++ -*-
//
// $Id$

#include  "SimpleDataWriter.h"
#include  "SimplePublisher.h"
#include  "dds/DCPS/DataSampleHeader.h"
#include  "dds/DCPS/DataSampleList.h"
#include  "ace/OS.h"
#include  <sstream>

#include "dds/DCPS/transport/framework/EntryExit.h"

SimpleDataWriter::SimpleDataWriter()
  : pub_id_(0),
    num_sent_(0),
    num_to_send_(0),
    num_delivered_(0),
    element_(0),
    condition_(this->lock_)
{
}


SimpleDataWriter::~SimpleDataWriter()
{
  delete this->element_;
}


void
SimpleDataWriter::init(TAO::DCPS::RepoId pub_id)
{
  // TURN_ON_VERBOSE_DEBUG ;
  this->pub_id_ = pub_id;
  this->element_ = new TAO::DCPS::DataSampleListElement(this->pub_id_,this,0);
}


int
SimpleDataWriter::run(SimplePublisher* publisher, unsigned num_messages)
{
  this->num_sent_ = 0;
  this->num_to_send_ = num_messages;
  this->num_delivered_ = 0;

  std::string data = "Hello World!";

  TAO::DCPS::DataSampleList samples;
  samples.head_ = this->element_;
  samples.tail_ = this->element_;
  samples.size_ = 1;

  TAO::DCPS::DataSampleHeader header;
  header.publication_id_ = this->pub_id_;
  header.message_id_ = 1;
  header.sequence_   = 0;

  while (this->num_sent_ < this->num_to_send_)
    {
      ++header.sequence_;

      // This is what goes in the "Data Block".
      std::ostringstream ostr;
      ostr << data << " [" << header.sequence_ << "]";

      std::string data_str = ostr.str();
      header.message_length_ = data_str.length() + 1;

      this->obtain_element(publisher);

      this->element_->sample_ = new ACE_Message_Block
                                                (header.max_marshaled_size());
      this->element_->sample_ << header;

      this->element_->sample_->cont
                              (new ACE_Message_Block(header.message_length_));
      this->element_->sample_->cont()->copy(data_str.c_str());

      publisher->send_samples(samples);

      GuardType guard(this->lock_);
      ++this->num_sent_;
    }

  return this->num_sent_ ;
}


void
SimpleDataWriter::transport_lost()
{
  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) The transport has been lost.\n"));
}


void
SimpleDataWriter::data_delivered(TAO::DCPS::DataSampleListElement* sample)
{
  unsigned num_delivered = this->release_element(sample);

  VDBG((LM_DEBUG,
             "(%P|%t) Got our data_delivered() for %d sample.\n",
             num_delivered));
}


void
SimpleDataWriter::data_dropped(TAO::DCPS::DataSampleListElement* sample)
{
  unsigned num_delivered = this->release_element(sample);

  VDBG((LM_DEBUG,
             "(%P|%t) Got our data_dropped() for %d sample.\n",
             num_delivered));
}


int
SimpleDataWriter::delivered_test_message()
{
  GuardType guard(this->lock_);
  return (this->num_delivered_ == this->num_to_send_) ? 1 : 0;
}


void
SimpleDataWriter::obtain_element(SimplePublisher* publisher)
{
  GuardType guard(this->lock_);

  if (this->num_delivered_ == this->num_sent_)
    {
      return;
    }

  publisher->remove_sample(this->element_);

  if (this->num_delivered_ == this->num_sent_)
    {
      return;
    }

  this->condition_.wait();
}


unsigned
SimpleDataWriter::release_element(TAO::DCPS::DataSampleListElement* sample)
{
  ACE_UNUSED_ARG(sample);

  GuardType guard(this->lock_);

  this->condition_.signal();

  return ++this->num_delivered_;
}
