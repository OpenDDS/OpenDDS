/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

// This file is here to avoid the use of VPATH.  Since the TAO_IDL
// counterparts are built with a different BE_GlobalData definition, the
// object files built in the TAO_IDL directory are not usable in the
// IFRService.  The BE_GlobalData is larger in TAO_IDL than it is in
// IFRService which causes problems when be_global is deleted.
//  -- Chad Elliott 12/16/2004

// NOTE: this was copied from the $TAO_ROOT/orbsvcs/IFR_Service

#include "TAO_IDL/driver/drv_preproc.cpp"
