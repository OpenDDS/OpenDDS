/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.transport.spi;

import org.opendds.jms.management.argument.SvcConfDirective;

/**
 * @author  Steven Stallion
 */
public interface Transport {

    String getName();

    SvcConfDirective getDirective();
}
