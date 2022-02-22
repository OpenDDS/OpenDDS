#ifndef OPENDDS_INSTALL_LIBRARY_MOVING_OBJECT_H
#define OPENDDS_INSTALL_LIBRARY_MOVING_OBJECT_H

#include <MovingObjectC.h>
#include <opendds_install_test_library_export.h>

opendds_install_test_library_Export void update(MovingObject& object, double time);
opendds_install_test_library_Export void print(const MovingObject& object, const char* name);

#endif
