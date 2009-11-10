/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_BUILDCHAINVISTOR_H
#define OPENDDS_DCPS_BUILDCHAINVISTOR_H

#include "dds/DCPS/dcps_export.h"
#include "BasicQueueVisitor_T.h"

class ACE_Message_Block;

namespace OpenDDS {
namespace DCPS {

class TransportQueueElement;

class OpenDDS_Dcps_Export BuildChainVisitor : public BasicQueueVisitor<TransportQueueElement> {
public:

  BuildChainVisitor();
  virtual ~BuildChainVisitor();

  virtual int visit_element(TransportQueueElement* element);

  /// Accessor to extract the chain, leaving the head_ and tail_
  /// set to 0 as a result.
  ACE_Message_Block* chain();

private:

  ACE_Message_Block* head_;
  ACE_Message_Block* tail_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "BuildChainVisitor.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_BUILDCHAINVISTOR_H */
