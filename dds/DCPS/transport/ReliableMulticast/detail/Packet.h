// -*- C++ -*-
//

#ifndef TAO_DCPS_PACKET_H
#define TAO_DCPS_PACKET_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

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
          typedef unsigned long id_type;

          enum packet_type
          {
            DATA,
            NACK,
            DATA_NOT_AVAILABLE,
            HEARTBEAT
          };

          Packet(
            id_type id = 0,
            const packet_type& type = DATA,
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
            return
              id_ == rhs.id_ &&
              type_ == rhs.type_ &&
              payload_ == rhs.payload_ &&
              nack_begin_ == rhs.nack_begin_ &&
              nack_end_ == rhs.nack_end_;
          }

          id_type id_;
          packet_type type_;
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
