// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_PACKETREMOVEVISTOR_H
#define TAO_DCPS_PACKETREMOVEVISTOR_H

#include  "dds/DCPS/dcps_export.h"
#include  "BasicQueueVisitor_T.h"
#include  "TransportDefs.h"
#include  "TransportReplacedElement.h"

class ACE_Message_Block;


namespace TAO
{

  namespace DCPS
  {



    class TAO_DdsDcps_Export PacketRemoveVisitor : public BasicQueueVisitor<TransportQueueElement>
    {
      public:

        PacketRemoveVisitor(const ACE_Message_Block* sample,
                            ACE_Message_Block*&      unsent_head_block,
                            ACE_Message_Block*       header_block);

        PacketRemoveVisitor(RepoId               pub_id,
                            ACE_Message_Block*&  unsent_head_block,
                            ACE_Message_Block*   header_block);

        virtual ~PacketRemoveVisitor();

        /// The BasicQueue<T>::accept_remove_visitor() method will call
        /// this visit_element() method for each element in the queue.
        virtual int visit_element_ref(TransportQueueElement*& element);

        /// Accessor for the status.  Called after this visitor object has
        /// been passed to BasicQueue<T>::accept_remove_visitor().
        /// status == 1  means the sample was found and removed.
        /// status == 0  means the sample was not found (thus not removed)
        /// status == -1 means a fatal error was encountered.
        int status() const;


      private:

        /// The sample that needs to be removed.
        const ACE_Message_Block* sample_;

        /// The publisher_id of the control samples to be removed.
        RepoId pub_id_;

        /// The head block of the chain of unsent blocks in the packet.
        ACE_Message_Block*& head_;

        /// The packet header block that was duplicate()'d to form the
        /// first block in the packet.
        ACE_Message_Block* header_block_;

        /// Holds the status of our visit.
        int status_;

        /// This is the message block in the chain that corresponds to the
        /// current (non-head) element being visited.
        ACE_Message_Block* current_block_;

        /// This is the message block in the chain that has its cont() set
        /// to the current_block_.
        ACE_Message_Block* previous_block_;

        /// Cached allocator for TransportReplaceElement.
        TransportReplacedElementAllocator replaced_element_allocator_;
    };

  }

}

#if defined (__ACE_INLINE__)
#include "PacketRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_PACKETREMOVEVISTOR_H */
