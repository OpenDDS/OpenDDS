#ifndef RTPSRELAY_WRITER_LISTENER_H_
#define RTPSRELAY_WRITER_LISTENER_H_

#include "AssociationTable.h"
#include "ListenerBase.h"
#include "lib/RelayTypeSupportImpl.h"

namespace RtpsRelay {

class WriterListener : public ListenerBase {
public:
  explicit WriterListener(AssociationTable& association_table);

private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;

  AssociationTable& association_table_;
};

}

#endif // RTPSRELAY_WRITER_LISTENER_H_
