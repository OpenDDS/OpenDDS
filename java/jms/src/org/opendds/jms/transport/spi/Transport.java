/*
 * $Id$
 */

package org.opendds.jms.transport.spi;

import org.opendds.jms.management.argument.SvcConfDirective;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface Transport {

    String getName();

    SvcConfDirective getDirective();
}
