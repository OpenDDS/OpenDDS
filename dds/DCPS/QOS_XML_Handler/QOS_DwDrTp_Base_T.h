/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 *
 * This template contains the parsing of all QOS XML setting
 * which the DataWriter, the DataReader, and the Topic Qos have
 * in common.
 * The DDS_QOS_TYPE template attribute can either contain an
 * ::DDS::DataReaderQos, an ::DDS::DataWriterQos, or an
 * ::DDS::TopicQos. These are the IDL representatives.
 *
 * The XML_QOS_TYPE can either contain an ::dds::datareaderQos,
 * an ::dds::datawriterQos, or an ::dds::topicQos. These are the
 * XML representatives.
 *
 */
#ifndef QOS_DWDRTP_BASE_T_H_
#define QOS_DWDRTP_BASE_T_H_

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
class QOS_DwDrTp_Base_T
{
public:
  QOS_DwDrTp_Base_T (void);
  ~QOS_DwDrTp_Base_T (void);

  void read_qos (DDS_QOS_TYPE&, const XML_QOS_TYPE);
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include "dds/DCPS/QOS_XML_Handler/QOS_DwDrTp_Base_T.cpp"

#endif /* QOS_DWDRTP_BASE_T_H_ */
