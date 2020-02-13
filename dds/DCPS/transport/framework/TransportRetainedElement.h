/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTRETAINEDELEMENT_H
#define OPENDDS_DCPS_TRANSPORTRETAINEDELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/Message_Block_Ptr.h"
#include "TransportQueueElement.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportRetainedElement;

class OpenDDS_Dcps_Export TransportRetainedElement
  : public TransportQueueElement {
public:
  /// Construct with message block chain and Id values.
  TransportRetainedElement(
    const ACE_Message_Block*           message,
    const RepoId&                      pubId,
    MessageBlockAllocator*             mb_allocator_ = 0,
    DataBlockAllocator*                db_allocator_ = 0
  );

  /// Copy constructor.
  TransportRetainedElement(const TransportRetainedElement& source);

  virtual ~TransportRetainedElement();

  ///{ @name TransportQueueElement methods

  virtual RepoId publication_id() const;

  virtual const ACE_Message_Block* msg() const;
  virtual const ACE_Message_Block* msg_payload() const;

  virtual bool owned_by_transport();

  virtual bool is_retained_replaced() const;

protected:
  virtual void release_element(bool dropped_by_transport);

  ///}

private:
  /// Sample data, if any.
  Message_Block_Ptr msg_;

  /// Originating publication Id, if any.
  RepoId publication_id_;

  /// Cached allocator for DataSampleHeader message block
  MessageBlockAllocator*             mb_allocator_;
  /// Cached allocator for DataSampleHeader data block
  DataBlockAllocator*                db_allocator_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TransportRetainedElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTRETAINEDELEMENT_H */

