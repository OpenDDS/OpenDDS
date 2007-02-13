// -*- C++ -*-
//

#ifndef TAO_DCPS_PACKET_H
#define TAO_DCPS_PACKET_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Basic_Types.h"
#include <string>

namespace TAO
{

  namespace DCPS
  {

    namespace ReliableMulticast
    {

      namespace detail
      {

        struct Packet
        {
          typedef ACE_UINT32 id_type;

          enum PacketType
          {
            DATA,
            NACK,
            DATA_NOT_AVAILABLE,
            HEARTBEAT
          };

          Packet(
            id_type id = 0,
            const PacketType& type = DATA,
            id_type begin = 0,
            id_type end = 0
            )
            : id_(id)
            , type_(type)
            , nack_begin_(begin)
            , nack_end_(end)
          {
          }

          bool operator<(
            const Packet& rhs
            ) const
          {
            return (id_ < rhs.id_) || (type_ < rhs.type_);
          }

          bool operator==(
            const Packet& rhs
            ) const
          {
            bool ok =
              id_ == rhs.id_ &&
              type_ == rhs.type_;
            if (type_ == NACK)
            {
              ok &=
                (nack_begin_ == rhs.nack_begin_) &&
                (nack_end_ == rhs.nack_end_);
            }
            else if (type_ == DATA)
            {
              ok &= payload_ == rhs.payload_;
            }
            return ok;
          }

          bool operator!=(
            const Packet& rhs
            ) const
          {
            return !(*this == rhs);
          }

          id_type id_;
          PacketType type_;
          id_type nack_begin_;
          id_type nack_end_;
          std::string payload_;
        };

      } /* namespace detail */

    } /* namespace ReliableMulticast */

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "Packet.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_PACKET_H */
