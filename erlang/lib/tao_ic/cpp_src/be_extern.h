/*
 * $Id$
 */

#ifndef TAO_IC_BE_EXTERN_H
#define TAO_IC_BE_EXTERN_H

#ifndef ACE_LACKS_PRAGMA_ONCE
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "14a_compat.h"
#include "be_global.h"

extern BE_GlobalData *be_global;

int   BE_init(int &, char **);
void  BE_post_init(BE_PI_CONST char **, long);
void  BE_version(void);
void  BE_produce(void);
void  BE_abort(void);
void  BE_cleanup(void);

#endif /* TAO_IC_BE_EXTERN_H */
