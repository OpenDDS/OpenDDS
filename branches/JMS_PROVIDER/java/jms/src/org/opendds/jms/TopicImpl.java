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

    @Override
    public String toString() {
        return null;
    }

    /**
     * TODO, placeholder, to be elaborated.
     * @return The DDS Topic that this JMS TopicImpl represents
     */
    public DDS.Topic extractDDSTopic() {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }
}
