/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL_H
#define DATAREADER_LISTENER_IMPL_H

#include <tools/modeling/codegen/model/NullReaderListener.h>
#include <ace/Global_Macros.h>

#include <dds/DdsDcpsSubscriptionS.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/Definitions.h>

using OpenDDS::Model::NullReaderListener;

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<NullReaderListener> {
public:
  DataReaderListenerImpl();

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  int sample_count;
};

#endif /* DATAREADER_LISTENER_IMPL_H */
