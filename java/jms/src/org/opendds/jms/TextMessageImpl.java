/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.JMSException;
import javax.jms.TextMessage;

import OpenDDS.JMS.MessagePayload;

/**
 * @author  Weiqi Gao
 */
public class TextMessageImpl extends AbstractMessageImpl implements TextMessage {
    public TextMessageImpl(SessionImpl sessionImpl) {
        super(sessionImpl);
        initTextBody();
    }

    public TextMessageImpl(MessagePayload messagePayload, int handle, SessionImpl sessionImpl) {
        super(messagePayload, handle, sessionImpl);
        setBodyState(new MessageStateBodyReadOnly(this));
    }

    private void initTextBody() {
        payload.theBody.theTextBody(null);
        setBodyState(new MessageStateWritable());
    }

    public void setText(String string) throws JMSException {
        getBodyState().checkWritable();
        payload.theBody.theTextBody(string);
    }

    public String getText() throws JMSException {
        return payload.theBody.theTextBody();
    }

    protected void doClearBody() {
        initTextBody();
    }
}
