package org.opendds.jms;

import static org.junit.Assert.*;
import org.junit.Test;
import javax.jms.TextMessage;
import javax.jms.JMSException;

public class TextMessageImplTest {
    @Test
    public void testSetAndGetText() {
        TextMessage textMessage = new TextMessageImpl();
        try {
            final String greeting = "Hello OpenDDS JMS Provider";
            textMessage.setText(greeting);
            assertEquals(greeting, textMessage.getText());
        } catch (JMSException e) {
            fail("Unexpected exception.");
        }
    }
}
