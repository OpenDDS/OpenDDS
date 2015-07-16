/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.JMSException;
import javax.jms.Session;

/**
 * @author  Weiqi Gao
 */
public class MessageDispatcher implements Runnable {
    private final AbstractMessageImpl message;
    private final DataReaderHandlePair dataReaderHandlePair;
    private final MessageConsumerImpl consumer;
    private final SessionImpl sessionImpl;

    public MessageDispatcher(AbstractMessageImpl message, DataReaderHandlePair dataReaderHandlePair, MessageConsumerImpl consumer, SessionImpl sessionImpl) {
        this.message = message;
        this.dataReaderHandlePair = dataReaderHandlePair;
        this.consumer = consumer;
        this.sessionImpl = sessionImpl;
    }

    public void run() {
        consumer.getMessageListener().onMessage(message);
        sessionImpl.addToUnacknowledged(dataReaderHandlePair, consumer);
        if (sessionImpl.getAcknowledgeMode() != Session.CLIENT_ACKNOWLEDGE) {
            try {
                sessionImpl.doAcknowledge();

            } catch (JMSException e) {
                throw new IllegalStateException(e);
            }
        }
    }
}
