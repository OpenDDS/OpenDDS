/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTCUSTOMIZEDELEMENT_H
#define OPENDDS_DCPS_TRANSPORTCUSTOMIZEDELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "TransportQueueElement.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TransportSendElement;

class OpenDDS_Dcps_Export TransportCustomizedElement
  : public TransportQueueElement {

public:
  static TransportCustomizedElement* alloc(TransportQueueElement* orig,
                                           bool fragment = false,
                                           ACE_Allocator* allocator = 0);

  virtual RepoId publication_id() const;
  void set_publication_id(const RepoId& id);

  virtual const ACE_Message_Block* msg() const;
  void set_msg(ACE_Message_Block* m);

  virtual const ACE_Message_Block* msg_payload() const;

  virtual SequenceNumber sequence() const;

  virtual bool owned_by_transport() { return false; }

  virtual bool is_fragment() const { return fragment_; }

  const TransportSendElement* original_send_element() const;

protected:
  virtual void release_element(bool dropped_by_transport);

  virtual bool requires_exclusive_packet() const { return exclusive_; }
  void set_requires_exclusive() { exclusive_ = true; }

  void set_fragment() { fragment_ = true; }

  TransportCustomizedElement(TransportQueueElement* orig,
                             bool fragment,
                             ACE_Allocator* allocator);
  virtual ~TransportCustomizedElement();

  ACE_Allocator* allocator() { return allocator_; }

private:
  RepoId subscription_id() const;

  TransportQueueElement* orig_;
  ACE_Message_Block* msg_;
  ACE_Allocator* allocator_;
  RepoId publication_id_;
  bool fragment_, exclusive_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TransportCustomizedElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTCUSTOMIZEDELEMENT_H */
