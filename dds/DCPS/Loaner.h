/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_LOANER_H
#define OPENDDS_DCPS_LOANER_H

#include "dds/Versioned_Namespace.h"
#include "dds/DdsDcpsInfrastructureC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ReceivedDataElement;

class Loaner
{
public:
  Loaner() {}
  virtual ~Loaner() {}

  /**
   * This method provides virtual access to type specific code
   * that is used when loans are automatically returned.
   * The destructor of the sequence supporting zero-copy read calls this
   * method on the datareader that provided the loan.
   *
   * @param seq - The sequence of loaned values.
   *
   * @returns Always RETCODE_OK.
   *
   * thows NONE.
   */
  virtual DDS::ReturnCode_t auto_return_loan(void* seq) = 0;

  virtual void dec_ref_data_element(ReceivedDataElement* r) = 0;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_LOANER_H  */
