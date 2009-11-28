/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.qos;

import java.io.Serializable;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface QosPolicy<T> extends Serializable {

    void setQos(T t);
}
