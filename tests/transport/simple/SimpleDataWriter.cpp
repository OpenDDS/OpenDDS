// -*- C++ -*-
//

#include "SimpleDataWriter.h"
#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/SendStateDataSampleList.h"
#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "dds/DCPS/GuidBuilder.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "ace/SString.h"
#include "ace/OS_NS_sys_time.h"
#include "ace/OS_NS_unistd.h"

#include "TestException.h"

#include <sstream>

SimpleDataWriter::SimpleDataWriter(const OpenDDS::DCPS::RepoId& pub_id)
  : pub_id_(pub_id)
  , num_messages_sent_(0)
  , num_messages_delivered_(0)
{
  DBG_ENTRY("SimpleDataWriter","SimpleDataWriter");
}


SimpleDataWriter::~SimpleDataWriter()
{
  DBG_ENTRY("SimpleDataWriter","~SimpleDataWriter");
}


void
SimpleDataWriter::init(const OpenDDS::DCPS::AssociationData& subscription)
{
  DBG_ENTRY("SimpleDataWriter","init");

  // Add the association between the local sub_id and the remote pub_id
  // to the transport via the TransportInterface.
  bool result = this->associate(subscription, true /* active */);

  if (!result) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) SimpleDataWriter::init() Failed to associate.\n"));
    throw TestException();
  }
}

namespace
{

  class Cleanup
  {
  public:
    Cleanup(OpenDDS::DCPS::DataSampleElementAllocator& alloc,
            OpenDDS::DCPS::SendStateDataSampleList& list)
    : alloc_(alloc)
    , list_(list)
    {
    }

    ~Cleanup()
    {
      DDS_TEST::cleanup(alloc_, list_);
    }

  private:
    OpenDDS::DCPS::DataSampleElementAllocator& alloc_;
    OpenDDS::DCPS::SendStateDataSampleList& list_;
  };
}


void
SimpleDataWriter::transport_lost()
{
  DBG_ENTRY("SimpleDataWriter","transport_lost");

  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) The transport has been lost.\n"));
}


void
SimpleDataWriter::data_delivered(const OpenDDS::DCPS::DataSampleElement* sample)
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

  ++this->num_messages_delivered_;
}


void
SimpleDataWriter::data_dropped(const OpenDDS::DCPS::DataSampleElement* sample,
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

  ++this->num_messages_delivered_;
}


int
SimpleDataWriter::delivered_test_message()
{
  return (this->num_messages_delivered_ == this->num_messages_sent_) ? 1 : 0;
}


DDS_TEST::DDS_TEST(const OpenDDS::DCPS::RepoId& pub_id)
  : SimpleDataWriter(pub_id)
{
}

void DDS_TEST::cleanup(OpenDDS::DCPS::DataSampleElementAllocator& alloc,
                       OpenDDS::DCPS::SendStateDataSampleList& list)
{
  OpenDDS::DCPS::DataSampleElement* element = list.head();
  while (element != 0) {
    OpenDDS::DCPS::DataSampleElement* const to_remove = element;
    element = element->get_next_send_sample();
    to_remove->~DataSampleElement();
    alloc.free(to_remove);
  }
}

int
DDS_TEST::run(int num_messages, int msg_size)
{
  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Build the DataSampleElementList\n"));

  this->num_messages_delivered_ = 0;
  this->num_messages_sent_      = num_messages;

  // Set up the SendStateDataSampleList
  OpenDDS::DCPS::SendStateDataSampleList samples;

  samples.size_ = num_messages;

  // Now we can create the DataSampleHeader struct and set its fields.
  OpenDDS::DCPS::DataSampleHeader header;

  // The +1 makes the null terminator ('\0') get placed into the block.
  header.message_id_ = 1;
  header.publication_id_ = this->pub_id_;

  OpenDDS::DCPS::DataSampleElement* prev_element = 0;

  OpenDDS::DCPS::DataSampleElementAllocator allocator(num_messages);
  Cleanup cleanup(allocator, samples);
  OpenDDS::DCPS::TransportSendElementAllocator trans_allocator(num_messages, sizeof (OpenDDS::DCPS::TransportSendElement));

  std::string data = "Hello World!";

  if (msg_size) {
    data.clear();
    for (int j = 1; j <= msg_size; ++j) {
      data += char(1 + (j % 255));
    }
  }

  for (int i = 1; i <= num_messages; ++i) {
    // This is what goes in the "Data Block".
    std::ostringstream ostr;
    ostr << data << " [" << i << "]";

    std::string data_str = msg_size ? data : ostr.str();

    ssize_t num_data_bytes = data_str.length() + 1;

    header.message_length_ = static_cast<ACE_UINT32>(num_data_bytes);
    header.sequence_ = i;

    // The DataSampleHeader is what goes in the "Header Block".
    ACE_Message_Block* header_block =
      new ACE_Message_Block(header.max_marshaled_size());
    *header_block << header;

    ACE_Message_Block* data_block = new ACE_Message_Block(num_data_bytes);
    data_block->copy(data_str.c_str());

    // Chain the "Data Block" to the "Header Block"
    header_block->cont(data_block);

    // Create the DataSampleElement now.
    OpenDDS::DCPS::DataSampleElement* element;

    ACE_NEW_MALLOC_RETURN(element,
      static_cast<OpenDDS::DCPS::DataSampleElement*>(allocator.malloc(sizeof (OpenDDS::DCPS::DataSampleElement))),
      OpenDDS::DCPS::DataSampleElement(this->pub_id_, this, OpenDDS::DCPS::PublicationInstance_rch(), &trans_allocator, 0), 1);

    // The Sample Element will hold on to the chain of blocks (header + data).
    element->sample_ = header_block;
    if (prev_element == 0) {
      samples.head_ = element;
    } else {
      prev_element->set_next_send_sample(element);
    }

    prev_element = element;
    samples.tail_ = element;
  }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Send the SendStateDataSampleList (samples).\n"));

  ACE_Time_Value start = ACE_OS::gettimeofday();
  this->send(samples);
  ACE_Time_Value finished = ACE_OS::gettimeofday();

  ACE_Time_Value total = finished - start;
  ACE_DEBUG((LM_INFO,
    "(%P|%t) Publisher total time required was %d.%d seconds.\n",
             total.sec(),
             total.usec() % 1000000));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The Publisher has finished sending the samples.\n"));

  if (msg_size) {
    ACE_OS::sleep(15);
  }

  return 0;
}

