#include "DummyTransport.h"
#include "dds/DCPS/transport/framework/LinkImpl.h"
#include "dds/DCPS/transport/framework/LinkImplCallback.h"
#include <stdexcept>

namespace
{
  void
  assertTrue(bool condition)
  {
    if (!condition)
    {
      throw std::runtime_error("failure");
    }
  }

  // We need this class to guarantee that the link exists as long as we have
  // a LinkImpl. See the test code below.
  class LinkGuard
  {
  public:
    LinkGuard(TransportAPI::Transport& transport, TransportAPI::Transport::Link* link)
      : transport_(transport)
      , link_(link)
    {
    }

    ~LinkGuard()
    {
      transport_.destroyLink(link_);
    }

    TransportAPI::Transport::Link& get()
    {
      return *link_;
    }

  private:
    TransportAPI::Transport& transport_;
    TransportAPI::Transport::Link* link_;
  };

  void testNoFragmentationSend()
  {
    DummyTransport transport;
    LinkGuard linkGuard(transport, transport.createLink());
    TAO::DCPS::LinkImpl linkImpl(linkGuard.get(), 0);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establish");
    assertTrue(transport.log_[0].second == 1);
    transport.log_.clear();

    ACE_Message_Block testMessage(1024);
    testMessage.wr_ptr(1024);
    TransportAPI::Id id(0);
    status = linkImpl.send(testMessage, id);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 2);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "send");
    assertTrue(transport.log_[0].second == id);
    transport.log_.clear();
  }

  void testFragmentationSend()
  {
    DummyTransport transport;
    LinkGuard linkGuard(transport, transport.createLink());
    TAO::DCPS::LinkImpl linkImpl(linkGuard.get(), 800);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establish");
    assertTrue(transport.log_[0].second == 1);
    transport.log_.clear();

    ACE_Message_Block testMessage(1024);
    testMessage.wr_ptr(1024);
    TransportAPI::Id id(0);
    status = linkImpl.send(testMessage, id);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 2);
    assertTrue(transport.log_.size() == 2);
    assertTrue(transport.log_[0].first == "send");
    assertTrue(transport.log_[0].second == id);
    assertTrue(transport.log_[1].first == "send");
    assertTrue(transport.log_[1].second == id);
    transport.log_.clear();
  }

  void testBackpressureSend()
  {
    DummyTransport transport;
    LinkGuard linkGuard(transport, transport.createLink());
    TAO::DCPS::LinkImpl linkImpl(linkGuard.get(), 0);

    linkImpl.open(0);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establish");
    assertTrue(transport.log_[0].second == 1);
    transport.log_.clear();

    linkImpl.backPressureChanged(true, TransportAPI::failure_reason());

    ACE_Message_Block testMessage(1024);
    testMessage.wr_ptr(1024);
    TransportAPI::Id id(0);

    status = linkImpl.send(testMessage, id);

    ACE_OS::sleep(1);

    // Due to backpressure, the message should not have been delivered.

    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 2);
    assertTrue(transport.log_.size() == 0);

    status = linkImpl.send(testMessage, id);

    ACE_OS::sleep(1);

    // Due to backpressure, the message should not have been delivered.

    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 3);
    assertTrue(transport.log_.size() == 0);

    linkImpl.backPressureChanged(false, TransportAPI::failure_reason());

    ACE_OS::sleep(1);

    // Backpressure has been relieved, the message should appear

    assertTrue(transport.log_.size() == 2);
    assertTrue(transport.log_[0].first == "send");
    assertTrue(transport.log_[0].second == (id - 1));
    assertTrue(transport.log_[1].first == "send");
    assertTrue(transport.log_[1].second == id);
    transport.log_.clear();

    // This message should go through immediately
    status = linkImpl.send(testMessage, id);

    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 4);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "send");
    assertTrue(transport.log_[0].second == id);
    transport.log_.clear();

    linkImpl.close(0);
  }

  void testDisconnectedSend()
  {
    DummyTransport transport;
    LinkGuard linkGuard(transport, transport.createLink());
    TAO::DCPS::LinkImpl linkImpl(linkGuard.get(), 0);

    linkImpl.open(0);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establish");
    assertTrue(transport.log_[0].second == 1);
    transport.log_.clear();

    linkImpl.disconnected(TransportAPI::failure_reason());

    ACE_Message_Block testMessage(1024);
    testMessage.wr_ptr(1024);
    TransportAPI::Id id(0);

    status = linkImpl.send(testMessage, id);

    ACE_OS::sleep(1);

    // Due to disconnection, the message should not have been delivered.

    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 2);
    assertTrue(transport.log_.size() == 0);

    status = linkImpl.send(testMessage, id);

    ACE_OS::sleep(1);

    // Due to disconnection, the message should not have been delivered.

    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 3);
    assertTrue(transport.log_.size() == 0);

    linkImpl.connected(0);

    ACE_OS::sleep(1);

    // Connection has been re-established, the message should appear

    assertTrue(transport.log_.size() == 2);
    assertTrue(transport.log_[0].first == "send");
    assertTrue(transport.log_[0].second == (id - 1));
    assertTrue(transport.log_[1].first == "send");
    assertTrue(transport.log_[1].second == id);
    transport.log_.clear();

    // This message should go through immediately
    status = linkImpl.send(testMessage, id);

    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 4);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "send");
    assertTrue(transport.log_[0].second == id);
    transport.log_.clear();

    linkImpl.close(0);
  }

  void testSendFailure()
  {
    DummyTransport transport;
    LinkGuard linkGuard(transport, transport.createLink());
    TAO::DCPS::LinkImpl linkImpl(linkGuard.get(), 0);

    linkImpl.open(0);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establish");
    assertTrue(transport.log_[0].second == 1);
    transport.log_.clear();

    // Make the link report 5 consecutive failures

    dynamic_cast<DummyTransport::Link&>(linkGuard.get()).setFailTimes(5);

    ACE_Message_Block testMessage(1024);
    testMessage.wr_ptr(1024);
    TransportAPI::Id id(0);

    status = linkImpl.send(testMessage, id);

    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 2);

    // The data was enqueued and is now delivered

    ACE_OS::sleep(1);

    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "send");
    assertTrue(transport.log_[0].second == id);
    transport.log_.clear();
  }

  void testDeferredSend()
  {
    // TBD
  }

  struct DummyCallback
    : public TAO::DCPS::LinkImplCallback
  {
    DummyCallback()
      : numCallbacks(0)
      , sizeReceived(0)
    {
    }

    virtual void receivedData(const ACE_Message_Block& mb)
    {
      ++numCallbacks;
      sizeReceived += mb.total_length();
    }

    void reset()
    {
      numCallbacks = 0;
      sizeReceived = 0;
    }

    size_t numCallbacks;
    size_t sizeReceived;
  };

  void synthesizeMessage(
    TAO::DCPS::LinkImpl& linkImpl,
    const TransportAPI::Id& requestId,
    size_t sequenceNumber,
    bool beginning,
    bool ending
    )
  {
    const size_t max_size = 1024;
    ACE_Message_Block mb(max_size);
    typedef unsigned char uchar;
    uchar* buffer = reinterpret_cast<uchar*>(mb.rd_ptr());

    buffer[0] = uchar(requestId >> 24);
    buffer[1] = uchar(requestId >> 16);
    buffer[2] = uchar(requestId >> 8);
    buffer[3] = uchar(requestId & 255);
    buffer[4] = (sequenceNumber >> 24) & 0xff;
    buffer[5] = (sequenceNumber >> 16) & 0xff;
    buffer[6] = (sequenceNumber >> 8) & 0xff;
    buffer[7] = sequenceNumber & 255;
    buffer[8] = (ending ? 1 : 0);
    buffer[9] = 0; // buffer[9] is reseved

    mb.wr_ptr(1024);
    iovec iov[2];
    iov[0].iov_base = buffer;
    iov[0].iov_len = 10;
    iov[1].iov_base = buffer + 10;
    iov[1].iov_len = max_size - 10;
    linkImpl.received(iov, 2);
  }

  void testInOrderReceipt()
  {
    DummyTransport transport;
    LinkGuard linkGuard(transport, transport.createLink());
    TAO::DCPS::LinkImpl linkImpl(linkGuard.get(), 0);
    DummyCallback cb;

    linkImpl.open(0);
    linkImpl.setCallback(&cb);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establish");
    assertTrue(transport.log_[0].second == 1);
    transport.log_.clear();

    synthesizeMessage(linkImpl, 1, 0, true, false);
    synthesizeMessage(linkImpl, 1, 1, false, false);
    synthesizeMessage(linkImpl, 1, 2, false, true);

    assertTrue(cb.numCallbacks == 1);
    assertTrue(cb.sizeReceived == (1024-10)*3);
    cb.reset();

    synthesizeMessage(linkImpl, 1, 2, false, true);

    synthesizeMessage(linkImpl, 2, 0, true, true);

    synthesizeMessage(linkImpl, 1, 0, true, false);
    synthesizeMessage(linkImpl, 1, 1, false, false);
    synthesizeMessage(linkImpl, 1, 2, false, true);

    assertTrue(cb.numCallbacks == 1);
    assertTrue(cb.sizeReceived == (1024-10));

    linkImpl.setCallback(0);
    linkImpl.close();
  }
}

int
main(
  int argc,
  char* argv[]
  )
{
  ACE_UNUSED_ARG(argc);
  ACE_UNUSED_ARG(argv);

  testNoFragmentationSend();
  testFragmentationSend();
  testBackpressureSend();
  testDisconnectedSend();
  testSendFailure();
  testDeferredSend();
  testInOrderReceipt();

  return 0;
}
