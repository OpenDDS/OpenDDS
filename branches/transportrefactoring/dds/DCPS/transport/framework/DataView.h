// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_DATAVIEW_H
#define OPENDDS_DCPS_DATAVIEW_H

#include "dds/DCPS/dcps_export.h"
#include "ace/Message_Block.h"
#include <vector>

namespace OpenDDS
{
  namespace DCPS
  {

    class DataView
    {
    public:
      typedef std::vector<std::pair<char*, size_t> > View;

      DataView(ACE_Message_Block& mb, size_t max_size);

      void get(View& packets);

    private:
      ACE_Message_Block& mb_;
      size_t max_size_;
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "DataView.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_DATAVIEW_H */
