/*
 * $Id$
 */

package org.opendds.jms.management;

import javax.naming.Context;
import javax.naming.InitialContext;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Constructor;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.Operation;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
@Description("OpenDDS Destination MBean")
public class Destination extends DynamicMBeanSupport implements ServiceMBean {
    private Log log;

    private boolean active;
    private String jndiName;

    private Context context;

    @Constructor
    public Destination() {}

    @Attribute
    @Description("Destination Name")
    public String getDestination() {
        return requireKeyProperty("destination");
    }

    @Attribute
    @Description("Destination Type")
    public String getType() {
        return requireKeyProperty("type");
    }

    @Attribute
    public boolean isActive() {
        return active;
    }

    @Attribute
    public String getJndiName() {
        return jndiName;
    }

    public void setJndiName(String jndiName) {
        this.jndiName = jndiName;
    }

    @Operation
    public void start() throws Exception {
        if (isActive()) {
            throw new IllegalStateException(getDestination() + " already started!");
        }

        log = LogFactory.getLog(getDestination());
        if (log.isInfoEnabled()) {
            log.info(String.format("Binding to JNDI name '%s'", jndiName));
        }

        // TODO create TopicImpl

        context = new InitialContext();
        context.bind(jndiName, null); // TODO: bind TopicImpl instance

        active = true;
    }

    @Operation
    public void stop() throws Exception {
        if (!isActive()) {
            throw new IllegalStateException(getDestination() + " already stopped!");
        }

        if (log.isInfoEnabled()) {
            log.info(String.format("Unbinding from JNDI name '%s'", jndiName));
        }
        log = null;

        context.unbind(jndiName);
        context = null;

        active = false;
    }
}
