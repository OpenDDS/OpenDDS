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
public class TopicImpl implements Topic {

    public String getTopicName() throws JMSException {
        return null;
    }

    protected String getDestinationName() {
        return null;
    }
}
