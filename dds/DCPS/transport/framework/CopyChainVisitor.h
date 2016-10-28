/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_COPYCHAINVISTOR_H
#define OPENDDS_DCPS_COPYCHAINVISTOR_H

#include "dds/DCPS/dcps_export.h"
#include "BasicQueue_T.h"
#include "TransportRetainedElement.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportQueueElement;

class OpenDDS_Dcps_Export CopyChainVisitor : public BasicQueueVisitor<TransportQueueElement> {
public:

  CopyChainVisitor(
    BasicQueue<TransportQueueElement>& target,
    TransportRetainedElementAllocator* allocator,
    MessageBlockAllocator*             mb_allocator,
    DataBlockAllocator*                db_allocator
  );

  virtual ~CopyChainVisitor();

  virtual int visit_element(TransportQueueElement* element);

  /// Access the status.
  int status() const;

private:
  /// Target queue to fill with copied elements.
  BasicQueue<TransportQueueElement>& target_;

  /// Allocator to create copied elements.
  TransportRetainedElementAllocator* allocator_;
  MessageBlockAllocator* mb_allocator_;
  DataBlockAllocator* db_allocator_;

  /// Status of visitation.
  int status_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "CopyChainVisitor.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_COPYCHAINVISTOR_H */
