/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.JMSException;
import javax.jms.Topic;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TemporaryTopicImpl implements Topic {

    public String getTopicName() throws JMSException {
        return null;
    }

    public void delete() throws JMSException {
    }

    @Override
    public String toString() {
        return null;
    }
}
