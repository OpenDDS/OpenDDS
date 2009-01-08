/*
 * $Id$
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
