/*
 * $Id$
 */

#ifndef TAO_IC_BE_EXTERN_H
#define TAO_IC_BE_EXTERN_H

#include "dds/Version.h"

#include "ace_compat.h"
#include "be_global.h"

#define TAO_IC_VERSION (DDS_VERSION)

extern BE_GlobalData *be_global;

int BE_init(int &, char **);
void BE_post_init(BE_PI_CONST char **, long);
void BE_version(void);
void BE_abort(void);
void BE_cleanup(void);
void BE_produce(void);

#endif /* TAO_IC_BE_EXTERN_H */
