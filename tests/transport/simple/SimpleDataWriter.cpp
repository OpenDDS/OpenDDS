// -*- C++ -*-
//
// $Id$

#include "SimpleDataWriter.h"
#include "SimplePublisher.h"
#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "dds/DCPS/GuidBuilder.h"
#include "ace/OS.h"

#include "dds/DCPS/transport/framework/EntryExit.h"
#include "ace/SString.h"


SimpleDataWriter::SimpleDataWriter()
: pub_id_ (OpenDDS::DCPS::GuidBuilder::create ()),
  delivered_test_message_(0)
{
  DBG_ENTRY("SimpleDataWriter","SimpleDataWriter");
}


SimpleDataWriter::~SimpleDataWriter()
{
  DBG_ENTRY("SimpleDataWriter","~SimpleDataWriter");
}


void
SimpleDataWriter::init(OpenDDS::DCPS::RepoId pub_id)
{
  DBG_ENTRY("SimpleDataWriter","init");

  this->pub_id_ = pub_id;
}


int
SimpleDataWriter::run(SimplePublisher* publisher)
{
  DBG_ENTRY("SimpleDataWriter","run");

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Build the DataSampleElementList to contain one element - "
             "our 'Hello World' string.\n"));

  // We just send one message.

  // This is what goes in the "Data Block".
  ACE_CString data = "Hello World!";

  // Now we can create the DataSampleHeader struct and set its fields.
  OpenDDS::DCPS::DataSampleHeader header;

  // The +1 makes the null terminator ('/0') get placed into the block.
  header.message_length_ = data.length() + 1;
  header.message_id_ = 1;
  header.sequence_ = 0;
  // TMB - Compiler no longer likes the next line...  source_timestamp_ is gone.
  //header.source_timestamp_ = ACE_OS::gettimeofday().msec();
  header.publication_id_ = this->pub_id_;

  // The DataSampleHeader is what goes in the "Header Block".
  ACE_Message_Block* header_block = new ACE_Message_Block
                                                (header.max_marshaled_size());
  header_block << header;

  // The +1 makes the null terminator ('/0') get placed into the block.
  ACE_Message_Block* data_block = new ACE_Message_Block(data.length() + 1);
  data_block->copy(data.c_str());

  // Chain the "Data Block" to the "Header Block"
  header_block->cont(data_block);

  // Create the DataSampleListElement now.
  OpenDDS::DCPS::DataSampleListElementAllocator allocator(3);
  OpenDDS::DCPS::TransportSendElementAllocator trans_allocator(3, sizeof (OpenDDS::DCPS::TransportSendElement));
  OpenDDS::DCPS::DataSampleListElement* element;

  ACE_NEW_MALLOC_RETURN(element,
           static_cast<OpenDDS::DCPS::DataSampleListElement*> (allocator.malloc(sizeof (OpenDDS::DCPS::DataSampleListElement))),
           OpenDDS::DCPS::DataSampleListElement(this->pub_id_, this, 0, &trans_allocator),
           1);


  // The Sample Element will hold on to the chain of blocks (header + data).
  element->sample_ = header_block;

  // Set up the DataSampleList
  OpenDDS::DCPS::DataSampleList samples;

  samples.head_ = element;
  samples.tail_ = element;
  samples.size_ = 1;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Ask the publisher to send the DataSampleList (samples).\n"));

  publisher->send_samples(samples);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The Publisher has finished sending the samples.\n"));

  return 0;
}


void
SimpleDataWriter::transport_lost()
{
  DBG_ENTRY("SimpleDataWriter","transport_lost");

  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) The transport has been lost.\n"));
}


void
SimpleDataWriter::data_delivered(const OpenDDS::DCPS::DataSampleListElement* sample)
{
  DBG_ENTRY("SimpleDataWriter","data_delivered");

  ACE_UNUSED_ARG(sample);

  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) The transport has confirmed that a sample has "
             "been delivered.\n"));

  //TBD: Cannot delete the sample here because this sample will be
  //     used by the TransportInterface::send to look for the next
  //     send sample.
  //     Just leak here or put into a list for deletion later.
  // Delete the element
  //delete sample;

  this->delivered_test_message_ = 1;
}


void
SimpleDataWriter::data_dropped(const OpenDDS::DCPS::DataSampleListElement* sample,
                               bool dropped_by_transport)
{
  DBG_ENTRY("SimpleDataWriter","data_dropped");

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

  this->delivered_test_message_ = 1;
}


int
SimpleDataWriter::delivered_test_message()
{
  return this->delivered_test_message_;
}
