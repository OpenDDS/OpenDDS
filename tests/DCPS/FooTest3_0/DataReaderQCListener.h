/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_QCLISTENER_IMPL
#define DATAREADER_QCLISTENER_IMPL

#include "DataReaderListener.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#ifndef OPENDDS_NO_QUERY_CONDITION

class DataReaderQCListenerImpl
  : public virtual DataReaderListenerImpl {
public:
  DataReaderQCListenerImpl();

  virtual ~DataReaderQCListenerImpl();

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  void set_qc (DDS::QueryCondition_ptr qc);

private:
  DDS::QueryCondition_var qc_;
};

#endif

#endif /* DATAREADER_QCLISTENER_IMPL  */
