/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DDS_TEST_IMPL_H
#define OPENDDS_DCPS_DDS_TEST_IMPL_H

#include <string>

namespace OpenDDS {
    namespace DCPS {
        class TransportClient;
        class EntityImpl;

    };
};

/**
 * @brief A bridge for tests that need access to non-public parts of the transport framework
 *
 */
class DDS_TEST {
public:

    static bool supports(const OpenDDS::DCPS::TransportClient* tc, const std::string& name);
    static bool negotiated(const OpenDDS::DCPS::TransportClient* tc, const std::string& name);

protected:

    DDS_TEST() {
    };

    virtual ~DDS_TEST() {
    };

};

#endif
