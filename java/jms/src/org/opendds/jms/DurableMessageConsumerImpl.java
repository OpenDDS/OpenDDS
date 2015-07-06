/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.Topic;
import javax.jms.TopicSubscriber;

import org.opendds.jms.persistence.DurableSubscriptionStore;

/**
 * @author  Weiqi Gao
 */
public class DurableMessageConsumerImpl extends MessageConsumerImpl implements TopicSubscriber {
    private boolean noLocal;
    private String name;
    private DurableSubscription durableSubscription;
    private DurableSubscriptionStore durableSubscriptionStore;

    public DurableMessageConsumerImpl(SessionImpl session, String name, Topic topic, String messageSelector, boolean noLocal) throws JMSException {
        super(session, topic, messageSelector, noLocal);
        this.name = name;
        this.noLocal = noLocal;
        ConnectionImpl connectionImpl = session.getOwningConnection();
        String clientID = connectionImpl.getClientID();
        durableSubscription = new DurableSubscription(clientID, name);
        durableSubscriptionStore = connectionImpl.getPersistenceManager().getDurableSubscriptionStore();
    }

    public Topic getTopic() throws JMSException {
        return (Topic) getDestination();
    }

    public boolean getNoLocal() throws JMSException {
        return noLocal;
    }

    @Override
    public void doAcknowledge() throws JMSException {
        super.doAcknowledge();
    }

    @Override
    protected void doDurableAcknowledge(Message message) {
        try {
            durableSubscriptionStore.acknowledge(durableSubscription, message);
        } catch (JMSException e) {
            throw new IllegalStateException(e);
        }
    }

    @Override
    protected boolean isDurableAcknowledged(AbstractMessageImpl message) throws JMSException {
        return durableSubscriptionStore.acknowledged(durableSubscription, message);
    }

    @Override
    protected void doDurableClose() {
        connection.unregisterDurableSubscription(name);
    }
}
