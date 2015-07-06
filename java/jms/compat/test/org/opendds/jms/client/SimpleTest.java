/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.client;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.naming.Context;
import javax.naming.InitialContext;

import org.junit.Test;
import org.junit.remote.annotation.Remote;
import org.junit.remote.runner.RemoteRunner;
import org.junit.runner.RunWith;

/**
 * @author  Steven Stallion
 */
@Remote(endpoint = "http://localhost:8080/opendds-jms-compat/")
@RunWith(RemoteRunner.class)
public class SimpleTest {

    @Test
    public void localMessage() throws Exception {
        Context context = new InitialContext();

        Topic topic = (Topic) context.lookup("DDS/DefaultTopic");

        ConnectionFactory cf = (ConnectionFactory)
            context.lookup("DDS/DefaultConnectionFactory");

        Connection connection = cf.createConnection();
        try {
            Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);

            MessageProducer producer = session.createProducer(topic);
            MessageConsumer consumer = session.createConsumer(topic);

            Thread.sleep(5000); // wait for association

            producer.send(session.createTextMessage("Hello, World!"));

            TextMessage message = (TextMessage) consumer.receive();
            assert "Hello, World!".equals(message.getText());

        } finally {
            connection.close();
        }
    }
}
