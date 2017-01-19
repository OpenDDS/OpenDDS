/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 *
 * Starting point for parsing the DataReader QOS settings.
 */
#ifndef QOS_DATAREADER_T_H_
#define QOS_DATAREADER_T_H_

#include "QOS_DwDrTp_Base_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
class QOS_DataReader_T
  : public QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>
{
public:
  QOS_DataReader_T (void);
  ~QOS_DataReader_T (void);

  void read_qos (DDS_QOS_TYPE&, const XML_QOS_TYPE);

private:
  typedef QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE> DwDrTpBase;
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include "dds/DCPS/QOS_XML_Handler/QOS_DataReader_T.cpp"

#endif /* QOS_DATAREADER_T_H_ */
