#ifndef RTPSRELAY_WRITER_LISTENER_H_
#define RTPSRELAY_WRITER_LISTENER_H_

#include "AssociationTable.h"
#include "ListenerBase.h"
#include "RelayHandler.h"

namespace RtpsRelay {

class WriterListener : public ListenerBase {
public:
  WriterListener(AssociationTable& association_table,
                 SpdpHandler& spdp_handler);

private:
  void on_data_available(DDS::DataReader_ptr /*reader*/) override;

  AssociationTable& association_table_;
  SpdpHandler& spdp_handler_;
};

}

#endif // RTPSRELAY_WRITER_LISTENER_H_
