/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 *
 * This template contains the parsing of all QOS XML setting
 * which the Publisher and the Subscriber have in common.
 *
 * The DDS_QOS_TYPE template attribute can either contain an
 * ::DDS::PublisherQos or an ::DDS::SubscriberQos.
 * These are the IDL representatives.
 *
 * The XML_QOS_TYPE can either contain an ::dds::publisherQos or
 * an ::dds::subscriberQos. These are the XML representatives.
 *
 */
#ifndef QOS_PUBSUB_T_H_
#define QOS_PUBSUB_T_H_

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
class QOS_PubSub_T
{
public:
  QOS_PubSub_T (void);
  ~QOS_PubSub_T (void);

  void read_qos (DDS_QOS_TYPE&, const XML_QOS_TYPE);

};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include "dds/DCPS/QOS_XML_Handler/QOS_PubSub_T.cpp"

#endif /* QOS_PUBSUB_T_H_ */
