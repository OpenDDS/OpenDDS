// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_TRANSPORT_EXCEPTIONS_H
#define OPENDDS_DCPS_TRANSPORT_EXCEPTIONS_H

namespace OpenDDS
{
  namespace DCPS
  {

    namespace Transport
    {
      class Exception {};
      class NotFound       : public Exception {};
      class Duplicate      : public Exception {};
      class UnableToCreate : public Exception {};
      class MiscProblem    : public Exception {};
      class NotConfigured  : public Exception {};
      class ConfigurationConflict  : public Exception {};
    }

  }  /* namespace DCPS */

}  /* namespace OpenDDS */

#endif  /* OPENDDS_DCPS_TRANSPORT_EXCEPTIONS_H */
