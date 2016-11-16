/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTSENDELEMENT_H
#define OPENDDS_DCPS_TRANSPORTSENDELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "TransportQueueElement.h"
#include "dds/DCPS/DataSampleElement.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportSendElement : public TransportQueueElement {
public:

  static TransportSendElement* alloc(int initial_count,
                                     const DataSampleElement* sample);

  TransportSendElement(int initial_count,
                       const DataSampleElement* sample,
                       TransportSendElementAllocator* allocator = 0);

  virtual ~TransportSendElement();

  /// Accessor for the publisher id.
  virtual RepoId publication_id() const;

  virtual RepoId subscription_id() const;

  /// Accessor for the ACE_Message_Block
  virtual const ACE_Message_Block* msg() const;

  virtual const ACE_Message_Block* msg_payload() const;

  virtual SequenceNumber sequence() const;

  /// Original sample from send listener.
  const DataSampleElement* sample() const;

  virtual bool owned_by_transport();

protected:

  virtual void release_element(bool dropped_by_transport);

private:

  /// This is the actual element that the transport framework was
  /// asked to send.
  const DataSampleElement* element_;

  /// Reference to TransportSendElement allocator.
  TransportSendElementAllocator* allocator_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TransportSendElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTSENDELEMENT_H */
