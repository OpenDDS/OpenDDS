/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.persistence;

import java.io.Serializable;

import javax.jms.JMSException;
import javax.jms.Message;

import org.hibernate.Query;
import org.hibernate.Session;
import org.hibernate.SessionFactory;
import org.hibernate.Transaction;

import org.opendds.jms.DurableSubscription;
import org.opendds.jms.common.ExceptionHelper;
import org.opendds.jms.common.lang.Strings;

/**
 * @author  Steven Stallion
 */
public class DurableSubscriptionStore implements Serializable {
    private SessionFactory sessionFactory;

    public DurableSubscriptionStore(SessionFactory sessionFactory) {
        assert sessionFactory != null;

        this.sessionFactory = sessionFactory;
    }

    public void acknowledge(DurableSubscription subscription, Message message) throws JMSException {
        assert subscription != null;
        assert message != null;

        Session session = sessionFactory.openSession();
        Transaction transaction = null;
        try {
            transaction = session.beginTransaction();

            session.save(new AcknowledgedMessage(subscription, requireMessageID(message)));
            transaction.commit();

        } catch (Exception e) {
            if (transaction != null) {
                transaction.rollback();
            }
            throw ExceptionHelper.wrap(e);

        } finally {
            session.close();
        }
    }

    public boolean acknowledged(DurableSubscription subscription, Message message) throws JMSException {
        assert subscription != null;
        assert message != null;

        Session session = sessionFactory.openSession();
        try {
            Query query = session.createQuery(
                "select 1 from AcknowledgedMessage m "
                + "where m.clientID = :clientID and m.name = :name and m.messageID = :messageID");

            query.setString("clientID", subscription.getClientID());
            query.setString("name", subscription.getName());
            query.setString("messageID", requireMessageID(message));

            return query.uniqueResult() != null;

        } catch (Exception e) {
            throw ExceptionHelper.wrap(e);

        } finally {
            session.close();
        }
    }

    public void unsubscribe(DurableSubscription subscription) throws JMSException {
        assert subscription != null;

        Session session = sessionFactory.openSession();
        Transaction transaction = null;
        try {
            transaction = session.beginTransaction();

            Query query = session.createQuery(
                "delete from AcknowledgedMessage m "
                + "where m.clientID = :clientID and m.name = :name");

            query.setString("clientID", subscription.getClientID());
            query.setString("name", subscription.getName());

            query.executeUpdate();
            transaction.commit();

        } catch (Exception e) {
            if (transaction != null) {
                transaction.rollback();
            }
            throw ExceptionHelper.wrap(e);

        } finally {
            session.close();
        }
    }

    //

    private static String requireMessageID(Message message) throws JMSException {
        String messageID = message.getJMSMessageID();
        if (Strings.isEmpty(messageID)) {
            throw new JMSException("Message ID is required for durable subscriptions!");
        }
        return messageID;
    }
}
