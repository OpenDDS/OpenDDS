/*
 * $Id$
 */

package org.opendds.jms.management;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface ServiceMBean {

    boolean isActive();

    void start() throws Exception;

    void stop() throws Exception;
}
