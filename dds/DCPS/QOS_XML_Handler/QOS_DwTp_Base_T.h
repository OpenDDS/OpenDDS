/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 *
 * This template contains the parsing of all QOS XML setting
 * which the DataWriter and the DataReader have in common.
 *
 * The DDS_QOS_TYPE template attribute can either contain an
 * ::DDS::DataReaderQos or an ::DDS::DataWriterQos.
 * These are the IDL representatives.
 *
 * The XML_QOS_TYPE can either contain an ::dds::datareaderQos or
 * an ::dds::datawriterQos. These are the XML representatives.
 *
 */
#ifndef QOS_DWTP_BASE_T_H_
#define QOS_DWTP_BASE_T_H_

#include "QOS_DwDrTp_Base_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
class QOS_DwTp_Base_T
  : public QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>
{
public:
  QOS_DwTp_Base_T (void);
  ~QOS_DwTp_Base_T (void);

  void read_qos (DDS_QOS_TYPE&, const XML_QOS_TYPE);

private:
  typedef QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE> DwDrTpBase;
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include "dds/DCPS/QOS_XML_Handler/QOS_DwTp_Base_T.cpp"

#endif /* QOS_DWTP_BASE_T_H_ */
