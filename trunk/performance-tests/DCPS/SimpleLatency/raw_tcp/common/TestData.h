#ifndef TESTDATA_H
#define TESTDATA_H

#include "TestException.h"
#include <ace/OS_NS_string.h>
#include <ace/Log_Msg.h>

#define TESTDATA_PACKET_DECL(X) typedef Packet<X> Packet_ ## X

#define TESTDATA_CASE_CREATE_PACKET(X) \
case X: \
  return new PacketHolderImpl<Packet_ ## X >(data_size, new Packet_ ## X ())


namespace TestData
{

  typedef unsigned char ElemType;

  struct Header
  {
    char     publisher_id_;
    unsigned id_;
    unsigned payload_size_;

    Header() : publisher_id_(0), id_(0), payload_size_(0) {}
  };

  template <unsigned DATA_SIZE>
  struct Packet
  {
    Header   header_;
    ElemType payload_[1 << DATA_SIZE];

    Packet()
    {
      header_.payload_size_ = (1 << DATA_SIZE) * sizeof(ElemType);
      ACE_OS::memset(payload_, 0, header_.payload_size_);
    }
  };

  TESTDATA_PACKET_DECL(0);
  TESTDATA_PACKET_DECL(1);
  TESTDATA_PACKET_DECL(2);
  TESTDATA_PACKET_DECL(3);
  TESTDATA_PACKET_DECL(4);
  TESTDATA_PACKET_DECL(5);
  TESTDATA_PACKET_DECL(6);
  TESTDATA_PACKET_DECL(7);
  TESTDATA_PACKET_DECL(8);
  TESTDATA_PACKET_DECL(9);
  TESTDATA_PACKET_DECL(10);
  TESTDATA_PACKET_DECL(11);
  TESTDATA_PACKET_DECL(12);
  TESTDATA_PACKET_DECL(13);
  TESTDATA_PACKET_DECL(14);
  TESTDATA_PACKET_DECL(15);
  TESTDATA_PACKET_DECL(16);
  TESTDATA_PACKET_DECL(17);
  TESTDATA_PACKET_DECL(18);
  TESTDATA_PACKET_DECL(19);
  TESTDATA_PACKET_DECL(20);


  class PacketHolder
  {
    public:

      virtual ~PacketHolder()    {}

      char get_data_size() const { return data_size_; }

      virtual void set_publisher_id(char publisher_id) = 0;
      virtual char get_publisher_id() const = 0;

      virtual void     set_packet_id(unsigned id) = 0;
      virtual unsigned get_packet_id() const = 0;

      virtual unsigned num_bytes() const = 0;
      virtual char*    as_bytes() = 0;


    protected:

      PacketHolder(char data_size) : data_size_(data_size) {}

    private:

      char data_size_;
  };


  template <typename T>
  class PacketHolderImpl : public PacketHolder
  {
    public:

      PacketHolderImpl(char data_size, T* packet)
        : PacketHolder(data_size), packet_(packet)
      {}

      virtual ~PacketHolderImpl()
      { delete packet_; }

      virtual void set_publisher_id(char publisher_id)
      { packet_->header_.publisher_id_ = publisher_id; }

      virtual char get_publisher_id() const
      { return packet_->header_.publisher_id_; }

      virtual void set_packet_id(unsigned id)
      { packet_->header_.id_ = id; }

      virtual unsigned get_packet_id() const
      { return packet_->header_.id_; }

      virtual unsigned num_bytes() const
      { return sizeof(T); }

      virtual char* as_bytes()
      { return (char*)packet_; }


    private:

      T* packet_;
  };


  class PacketFactory
  {
    public:

      static PacketHolder* create_packet(char data_size)
      {
        switch (data_size) {
          TESTDATA_CASE_CREATE_PACKET(0);
          TESTDATA_CASE_CREATE_PACKET(1);
          TESTDATA_CASE_CREATE_PACKET(2);
          TESTDATA_CASE_CREATE_PACKET(3);
          TESTDATA_CASE_CREATE_PACKET(4);
          TESTDATA_CASE_CREATE_PACKET(5);
          TESTDATA_CASE_CREATE_PACKET(6);
          TESTDATA_CASE_CREATE_PACKET(7);
          TESTDATA_CASE_CREATE_PACKET(8);
          TESTDATA_CASE_CREATE_PACKET(9);
          TESTDATA_CASE_CREATE_PACKET(10);
          TESTDATA_CASE_CREATE_PACKET(11);
          TESTDATA_CASE_CREATE_PACKET(12);
          TESTDATA_CASE_CREATE_PACKET(13);
          TESTDATA_CASE_CREATE_PACKET(14);
          TESTDATA_CASE_CREATE_PACKET(15);
          TESTDATA_CASE_CREATE_PACKET(16);
          TESTDATA_CASE_CREATE_PACKET(17);
          TESTDATA_CASE_CREATE_PACKET(18);
          TESTDATA_CASE_CREATE_PACKET(19);
          TESTDATA_CASE_CREATE_PACKET(20);
          default:
            ACE_ERROR((LM_ERROR,
                       "(%P|%t) TestData::PacketFactory::create_packet(): "
                       "Unsupported Data Size: [%d].\n", data_size));
            throw TestException();
        }
      }


    private:

      PacketFactory() {}
  };

}

#endif
