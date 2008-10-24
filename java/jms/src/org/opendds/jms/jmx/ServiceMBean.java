/*
 * $Id$
 */

package org.opendds.jms.jmx;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface ServiceMBean {

    void start() throws Exception;

    void stop() throws Exception;

    void restart() throws Exception;
}
