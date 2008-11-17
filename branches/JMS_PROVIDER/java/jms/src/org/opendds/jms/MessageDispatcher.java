package org.opendds.jms;

import javax.jms.Session;

class MessageDispatcher implements Runnable {
    private final AbstractMessageImpl message;
    private final DataReaderHandlePair dataReaderHandlePair;
    private final TopicMessageConsumerImpl consumer;
    private final SessionImpl sessionImpl;

    public MessageDispatcher(AbstractMessageImpl message, DataReaderHandlePair dataReaderHandlePair, TopicMessageConsumerImpl consumer, SessionImpl sessionImpl) {
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