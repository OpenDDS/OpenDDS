/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.io.Serializable;

import javax.jms.JMSException;
import javax.jms.MessageNotWriteableException;
import javax.jms.ObjectMessage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;
import org.junit.Test;

/**
 * @author  Weiqi Gao
 */
public class ObjectMessageImplTest {
    @Test
    public void testNewlyCreatedObjectMessage() throws JMSException {
        ObjectMessage objectMessage = new ObjectMessageImpl(null);
        assertNull(objectMessage.getObject());
    }

    @Test
    public void testSetAndGetObject() throws JMSException {
        ObjectMessage objectMessage = new ObjectMessageImpl(null);
        Foo foo = new Foo("Hello OpenDDS JMS Provider");
        objectMessage.setObject(foo);
        assertEquals(foo, objectMessage.getObject());
    }

    @Test
    public void testCLearBody() throws JMSException {
        ObjectMessage objectMessage = new ObjectMessageImpl(null);
        final Foo foo = new Foo("Hello OpenDDS JMS Provider");
        objectMessage.setObject(foo);
        assertEquals(foo, objectMessage.getObject());

        objectMessage.clearBody();
        assertNull(objectMessage.getObject());
    }

    @Test
    public void testSetObjectInNonWritableState() throws JMSException {
        ObjectMessageImpl objectMessage = new ObjectMessageImpl(null);
        final Foo foo = new Foo("Hello OpenDDS JMS Provider");
        objectMessage.setObject(foo);
        assertEquals(foo, objectMessage.getObject());
        objectMessage.setBodyState(new MessageStateBodyNonWritable(objectMessage));

        try {
            objectMessage.setObject(new Foo("Goodbye OpenDDS JMS Provider"));
            fail("Should throw");
        } catch (MessageNotWriteableException e) {
            assertEquals("The message is in a body non-writable state", e.getMessage());
        }
    }

    @Test
    public void testClearBodyInNonWritableState() throws JMSException {
        ObjectMessageImpl objectMessage = new ObjectMessageImpl(null);
        final Foo foo = new Foo("Hello OpenDDS JMS Provider");
        objectMessage.setObject(foo);
        assertEquals(foo, objectMessage.getObject());
        objectMessage.setBodyState(new MessageStateBodyNonWritable(objectMessage));

        objectMessage.clearBody();
        assertNull(objectMessage.getObject());

        final Foo foo2 = new Foo("Goodbye OpenDDS JMS Provider");
        objectMessage.setObject(foo2);
        assertEquals(foo2, objectMessage.getObject());
    }

    private static class Foo implements Serializable {
        public String message;

        private Foo(String message) {
            this.message = message;
        }

        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            Foo that = (Foo) o;

            if (message != null ? !message.equals(that.message) : that.message != null) return false;

            return true;
        }

        public int hashCode() {
            return (message != null ? message.hashCode() : 0);
        }
    }
}
