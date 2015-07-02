/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.example.helloworld;

import java.io.IOException;

import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.jboss.soa.esb.client.ServiceInvoker;
import org.jboss.soa.esb.message.Message;
import org.jboss.soa.esb.message.format.MessageFactory;

/**
 * @author  Steven Stallion
 */
public class HelloWorldServlet extends HttpServlet {

    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException, ServletException {
        forwardToView(request, response);
    }

    @Override
    protected void doPost(HttpServletRequest request, HttpServletResponse response) throws IOException, ServletException {
        int numberOfSamples = Integer.parseInt(request.getParameter("numberOfSamples"));
        long delay = Long.parseLong(request.getParameter("delay"));

        try {
            Message message = MessageFactory.getInstance().getMessage();
            message.getBody().add("Hello World!");

            ServiceInvoker invoker =
                new ServiceInvoker("OpenDDS", "HelloWorld");

            for (int i = 0; i < numberOfSamples; ++i) {
                invoker.deliverAsync(message);
                Thread.sleep(delay);
            }

        } catch (Exception e) {
            throw new ServletException(e);
        }
        forwardToView(request, response);
    }

    //

    private void forwardToView(ServletRequest request, ServletResponse response) throws IOException, ServletException {
        RequestDispatcher rd = request.getRequestDispatcher("/view.jsp");
        rd.forward(request, response);
    }
}
