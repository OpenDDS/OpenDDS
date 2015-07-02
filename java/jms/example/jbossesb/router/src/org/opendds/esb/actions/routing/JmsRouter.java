/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.esb.actions.routing;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.naming.InitialContext;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.jboss.soa.esb.ConfigurationException;
import org.jboss.soa.esb.actions.ActionLifecycleException;
import org.jboss.soa.esb.actions.routing.AbstractRouter;
import org.jboss.soa.esb.helpers.ConfigTree;
import org.jboss.soa.esb.message.Message;
import org.jboss.soa.esb.util.Util;

import org.opendds.esb.helpers.ConfigTreeHelper;
import org.opendds.esb.helpers.ThreadedQueue;

/**
 * JMS Message Router implementation which focuses on simple, yet
 * efficient processing.  Each sending thread maintains a single set
 * of resources for the lifetime of the router instance.  This ensures
 * resource thrashing does not occur as in other implementations.
 * <p/>
 * A number of features remain to be implemented:
 * <li>
 *  <ul>ESB Message unwrapping (i.e. unwrap)</ul>
 *  <ul>Advanced endpoints (i.e. jndi-context-factory, busidref)</ul>
 *  <ul>JMS QoS policies (i.e. persistent, priority, time-to-live)</ul>
 * </li>
 *
 * @author  Steven Stallion
 */
public class JmsRouter extends AbstractRouter {
    private static Log log = LogFactory.getLog(JmsRouter.class);

    private Connection connection;
    private Destination destination;

    private ThreadedQueue<Message> messages;

    private class MessageListener implements ThreadedQueue.QueueListener<Message> {
        private class MessageContext {
            private Session session;
            private MessageProducer producer;

            public MessageContext(Session session, MessageProducer producer) {
                this.session = session;
                this.producer = producer;
            }

            public Session getSession() {
                return session;
            }

            public MessageProducer getMessageProducer() {
                return producer;
            }
        }

        private ThreadLocal<MessageContext> contextLocal =
            new ThreadLocal<MessageContext>() {
                @Override
                protected MessageContext initialValue() {
                    try {
                        Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
                        return new MessageContext(session, session.createProducer(destination));

                    } catch (JMSException e) {
                        throw new IllegalStateException(e);
                    }
                }
            };

        public void dequeue(Message message) {
            try {
                MessageContext context = contextLocal.get();

                Session session = context.getSession();
                MessageProducer producer = context.getMessageProducer();

                ObjectMessage objMessage = session.createObjectMessage();
                objMessage.setObject(Util.serialize(message));

                producer.send(objMessage);

            } catch (Exception e) {
                log.warn("Unexpected problem routing message: " + e.getMessage(), e);
            }
        }
    }

    public JmsRouter(ConfigTree config) throws ConfigurationException {
        super(config);
        try {
            InitialContext ctx = new InitialContext();

            ConnectionFactory cf = (ConnectionFactory) ctx.lookup(
                ConfigTreeHelper.requireAttribute(config, "connection-factory"));

            connection = cf.createConnection();

            destination = (Destination) ctx.lookup(
                ConfigTreeHelper.requireAttribute(config, "dest-name"));

            messages = new ThreadedQueue<Message>(new MessageListener(),
                ConfigTreeHelper.getAttribute(config, "maxThreads"));

        } catch (Exception e) {
            throw new ConfigurationException(e);
        }
    }

    @Override
    public void initialise() throws ActionLifecycleException {
        log.info("Starting OpenDDS JMS Router");
        if (log.isDebugEnabled()) {
            log.debug("Initializing " + messages.numberOfThreads() + " sending thread(s)");
        }

        super.initialise();
        messages.start();
    }

    public void route(Object o) {
        if (!(o instanceof Message)) {
            throw new IllegalArgumentException();
        }
        messages.enqueue((Message) o);
    }

    @Override
    public void destroy() throws ActionLifecycleException {
        log.info("Stopping OpenDDS JMS Router");

        messages.shutdown();
        if (!messages.isEmpty()) {
            log.warn(messages.size() + " message(s) unsent; consider increasing maxThreads!");
        }

        if (connection != null) {
            try {
                connection.close();

            } catch (JMSException e) {}
        }
        super.destroy();
    }
}
