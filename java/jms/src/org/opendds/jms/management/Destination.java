/*
 * $
 */

package org.opendds.jms.management;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.Operation;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
@MBean(description = "OpenDDS Destination MBean")
public class Destination extends DynamicMBeanSupport implements ServiceMBean {
    private static Log log = LogFactory.getLog(Destination.class);

    private boolean active;

    private Attributes attributes = new Attributes();

    public Destination() {
        super();

        attributes.register("JndiName", String.class);
    }

    protected Attributes getAttributes() {
        return attributes;
    }

    protected String getDescription() {
        return "OpenDDS Destination Description";
    }

    @Attribute
    public boolean isActive() {
        return active;
    }

    @Attribute
    public String getJndiName() {
        return null;
    }

    public void setJndiName() {

    }

    @Operation
    public void start() throws Exception {
        if (isActive()) {
            throw new IllegalStateException("Destination already started!");
        }

        active = true;
    }

    @Operation
    public void stop() throws Exception {
        if (!isActive()) {
            throw new IllegalStateException("Destination already stopped!");
        }

        active = false;
    }
}
