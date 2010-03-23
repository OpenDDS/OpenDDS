/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RECEIVEDDATASAMPLE_H
#define OPENDDS_DCPS_RECEIVEDDATASAMPLE_H

#include "dds/DCPS/DataSampleHeader.h"
#include "ace/Synch.h"

class ACE_Message_Block;

namespace OpenDDS {
namespace DCPS {

/**
 * @class ReceivedDataSample
 *
 * @brief Holds a data sample received by the transport.
 *
 * This is the type of object that is delivered to the
 * TransportReceiveListener objects by the transport.
 * Note that the data sample header has already been
 * demarshalled by the transport, and the ACE_Message_Block (chain)
 * represents the "data" portion of the sample.
 */
class OpenDDS_Dcps_Export ReceivedDataSample {
public:

  /// Default ctor
  ReceivedDataSample();

  /// Dtor
  ~ReceivedDataSample();

  /// The demarshalled sample header.
  DataSampleHeader header_;

  /// The "data" part (ie, no "header" part) of the sample.
  ACE_Message_Block* sample_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined(__ACE_INLINE__)
#include "ReceivedDataSample.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_RECEIVEDDATASAMPLE_H */
