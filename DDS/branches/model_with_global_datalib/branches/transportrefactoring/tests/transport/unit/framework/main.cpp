#include "DummyTransport.h"
#include "dds/DCPS/transport/framework/LinkImpl.h"
#include "dds/DCPS/transport/framework/LinkImplCallback.h"
#include <stdexcept>

#define assertTrue(X) assertTrue_i(X, #X, __FILE__, __LINE__)

namespace
{
  std::string
  toString(unsigned int x)
  {
    const char* nums = "0123456789";
    std::string result;
    do
    {
      result = std::string() + nums[x % 10] + result;
      x /= 10;
    } while (x != 0);
    return result;
  }

  void
  assertTrue_i(bool condition, const std::string test, const char* file, unsigned int line)
  {
    if (!condition)
    {
      std::string errMsg(test);
      errMsg += " at ";
      errMsg += file;
      errMsg += "(";
      errMsg += toString(line);
      errMsg += ")";
      throw std::runtime_error(errMsg);
    }
  }

  void testNoFragmentationSend()
  {
    DummyTransport transport;
    OpenDDS::DCPS::LinkImpl linkImpl(transport, 0);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establishLink");
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
    OpenDDS::DCPS::LinkImpl linkImpl(transport, 800);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establishLink");
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
    OpenDDS::DCPS::LinkImpl linkImpl(transport, 0);

    linkImpl.open(0);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establishLink");
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
    OpenDDS::DCPS::LinkImpl linkImpl(transport, 0);

    linkImpl.open(0);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establishLink");
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

  void testAsynchronousSend()
  {
    DummyTransport transport;
    OpenDDS::DCPS::LinkImpl linkImpl(transport, 0);

    linkImpl.open(0);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establishLink");
    assertTrue(transport.log_[0].second == 1);
    transport.log_.clear();

    transport.setDeferred();

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

    // This message should be enqueued until the first request is resolved
    status = linkImpl.send(testMessage, id);

    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 3);

    assertTrue(transport.log_.size() == 0);

    // Resolve the first request
    linkImpl.sendSucceeded(id - 1);

    // Allow the queue to drain
    ACE_OS::sleep(1);

    // We're still deferring requests, but we got the next message now.
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "send");
    assertTrue(transport.log_[0].second == id);
    transport.log_.clear();

    // In the meantime, send another message out
    status = linkImpl.send(testMessage, id);

    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(id == 4);

    // Of course, the queue hasn't drained yet.
    assertTrue(transport.log_.size() == 0);

    // Now we stop deferring, but we had an outstanding deferred request.
    transport.setDeferred(false);

    // If we send success, the queue will drain and we'll get the remainnig message.
    linkImpl.sendSucceeded(id - 1);

    // Allow the queue to drain
    ACE_OS::sleep(1);

    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "send");
    assertTrue(transport.log_[0].second == id);
    transport.log_.clear();
  }

  void testSendFailure()
  {
    DummyTransport transport;
    OpenDDS::DCPS::LinkImpl linkImpl(transport, 0);

    linkImpl.open(0);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establishLink");
    assertTrue(transport.log_[0].second == 1);
    transport.log_.clear();

    // Make the link report 5 consecutive failures

    transport.setFailTimes(5);

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

  struct DummyCallback
    : public OpenDDS::DCPS::LinkImplCallback
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
    OpenDDS::DCPS::LinkImpl& linkImpl,
    const TransportAPI::Id& requestId,
    size_t sequenceNumber,
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
    iov[0].iov_base = reinterpret_cast<char*>(buffer);
    iov[0].iov_len = 10;
    iov[1].iov_base = reinterpret_cast<char*>(buffer + 10);
    iov[1].iov_len = max_size - 10;
    linkImpl.received(iov, 2);
  }

  void testInOrderReceipt()
  {
    DummyTransport transport;
    OpenDDS::DCPS::LinkImpl linkImpl(transport, 0);
    DummyCallback cb;

    linkImpl.open(0);
    linkImpl.setCallback(&cb);

    assertTrue(transport.log_.empty());

    TransportAPI::BLOB* blob = 0;
    TransportAPI::Status status = linkImpl.connect(blob);
    assertTrue(status.first == TransportAPI::SUCCESS);
    assertTrue(transport.log_.size() == 1);
    assertTrue(transport.log_[0].first == "establishLink");
    assertTrue(transport.log_[0].second == 1);
    transport.log_.clear();

    synthesizeMessage(linkImpl, 1, 0, false);
    synthesizeMessage(linkImpl, 1, 1, false);
    synthesizeMessage(linkImpl, 1, 2, true);

    assertTrue(cb.numCallbacks == 1);
    assertTrue(cb.sizeReceived == (1024-10)*3);
    cb.reset();

    synthesizeMessage(linkImpl, 1, 2, true);

    synthesizeMessage(linkImpl, 2, 0, true);

    synthesizeMessage(linkImpl, 1, 0, false);
    synthesizeMessage(linkImpl, 1, 1, false);
    synthesizeMessage(linkImpl, 1, 2, true);

    assertTrue(cb.numCallbacks == 1);
    assertTrue(cb.sizeReceived == (1024-10));
    cb.reset();

    synthesizeMessage(linkImpl, 0xffffffff, 0, true);
    synthesizeMessage(linkImpl, 0xfffffffe, 0, true);
    synthesizeMessage(linkImpl, 0, 0, true);

    assertTrue(cb.numCallbacks == 2);
    assertTrue(cb.sizeReceived == (1024-10)*2);
    cb.reset();

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
  testAsynchronousSend();
  testSendFailure();
  testInOrderReceipt();

  return 0;
}
