/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management.argument;

import java.util.List;

import org.opendds.jms.management.DynamicMBeanSupport;

/**
 * @author  Steven Stallion
 */
public interface DynamicArgumentProvider {

    void setInstance(DynamicMBeanSupport instance);

    void registerAttributes();

    void addArgs(List<String> args) throws Exception;
}
