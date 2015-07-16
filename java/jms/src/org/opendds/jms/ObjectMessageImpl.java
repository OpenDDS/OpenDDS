/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;

import javax.jms.JMSException;
import javax.jms.ObjectMessage;

import OpenDDS.JMS.MessageBodyKind;
import OpenDDS.JMS.MessagePayload;

import org.opendds.jms.common.ExceptionHelper;

/**
 * @author  Weiqi Gao
 */
public class ObjectMessageImpl extends AbstractMessageImpl implements ObjectMessage {
    public ObjectMessageImpl(SessionImpl sessionImpl) {
        super(sessionImpl);
        initObjectBody();
    }

    public ObjectMessageImpl(MessagePayload messagePayload, int handle, SessionImpl sessionImpl) {
        super(messagePayload, handle, sessionImpl);
        setBodyState(new MessageStateBodyNonWritable(this));
    }

    private void initObjectBody() {
        payload.theBody.theOctetSeqBody(MessageBodyKind.OBJECT_KIND, null);
        setBodyState(new MessageStateWritable());
    }

    public void setObject(Serializable serializable) throws JMSException {
        getBodyState().checkWritable();
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try {
            ObjectOutputStream oos = new ObjectOutputStream(baos);
            oos.writeObject(serializable);
            payload.theBody.theOctetSeqBody(baos.toByteArray());
            oos.close();
        } catch (IOException e) {
            // Can't happen
        }
    }

    public Serializable getObject() throws JMSException {
        final byte[] buf = payload.theBody.theOctetSeqBody();

        if (buf == null) return null;
        ByteArrayInputStream bais = new ByteArrayInputStream(buf);
        try {
            ObjectInputStream ois = new ObjectInputStream(bais);
            final Serializable retVal = (Serializable) ois.readObject();
            ois.close();
            return retVal;

        } catch (Exception e) {
            throw ExceptionHelper.wrap(e);
        }
    }

    protected void doClearBody() {
        initObjectBody();
    }
}
