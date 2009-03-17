/*
 * $Id$
 */

#ifndef BE_EXTERN_H
#define BE_EXTERN_H

#include "tao/Version.h"

#include "ace_compat.h"
#include "be_global.h"

#define TAO_IC_VERSION (TAO_VERSION)

extern BE_GlobalData* be_global;

int BE_init(int& argc, char** argv);

void BE_post_init(BE_PI_CONST char** DRV_files, long DRV_nfiles);

void BE_version();

void BE_abort();

void BE_cleanup();

void BE_produce();

#endif /* BE_EXTERN_H */
