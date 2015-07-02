/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef QOSFORMATTER_H
#define QOSFORMATTER_H

#include <QtCore/QString>

// This is left in the global namespace to avoid issues with the template
// specializations that we use to define the operations for each type.

template<typename QosType>
QString QosToQString( const QosType& value);

#endif /* QOSFORMATTER_H */

