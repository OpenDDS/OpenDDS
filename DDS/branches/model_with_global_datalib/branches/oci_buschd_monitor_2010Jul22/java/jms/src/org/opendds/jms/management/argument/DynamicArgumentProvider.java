/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management.argument;

import java.util.List;

import org.opendds.jms.management.DynamicMBeanSupport;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface DynamicArgumentProvider {

    void setInstance(DynamicMBeanSupport instance);

    void registerAttributes();

    void addArgs(List<String> args) throws Exception;
}
