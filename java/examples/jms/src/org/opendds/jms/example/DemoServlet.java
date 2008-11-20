package org.opendds.jms.example;

import java.io.IOException;
import java.io.PrintWriter;

import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.ServletException;
import javax.servlet.RequestDispatcher;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;
import javax.jms.ConnectionFactory;
import javax.jms.Connection;
import javax.jms.JMSException;
import javax.jms.Destination;
import javax.jms.Session;
import javax.jms.MessageProducer;
import javax.jms.TextMessage;
import javax.jms.MessageConsumer;
import javax.jms.Message;

public class DemoServlet extends HttpServlet {
    protected void doGet(HttpServletRequest httpServletRequest, HttpServletResponse httpServletResponse)
        throws ServletException, IOException {
        processRequest(httpServletRequest, httpServletResponse);
    }

    protected void doPost(HttpServletRequest httpServletRequest, HttpServletResponse httpServletResponse)
        throws ServletException, IOException {
        processRequest(httpServletRequest, httpServletResponse);
    }

    private void processRequest(HttpServletRequest request, HttpServletResponse response) throws IOException, ServletException {
        String toDisplay = "";
        if (shouldSendMessage(request, response)) {
            toDisplay = sendMessage();
        } else {
            toDisplay = receiveMessage();
        }

        request.setAttribute("toDisplay", toDisplay);

        final RequestDispatcher dispatcher = getServletContext().getRequestDispatcher("/demo.jsp");
        dispatcher.forward(request, response);
    }

    private boolean shouldSendMessage(HttpServletRequest request, HttpServletResponse response) {
        final String actionString = request.getParameter("action");
        if ("send".equals(actionString)) {
            return true;
        } else {
            return false;
        }
    }

    private String sendMessage() {
        Context ctx = null;
        Connection connection = null;
        String str = "";
        try {
            ctx = new InitialContext();
            ConnectionFactory connectionFactory = (ConnectionFactory) ctx.lookup("DDS/DefaultConnectionFactory");
            connection = connectionFactory.createConnection();
            Destination destination = (Destination) ctx.lookup("DDS/DefaultTopic");
            final Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
            final MessageProducer producer = session.createProducer(destination);
            final TextMessage textMessage = session.createTextMessage("Hello OpenDDS JMS Provider");
            producer.send(textMessage);
            str = "TextMessage send: " + textMessage.getText();
        } catch (NamingException e) {
            str = e.getMessage();
            e.printStackTrace();
        } catch (JMSException e) {
            str = e.getMessage();
            e.printStackTrace();
        } finally {
            if (connection != null) {
                try {
                    connection.close();
                } catch (JMSException e) {
                    str = e.getMessage();
                    e.printStackTrace();
                }
            }
        }
        return str;
    }

    private String receiveMessage() {
        Context ctx = null;
        Connection connection = null;
        String str = "";
        try {
            ctx = new InitialContext();
            ConnectionFactory connectionFactory = (ConnectionFactory) ctx.lookup("DDS/DefaultConnectionFactory");
            connection = connectionFactory.createConnection();
            Destination destination = (Destination) ctx.lookup("DDS/DefaultTopic");
            final Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
            final MessageConsumer consumer = session.createConsumer(destination, "", true);
            final Message message = consumer.receive();
            TextMessage textMessage = (TextMessage) message;
            str = "TextMessage received: " + textMessage.getText();
        } catch (NamingException e) {
            str = e.getMessage();
            e.printStackTrace();
        } catch (JMSException e) {
            str = e.getMessage();
            e.printStackTrace();
        } finally {
            if (connection != null) {
                try {
                    connection.close();
                } catch (JMSException e) {
                    str = e.getMessage();
                    e.printStackTrace();
                }
            }
        }
        return str;
    }
}
