#include <dds/DCPS/transport/framework/CopyChainVisitor.h>
#include <dds/DCPS/transport/framework/RemoveAllVisitor.h>
#include <dds/DCPS/transport/framework/TransportSendElement.h>

#include <dds/DCPS/DataSampleElement.h>
#include <dds/DCPS/PublicationInstance.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

// Tests

TEST(copy_chain_visitor, simple_copy)
{
  ACE_Log_Msg *lm = ACE_LOG_MSG;
  lm->start_tracing();

  OpenDDS::DCPS::Message_Block_Ptr amb(new ACE_Message_Block(5));
  amb->cont(new ACE_Message_Block(8));
  amb->cont()->cont(new ACE_Message_Block(16));

  DataSampleElement* dse = new DataSampleElement(GUID_UNKNOWN, 0, PublicationInstance_rch());
  dse->set_sample(move(amb));

  ASSERT_EQ(dse->get_sample()->reference_count(), 1);

  BasicQueue<TransportQueueElement> queue_in;
  queue_in.put(new TransportSendElement(1, dse));

  BasicQueue<TransportQueueElement> queue_out;
  MessageBlockAllocator mba(1024);
  DataBlockAllocator dba(1024);
  CopyChainVisitor ccv(queue_out, &mba, &dba); // copy

  queue_in.accept_visitor(ccv);

  ASSERT_EQ(dse->get_sample()->reference_count(), 1);

  ASSERT_EQ(queue_in.size(), queue_out.size());
  ASSERT_EQ(queue_out.size(), 1u);

  TransportRetainedElement* tse_out = dynamic_cast<TransportRetainedElement*>(queue_out.peek());
  ASSERT_NE(tse_out, static_cast<TransportRetainedElement*>(0));
  ASSERT_EQ(tse_out->msg()->capacity(), 5u);
  ASSERT_EQ(tse_out->msg()->cont()->capacity(), 8u);
  ASSERT_EQ(tse_out->msg()->cont()->cont()->capacity(), 16u);

  ASSERT_EQ(dse->get_sample()->reference_count(), 1);
  ASSERT_EQ(tse_out->msg()->reference_count(), 1);
  ASSERT_NE(dse->get_sample(), tse_out->msg());
  ASSERT_NE(dse->get_sample()->data_block(), tse_out->msg()->data_block());

  RemoveAllVisitor rav_out;
  queue_out.accept_remove_visitor(rav_out);

  RemoveAllVisitor rav_in;
  queue_in.accept_remove_visitor(rav_in);

  delete dse;
  lm->stop_tracing();
}

TEST(copy_chain_visitor, simple_duplicate)
{
  ACE_Log_Msg *lm = ACE_LOG_MSG;
  lm->start_tracing();

  OpenDDS::DCPS::Message_Block_Ptr amb(new ACE_Message_Block(5));
  amb->cont(new ACE_Message_Block(8));
  amb->cont()->cont(new ACE_Message_Block(16));

  DataSampleElement* dse = new DataSampleElement(GUID_UNKNOWN, 0, PublicationInstance_rch());
  dse->set_sample(move(amb));

  ASSERT_EQ(dse->get_sample()->reference_count(), 1);

  BasicQueue<TransportQueueElement> queue_in;
  queue_in.put(new TransportSendElement(1, dse));

  ASSERT_EQ(dse->get_sample()->reference_count(), 1);

  BasicQueue<TransportQueueElement> queue_out;
  MessageBlockAllocator mba(1024);
  DataBlockAllocator dba(1024);
  CopyChainVisitor ccv(queue_out, &mba, &dba, true); // duplicate

  queue_in.accept_visitor(ccv);

  ASSERT_EQ(dse->get_sample()->reference_count(), 2);

  ASSERT_EQ(queue_in.size(), queue_out.size());
  ASSERT_EQ(queue_out.size(), 1u);

  TransportRetainedElement* tse_out = dynamic_cast<TransportRetainedElement*>(queue_out.peek());
  ASSERT_NE(tse_out, static_cast<TransportRetainedElement*>(0));
  ASSERT_EQ(tse_out->msg()->capacity(), 5u);
  ASSERT_EQ(tse_out->msg()->cont()->capacity(), 8u);
  ASSERT_EQ(tse_out->msg()->cont()->cont()->capacity(), 16u);

  ASSERT_EQ(dse->get_sample()->reference_count(), 2);
  ASSERT_EQ(tse_out->msg()->reference_count(), 2);
  ASSERT_NE(dse->get_sample(), tse_out->msg());
  ASSERT_EQ(dse->get_sample()->data_block(), tse_out->msg()->data_block());

  RemoveAllVisitor rav_out;
  queue_out.accept_remove_visitor(rav_out);

  RemoveAllVisitor rav_in;
  queue_in.accept_remove_visitor(rav_in);

  delete dse;
  lm->stop_tracing();
}
