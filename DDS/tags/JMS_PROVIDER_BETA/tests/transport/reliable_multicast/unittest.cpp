#include "dds/DCPS/transport/ReliableMulticast/detail/Packet.h"
#include "dds/DCPS/transport/ReliableMulticast/detail/PacketSerializer.h"
#include "dds/DCPS/transport/ReliableMulticast/detail/Packetizer.h"
#include "dds/DCPS/transport/ReliableMulticast/detail/ReceiverLogic.h"
#include "dds/DCPS/transport/ReliableMulticast/detail/SenderLogic.h"
#include "ace/Auto_Ptr.h"
#include <iostream>
#include <string>
#include <stdexcept>

#define VERIFY(X) verify((X), __FILE__, __LINE__)
#define VERIFY_IN(X,Y) verify_in(X, Y, __FILE__, __LINE__)

namespace
{
  std::string to_string(
    int line
    )
  {
    const std::string nums("0123456789");
    std::string result;

    while (line != 0)
    {
      result = nums.substr(line % 10, 1) + result;
      line /= 10;
    }
    return result;
  }

  void verify(
    bool b,
    const char* file,
    int line
    )
  {
    if (!b)
    {
      throw std::runtime_error(
        "verification failed, " + std::string(file) + ":" + to_string(line)
        );
    }
  }

  template <typename Container, typename Contained> void verify_in(
    Container& container,
    const Contained& item,
    const char* file,
    unsigned long line
    )
  {
    for (
      typename Container::const_iterator iter = container.begin();
      iter != container.end();
      ++iter
      )
    {
      if (*iter == item)
      {
        return;
      }
    }
    verify(false, file, line);
  }

  typedef OpenDDS::DCPS::ReliableMulticast::detail::Packet Packet;
  typedef OpenDDS::DCPS::ReliableMulticast::detail::PacketSerializer PacketSerializer;
  typedef OpenDDS::DCPS::ReliableMulticast::detail::Packetizer Packetizer;
  typedef OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic ReceiverLogic;
  typedef OpenDDS::DCPS::ReliableMulticast::detail::SenderLogic SenderLogic;

  void test_PacketSerializer()
  {
    Packet p1(10, Packet::DATA_NOT_AVAILABLE);
    Packet p2(12, Packet::HEARTBEAT);
    Packet p3(1, Packet::DATA_END_OF_MESSAGE);
    p3.payload_.assign("Test Payload");

    Packet pout;
    PacketSerializer ps;
    size_t size = 0;
    ACE_Auto_Basic_Array_Ptr<char> safe_array;

    safe_array.reset(ps.getBuffer(p1, size));
    char* begin = ps.serializeFromTo(p1, safe_array.get(), size);
    ps.serializeFromTo(begin, size - (begin - safe_array.get()), pout);
    VERIFY(p1 == pout);
    VERIFY(p2 != pout);
    VERIFY(p3 != pout);

    safe_array.reset(ps.getBuffer(p2, size));
    begin = ps.serializeFromTo(p2, safe_array.get(), size);
    ps.serializeFromTo(begin, size - (begin - safe_array.get()), pout);
    VERIFY(p1 != pout);
    VERIFY(p2 == pout);
    VERIFY(p3 != pout);

    safe_array.reset(ps.getBuffer(p3, size));
    begin = ps.serializeFromTo(p3, safe_array.get(), size);
    ps.serializeFromTo(begin, size - (begin - safe_array.get()), pout);
    VERIFY(p1 != pout);
    VERIFY(p2 != pout);
    VERIFY(p3 == pout);
  }

  void test_Packetizer()
  {
    Packetizer packetizer;
    iovec iov[3];
    char buf1[] = "This is a test";
    char buf2[] = "This is another test";
    ACE_Auto_Basic_Array_Ptr<char> buf3_safe_array(new char[Packetizer::MAX_PAYLOAD_SIZE]);
    iov[0].iov_base = buf1;
    iov[0].iov_len = sizeof(buf1) - 1; // no trailing NULL
    iov[1].iov_base = buf2;
    iov[1].iov_len = sizeof(buf2) - 1; // no trailing NULL
    iov[2].iov_base = buf3_safe_array.get();
    iov[2].iov_len = Packetizer::MAX_PAYLOAD_SIZE;
    std::vector<Packet> packets;

    packetizer.packetize(iov, 2, packets);
    VERIFY(packets.size() == 1);
    VERIFY(packets[0].type_ == Packet::DATA_END_OF_MESSAGE);
    VERIFY(packets[0].payload_.size() == strlen("This is a testThis is another test"));
    VERIFY(packets[0].payload_ == "This is a testThis is another test");

    packetizer.packetize(iov, 3, packets);
    VERIFY(packets.size() == 2);
    VERIFY(packets[0].type_ == Packet::DATA_END_OF_MESSAGE);
    VERIFY(packets[0].payload_.size() == Packetizer::MAX_PAYLOAD_SIZE);
    VERIFY(packets[1].type_ == Packet::DATA_END_OF_MESSAGE);
    VERIFY(packets[1].payload_.size() == strlen("This is a testThis is another test"));
  }

  void test_ReceiverLogic_nack_and_delivery(
    unsigned long base
    )
  {
    ReceiverLogic receiver_logic(128, ReceiverLogic::HARD_RELIABILITY);
    std::vector<Packet> nacks;
    std::vector<Packet> delivered;
    Packet pm2na(-2 + base, Packet::DATA_NOT_AVAILABLE);
    Packet pm1(-1 + base);
    Packet p0(0 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p1(1 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p2(2 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p3(3 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p4(4 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p5(5 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p6(6 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p7(7 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p8(8 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p9(9 + base, Packet::DATA_END_OF_MESSAGE);

    receiver_logic.receive(pm2na, nacks, delivered);
    VERIFY(nacks.size() == 0);
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p0, nacks, delivered);
    VERIFY(nacks.size() == 0);
    VERIFY(delivered.size() == 1);
    VERIFY_IN(delivered, Packet(0 + base, Packet::DATA_END_OF_MESSAGE));

    receiver_logic.receive(pm1, nacks, delivered);
    VERIFY(nacks.size() == 0);
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p2, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 2 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p9, nacks, delivered);
    VERIFY(nacks.size() == 2);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 2 + base));
    VERIFY_IN(nacks, Packet(3 + base, Packet::NACK, 3 + base, 9 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(pm2na, nacks, delivered);
    VERIFY(nacks.size() == 2);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 2 + base));
    VERIFY_IN(nacks, Packet(3 + base, Packet::NACK, 3 + base, 9 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p3, nacks, delivered);
    VERIFY(nacks.size() == 2);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 2 + base));
    VERIFY_IN(nacks, Packet(4 + base, Packet::NACK, 4 + base, 9 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p1, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(4 + base, Packet::NACK, 4 + base, 9 + base));
    VERIFY(delivered.size() == 3);
    VERIFY_IN(delivered, Packet(1 + base, Packet::DATA_END_OF_MESSAGE));
    VERIFY_IN(delivered, Packet(2 + base, Packet::DATA_END_OF_MESSAGE));
    VERIFY_IN(delivered, Packet(3 + base, Packet::DATA_END_OF_MESSAGE));

    receiver_logic.receive(p1, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(4 + base, Packet::NACK, 4 + base, 9 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p5, nacks, delivered);
    VERIFY(nacks.size() == 2);
    VERIFY_IN(nacks, Packet(4 + base, Packet::NACK, 4 + base, 5 + base));
    VERIFY_IN(nacks, Packet(6 + base, Packet::NACK, 6 + base, 9 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p4, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(6 + base, Packet::NACK, 6 + base, 9 + base));
    VERIFY(delivered.size() == 2);
    VERIFY_IN(delivered, Packet(4 + base, Packet::DATA_END_OF_MESSAGE));
    VERIFY_IN(delivered, Packet(5 + base, Packet::DATA_END_OF_MESSAGE));

    receiver_logic.receive(p8, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(6 + base, Packet::NACK, 6 + base, 8 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p7, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(6 + base, Packet::NACK, 6 + base, 7 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p6, nacks, delivered);
    VERIFY(nacks.size() == 0);
    VERIFY(delivered.size() == 4);
    VERIFY_IN(delivered, Packet(6 + base, Packet::DATA_END_OF_MESSAGE));
    VERIFY_IN(delivered, Packet(7 + base, Packet::DATA_END_OF_MESSAGE));
    VERIFY_IN(delivered, Packet(8 + base, Packet::DATA_END_OF_MESSAGE));
    VERIFY_IN(delivered, Packet(9 + base, Packet::DATA_END_OF_MESSAGE));
  }

  void test_reliability_failure_common(
    ReceiverLogic& receiver_logic,
    unsigned long base
    )
  {
    std::vector<Packet> nacks;
    std::vector<Packet> delivered;
    Packet p0(0 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p1(1 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p3(3 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p4(4 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p5(5 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p6(6 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p7(7 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p8(8 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p9(9 + base, Packet::DATA_END_OF_MESSAGE);

    receiver_logic.receive(p0, nacks, delivered);
    VERIFY(nacks.size() == 0);
    VERIFY(delivered.size() == 1);
    VERIFY_IN(delivered, Packet(0 + base, Packet::DATA_END_OF_MESSAGE));

    receiver_logic.receive(p9, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 9 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p8, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 8 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p7, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 7 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p6, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 6 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p5, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 5 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p4, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 4 + base));
    VERIFY(delivered.size() == 0);

    receiver_logic.receive(p3, nacks, delivered);
    VERIFY(nacks.size() == 1);
    VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 3 + base));
    VERIFY(delivered.size() == 0);
  }

  void test_ReceiverLogic_hard_reliability_failure(
    unsigned long base,
    bool should_fail
    )
  {
    size_t max_size = 8;
    if (!should_fail)
    {
      max_size = 64;
    }
    ReceiverLogic receiver_logic(max_size, ReceiverLogic::HARD_RELIABILITY);

    test_reliability_failure_common(receiver_logic, base);

    std::vector<Packet> nacks;
    std::vector<Packet> delivered;
    bool failed = false;

    try
    {
      Packet p2(2 + base, Packet::DATA_END_OF_MESSAGE);

      receiver_logic.receive(p2, nacks, delivered);
      VERIFY(nacks.size() == 1);
      VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 2 + base));
      VERIFY(delivered.size() == 0);
    }
    catch(std::exception&)
    {
      failed = true;
    }
    VERIFY(should_fail == failed);
  }

  void test_ReceiverLogic_soft_reliability_failure(
    unsigned long base,
    bool should_fail
    )
  {
    size_t max_size = 8;
    if (!should_fail)
    {
      max_size = 64;
    }
    ReceiverLogic receiver_logic(max_size, ReceiverLogic::SOFT_RELIABILITY);

    test_reliability_failure_common(receiver_logic, base);

    std::vector<Packet> nacks;
    std::vector<Packet> delivered;

    Packet p2(2 + base, Packet::DATA_END_OF_MESSAGE);

    receiver_logic.receive(p2, nacks, delivered);
    if (should_fail)
    {
      VERIFY(nacks.size() == 0);
      VERIFY(delivered.size() == 8);
      VERIFY_IN(delivered, Packet(2 + base, Packet::DATA_END_OF_MESSAGE));
      VERIFY_IN(delivered, Packet(3 + base, Packet::DATA_END_OF_MESSAGE));
      VERIFY_IN(delivered, Packet(4 + base, Packet::DATA_END_OF_MESSAGE));
      VERIFY_IN(delivered, Packet(5 + base, Packet::DATA_END_OF_MESSAGE));
      VERIFY_IN(delivered, Packet(6 + base, Packet::DATA_END_OF_MESSAGE));
      VERIFY_IN(delivered, Packet(7 + base, Packet::DATA_END_OF_MESSAGE));
      VERIFY_IN(delivered, Packet(8 + base, Packet::DATA_END_OF_MESSAGE));
      VERIFY_IN(delivered, Packet(9 + base, Packet::DATA_END_OF_MESSAGE));
    }
    else
    {
      VERIFY(nacks.size() == 1);
      VERIFY_IN(nacks, Packet(1 + base, Packet::NACK, 1 + base, 2 + base));
      VERIFY(delivered.size() == 0);
    }
  }

  void test_ReceiverLogic()
  {
    test_ReceiverLogic_nack_and_delivery(0x00000000);
    test_ReceiverLogic_nack_and_delivery(0x7fffffff);
    test_ReceiverLogic_nack_and_delivery(0xffffffff);
    test_ReceiverLogic_nack_and_delivery(0xfffffffc);

    test_ReceiverLogic_hard_reliability_failure(0x00000000, true);
    test_ReceiverLogic_hard_reliability_failure(0x7fffffff, true);
    test_ReceiverLogic_hard_reliability_failure(0xffffffff, true);
    test_ReceiverLogic_hard_reliability_failure(0xfffffffc, true);

    test_ReceiverLogic_hard_reliability_failure(0x00000000, false);
    test_ReceiverLogic_hard_reliability_failure(0x7fffffff, false);
    test_ReceiverLogic_hard_reliability_failure(0xffffffff, false);
    test_ReceiverLogic_hard_reliability_failure(0xfffffffc, false);

    test_ReceiverLogic_soft_reliability_failure(0x00000000, true);
    test_ReceiverLogic_soft_reliability_failure(0x7fffffff, true);
    test_ReceiverLogic_soft_reliability_failure(0xffffffff, true);
    test_ReceiverLogic_soft_reliability_failure(0xfffffffc, true);

    test_ReceiverLogic_soft_reliability_failure(0x00000000, false);
    test_ReceiverLogic_soft_reliability_failure(0x7fffffff, false);
    test_ReceiverLogic_soft_reliability_failure(0xffffffff, false);
    test_ReceiverLogic_soft_reliability_failure(0xfffffffc, false);
  }

  void test_SenderLogic(
    unsigned long base
    )
  {
    SenderLogic sender_logic(2);
    Packet p0(1000 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p1(-298 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p2(391123 + base, Packet::DATA_END_OF_MESSAGE);
    Packet p0n(0, Packet::NACK, 0, 1);
    Packet p1n(1, Packet::NACK, 1, 2);
    Packet p2n(2, Packet::NACK, 2, 3);
    Packet p02n(0, Packet::NACK, 0, 3);
    std::vector<Packet> delivered;
    std::vector<Packet> redelivered;

    sender_logic.send(p0, delivered);
    VERIFY(delivered.size() == 1);
    VERIFY_IN(delivered, Packet(0, Packet::DATA_END_OF_MESSAGE));

    sender_logic.send(p1, delivered);
    VERIFY(delivered.size() == 1);
    VERIFY_IN(delivered, Packet(1, Packet::DATA_END_OF_MESSAGE));

    sender_logic.receive(p0n, redelivered);
    VERIFY(redelivered.size() == 1);
    VERIFY_IN(redelivered, Packet(0, Packet::DATA_END_OF_MESSAGE));

    sender_logic.send(p2, delivered);
    VERIFY(delivered.size() == 1);
    VERIFY_IN(delivered, Packet(2, Packet::DATA_END_OF_MESSAGE));

    sender_logic.receive(p0n, redelivered);
    VERIFY(redelivered.size() == 1);
    VERIFY_IN(redelivered, Packet(0, Packet::DATA_NOT_AVAILABLE));

    sender_logic.receive(p1n, redelivered);
    VERIFY(redelivered.size() == 1);
    VERIFY_IN(redelivered, Packet(1, Packet::DATA_END_OF_MESSAGE));

    sender_logic.receive(p2n, redelivered);
    VERIFY(redelivered.size() == 1);
    VERIFY_IN(redelivered, Packet(2, Packet::DATA_END_OF_MESSAGE));

    sender_logic.receive(p02n, redelivered);
    VERIFY(redelivered.size() == 3);
    VERIFY_IN(redelivered, Packet(0, Packet::DATA_NOT_AVAILABLE));
    VERIFY_IN(redelivered, Packet(1, Packet::DATA_END_OF_MESSAGE));
    VERIFY_IN(redelivered, Packet(2, Packet::DATA_END_OF_MESSAGE));
  }

  void test_SenderLogic()
  {
    test_SenderLogic(0x00000000);
    test_SenderLogic(0x7fffffff);
    test_SenderLogic(0xffffffff);
    test_SenderLogic(0xfffffffc);
  }
}

int
main(int argc, char* argv[])
{
  ACE_UNUSED_ARG(argc);
  ACE_UNUSED_ARG(argv);

  test_PacketSerializer();
  test_Packetizer();
  test_ReceiverLogic();
  test_SenderLogic();

  return 0;
}
