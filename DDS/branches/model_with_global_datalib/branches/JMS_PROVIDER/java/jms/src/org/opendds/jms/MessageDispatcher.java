/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.Session;

/**
 * @author  Weiqi Gao
 * @version $Revision$
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
            sessionImpl.doAcknowledge();
        }
    }
}