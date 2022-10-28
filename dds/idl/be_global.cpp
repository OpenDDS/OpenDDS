/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "be_global.h"
#include "be_init.h"

// Referenced through extern via be_extern.h
BE_GlobalData* be_global = 0;

BE_GlobalData::BE_GlobalData()
{
}

BE_GlobalData::~BE_GlobalData()
{
}

void
BE_GlobalData::destroy()
{
  BE_destroy();
}

void
BE_GlobalData::parse_args(long& i, char** av)
{
  BE_parse_args(i, av);
}
