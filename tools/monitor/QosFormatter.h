/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef QOSFORMATTER_H
#define QOSFORMATTER_H

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QtCore/QString>
#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

// This is left in the global namespace to avoid issues with the template
// specializations that we use to define the operations for each type.

template<typename QosType>
QString QosToQString( const QosType& value);

#endif /* QOSFORMATTER_H */
