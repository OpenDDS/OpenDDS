package org.opendds.jms;

import javax.jms.TextMessage;
import javax.jms.JMSException;
import java.io.UnsupportedEncodingException;

public class TextMessageImpl extends AbstractMessageImpl implements TextMessage {
    public void setText(String s) throws JMSException {
        try {
            byte[] bytes = s.getBytes("UTF-8");
            body.theTextBody(bytes);
        } catch (UnsupportedEncodingException e) {
            // Can't happen
        }
    }

    public String getText() throws JMSException {
        try {
            byte[] bytes = body.theTextBody();
            return new String(bytes, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            return ""; // Can't happen
        }
    }
}