/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.client;

import javax.naming.InitialContext;
import javax.naming.NamingException;

import org.junit.Test;
import org.junit.remote.annotation.Remote;
import org.junit.remote.runner.RemoteRunner;
import org.junit.runner.RunWith;

/**
 * @author  Steven Stallion
 */
@Remote(endpoint = "http://localhost:8080/opendds-jms-compat/")
@RunWith(RemoteRunner.class)
public class JndiTest {

    @Test
    public void lookupConnectionFactory() throws NamingException {
        InitialContext context = new InitialContext();
        assert context.lookup("DDS/DefaultConnectionFactory") != null;
    }

    @Test
    public void lookupConnectionFactoryWithENC() throws NamingException {
        InitialContext context = new InitialContext();
        assert context.lookup("java:comp/env/jms/ConnectionFactory") != null;
    }

    @Test
    public void lookupDestination() throws NamingException {
        InitialContext context = new InitialContext();
        assert context.lookup("DDS/DefaultTopic") != null;
    }

    @Test
    public void lookupDestinationWithENC() throws NamingException {
        InitialContext context = new InitialContext();
        assert context.lookup("java:comp/env/jms/Topic") != null;
    }
}
