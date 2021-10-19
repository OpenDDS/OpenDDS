/**
 * @author Marcel Smit (msmit@remedy.nl)
 *
 *
 * Starting point for parsing the Topic QOS settings.
 *
 */
#ifndef OPENDDS_DCPS_QOS_XML_HANDLER_QOS_TOPIC_T_H
#define OPENDDS_DCPS_QOS_XML_HANDLER_QOS_TOPIC_T_H

#include "QOS_DwDrTp_Base_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

template <typename XML_QOS_TYPE, typename DDS_QOS_TYPE>
class QOS_Topic_T
  : public QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE>
{
public:
  QOS_Topic_T ();
  ~QOS_Topic_T ();

  void read_qos (DDS_QOS_TYPE&, const XML_QOS_TYPE);

private:
  typedef QOS_DwDrTp_Base_T<XML_QOS_TYPE, DDS_QOS_TYPE> DwDrTpBase;
};

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include "QOS_Topic_T.cpp"

#endif /* QOS_TOPIC_T_H_ */
