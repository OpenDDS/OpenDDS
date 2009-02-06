// -*- C++ -*-
//
// $Id$

#include "SimpleDataWriter.h"
#include "SimplePublisher.h"
#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "ace/OS.h"
#include <sstream>

#include "dds/DCPS/transport/framework/EntryExit.h"


SimpleDataWriter::SimpleDataWriter()
  : pub_id_( OpenDDS::DCPS::GUID_UNKNOWN),
    num_messages_sent_(0),
    num_messages_delivered_(0)
{
}


SimpleDataWriter::~SimpleDataWriter()
{
}


void
SimpleDataWriter::init(OpenDDS::DCPS::RepoId pub_id)
{
  this->pub_id_ = pub_id;
}


int
SimpleDataWriter::run(SimplePublisher* publisher,
                      unsigned num_messages,
                      unsigned msg_size)
{
  this->num_messages_delivered_ = 0;
  this->num_messages_sent_      = num_messages;

  // Set up the DataSampleList
  OpenDDS::DCPS::DataSampleList samples;

  samples.head_ = 0;
  samples.tail_ = 0;
  samples.size_ = num_messages;

  // This is what goes in the "Data Block".
  std::string data;
  for (unsigned j = 1; j <= msg_size; ++j)
    {
      data += char (1 + (j % 255));
    }

  // Now we can create the DataSampleHeader struct and set its fields.
  OpenDDS::DCPS::DataSampleHeader header;
  header.message_id_ = 1;
  header.publication_id_ = this->pub_id_;

  OpenDDS::DCPS::DataSampleListElement* prev_element = 0;

  OpenDDS::DCPS::DataSampleListElementAllocator allocator(num_messages);
  OpenDDS::DCPS::TransportSendElementAllocator trans_allocator(num_messages, sizeof (OpenDDS::DCPS::TransportSendElement));

  for (unsigned i = 1; i <= num_messages; ++i)
    {
      // This is what goes in the "Data Block".
      std::ostringstream ostr;
      ostr << data;

      std::string data_str = ostr.str();

      ssize_t num_data_bytes = data_str.length() + 1;

      header.message_length_ = num_data_bytes;
      header.sequence_ = i;

      // The DataSampleHeader is what goes in the "Header Block".
      ACE_Message_Block* header_block =
                          new ACE_Message_Block(header.max_marshaled_size());
      header_block << header;

      ACE_Message_Block* data_block = new ACE_Message_Block(num_data_bytes);
      data_block->copy(data_str.c_str());

      // Chain the "Data Block" to the "Header Block"
      header_block->cont(data_block);

      // Create the DataSampleListElement now.
      OpenDDS::DCPS::DataSampleListElement* element;

      ACE_NEW_MALLOC_RETURN(element,
              static_cast<OpenDDS::DCPS::DataSampleListElement*> (allocator.malloc(sizeof (OpenDDS::DCPS::DataSampleListElement))),
              OpenDDS::DCPS::DataSampleListElement(this->pub_id_, this, 0, &trans_allocator),
              1);

      // The Sample Element will hold the chain of blocks (header + data).
      element->sample_ = header_block;

      if (prev_element == 0)
        {
          samples.head_ = element;
        }
      else
        {
          prev_element->next_send_sample_ = element;
        }

      prev_element = element;
      samples.tail_ = element;
    }

  publisher->send_samples(samples);

  ACE_OS::sleep(15);

  return 0;
}


void
SimpleDataWriter::transport_lost()
{
  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) The transport has been lost.\n"));
}


void
SimpleDataWriter::data_delivered(OpenDDS::DCPS::DataSampleListElement* sample)
{
  ACE_UNUSED_ARG(sample);

  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) Got our data_delivered() for %d sample.\n",
             this->num_messages_delivered_ + 1));

  //TBD: Cannot delete the sample here because this sample will be
  //     used by the TransportInterface::send to look for the next
  //     send sample. 
  //     Just leak here or put into a list for deletion later.
  // Delete the element
  //delete sample;

  ++this->num_messages_delivered_;

#if 0
  // DEBUG CODE
  // Lets turn on verbose debugging when we hit a certain number of delivered
  // messages.
  if (this->num_messages_delivered_ == 2300)
    {
      TURN_ON_VERBOSE_DEBUG;
    }
#endif
}


void
SimpleDataWriter::data_dropped(OpenDDS::DCPS::DataSampleListElement* sample,
                               bool dropped_by_transport)
{
  ACE_UNUSED_ARG(sample);
  ACE_UNUSED_ARG(dropped_by_transport);

  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) The transport has confirmed that a sample has "
             "been dropped.\n"));

  //TBD: Cannot delete the sample here because this sample will be
  //     used by the TransportInterface::send to look for the next
  //     send sample. 
  //     Just leak here or put into a list for deletion later.
  // Delete the element
  //delete sample;

  ++this->num_messages_delivered_;
}


int
SimpleDataWriter::delivered_test_message()
{
  return (this->num_messages_delivered_ == this->num_messages_sent_) ? 1 : 0;
}
