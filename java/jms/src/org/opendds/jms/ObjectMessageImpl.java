package org.opendds.jms;

import javax.jms.ObjectMessage;
import javax.jms.JMSException;
import java.io.Serializable;
import java.io.ObjectOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ByteArrayInputStream;
import java.io.ObjectInputStream;

public class ObjectMessageImpl extends AbstractMessageImpl implements ObjectMessage {
    public void setObject(Serializable serializable) throws JMSException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try {
            ObjectOutputStream oos = new ObjectOutputStream(baos);
            oos.writeObject(serializable);
            body.theOctetSeqBody(baos.toByteArray());
        } catch (IOException e) {
            // Can't happen
        }
    }

    public Serializable getObject() throws JMSException {
        ByteArrayInputStream bais = new ByteArrayInputStream(body.theOctetSeqBody());
        try {
            ObjectInputStream ois = new ObjectInputStream(bais);
            return (Serializable) ois.readObject();
        } catch (IOException e) {
            return null; // Can't happen
        } catch (ClassNotFoundException e) {
            return null; // TODO
        }
    }
}