/*
 * $Id$
 */

package org.opendds.esb.actions.routing;

import java.util.concurrent.LinkedBlockingQueue;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.naming.InitialContext;

import org.jboss.soa.esb.ConfigurationException;
import org.jboss.soa.esb.actions.ActionLifecycleException;
import org.jboss.soa.esb.actions.ActionProcessingException;
import org.jboss.soa.esb.actions.routing.AbstractRouter;
import org.jboss.soa.esb.helpers.ConfigTree;
import org.jboss.soa.esb.message.Message;
import org.jboss.soa.esb.util.Util;

import org.opendds.esb.helpers.ConfigTreeHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class JmsRouter extends AbstractRouter {
    private Connection connection;
    private Destination destination;

    private LinkedBlockingQueue<Message> messages =
        new LinkedBlockingQueue<Message>();

    private class RouterThread extends Thread {
        private Session session;
        private MessageProducer producer;

        public RouterThread() throws Exception {
            session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
            producer = session.createProducer(destination);
        }

        @Override
        public void run() {
            try {
                while (!interrupted()) {
                    Message m = messages.take();
                    try {
                        ObjectMessage message = session.createObjectMessage();
                        message.setObject(Util.serialize(m));

                        producer.send(message);

                    } catch (Exception e) {
                        // TODO
                    }
                }

            } catch (InterruptedException e) {}
        }
    }

    private RouterThread routerThread;

    public JmsRouter(ConfigTree config) throws ConfigurationException {
        super(config);
        try {
            InitialContext ctx = new InitialContext();

            ConnectionFactory cf = (ConnectionFactory) ctx.lookup(
                ConfigTreeHelper.requireProperty(config, "connection-factory"));

            connection = cf.createConnection();

            destination = (Destination) ctx.lookup(
                ConfigTreeHelper.requireProperty(config, "dest-name"));

            routerThread = new RouterThread();

        } catch (Exception e) {
            throw new ConfigurationException(e);
        }
    }

    @Override
    public void initialise() throws ActionLifecycleException {
        super.initialise();
        routerThread.start();
    }

    public void route(Object o) throws ActionProcessingException {
        if (!(o instanceof Message)) {
            throw new IllegalArgumentException();
        }

        try {
            messages.put((Message) o);

        } catch (InterruptedException e) {
            // TODO
        }
    }

    @Override
    public void destroy() throws ActionLifecycleException {
        routerThread.interrupt();
        try {
            routerThread.join();

        } catch (InterruptedException e) {}

        super.destroy();
    }
}
