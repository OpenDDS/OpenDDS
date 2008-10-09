package org.opendds.jms;

import java.io.UnsupportedEncodingException;
import javax.jms.JMSException;
import javax.jms.TextMessage;

public class TextMessageImpl extends AbstractMessageImpl implements TextMessage {
    public void setText(String s) throws JMSException {
        body.theTextBody(s);
    }

    public String getText() throws JMSException {
        return body.theTextBody();
    }
}