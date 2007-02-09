// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

template <typename Container>
void
TAO::DCPS::ReliableMulticast::detail::PacketHandler::send_many(
  const Container& container,
  const ACE_INET_Addr& dest
  )
{
  for (
    typename Container::const_iterator iter = container.begin();
    iter != container.end();
    ++iter
    )
  {
    TAO::DCPS::ReliableMulticast::detail::PacketHandler::send(
      *iter,
      dest
      );
  }
}
