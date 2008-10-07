package org.opendds.jms;

import static org.junit.Assert.*;
import org.junit.Test;
import javax.jms.ObjectMessage;
import javax.jms.JMSException;
import java.io.Serializable;

public class ObjectMessageImplTest {
    @Test
    public void testSetAndGetObject() {
        ObjectMessage objectMessage = new ObjectMessageImpl();
        TestObjectPayload payload = new TestObjectPayload("Hello OpenDDS JMS Provider");
        try {
            objectMessage.setObject(payload);
            assertEquals(payload, objectMessage.getObject());
        } catch (JMSException e) {
            fail("Unexpected exception.");
        }

    }

    private static class TestObjectPayload implements Serializable {
        public String message;

        private TestObjectPayload(String message) {
            this.message = message;
        }

        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            TestObjectPayload that = (TestObjectPayload) o;

            if (message != null ? !message.equals(that.message) : that.message != null) return false;

            return true;
        }

        public int hashCode() {
            return (message != null ? message.hashCode() : 0);
        }
    }
}
