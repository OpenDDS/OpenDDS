/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.JMSException;

import org.opendds.jms.impl.DDSTopic;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TemporaryTopicImpl implements DDSTopic {

    public String getDestinationName() {
        return null;
    }

    public String getTopicName() throws JMSException {
        return null;
    }

    public void delete() throws JMSException {
    }
}
