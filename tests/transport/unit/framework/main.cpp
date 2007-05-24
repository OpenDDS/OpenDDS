#include "DummyTransport.h"
#include "dds/DCPS/transport/framework/LinkImpl.h"
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
  }}

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

  return 0;
}
