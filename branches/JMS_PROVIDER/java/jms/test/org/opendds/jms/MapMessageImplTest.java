package org.opendds.jms;

import static org.junit.Assert.*;
import org.junit.*;
import javax.jms.MapMessage;
import javax.jms.JMSException;

public class MapMessageImplTest {
    @Test
    public void testSetAndGetMapItems() throws JMSException {
        MapMessage message = new MapMessageImpl();

        message.setBoolean("boolean", true);
        assertEquals(true, message.getBoolean("boolean"));

        message.setBoolean("boolean", false);
        assertEquals(false, message.getBoolean("boolean"));
    }
}
