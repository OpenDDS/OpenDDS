/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface ServiceMBean {

    boolean isStarted();

    void start() throws Exception;

    void stop() throws Exception;
}
