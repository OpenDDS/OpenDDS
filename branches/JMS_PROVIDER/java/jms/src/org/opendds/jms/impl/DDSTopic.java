/*
 * $Id$
 */

package org.opendds.jms.impl;

import javax.jms.Topic;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface DDSTopic extends Topic {

    String getDestinationName();
}
