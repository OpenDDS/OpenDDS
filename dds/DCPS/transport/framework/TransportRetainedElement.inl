/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::TransportRetainedElement::TransportRetainedElement(
    const ACE_Message_Block*           message,
    const RepoId&                      pubId,
    TransportRetainedElementAllocator* allocator
) : TransportQueueElement(1),
    msg_( ACE_Message_Block::duplicate( message)),
    publication_id_( pubId),
    allocator_( allocator)
{
}

ACE_INLINE
OpenDDS::DCPS::TransportRetainedElement::TransportRetainedElement(
  const TransportRetainedElement& source
) : TransportQueueElement(1),
    msg_( ACE_Message_Block::duplicate( source.msg())),
    publication_id_( source.publication_id()),
    allocator_( source.allocator_)
{
  DBG_ENTRY_LVL("TransportRetainedElement","TransportRetainedElement",6);
}

