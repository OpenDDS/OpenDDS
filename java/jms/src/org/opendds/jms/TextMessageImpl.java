package org.opendds.jms;

import javax.jms.JMSException;
import javax.jms.TextMessage;

public class TextMessageImpl extends AbstractMessageImpl implements TextMessage {
    public TextMessageImpl() {
        initTextBody();
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