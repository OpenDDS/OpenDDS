/*
 * $Id$
 */

package org.opendds.jms.management;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.opendds.jms.DCPSInfoRepo;
import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Constructor;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.KeyProperty;
import org.opendds.jms.management.annotation.Operation;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
@Description("OpenDDS DCPSInfoRepo MBean")
public class DCPSInfoRepoService extends DynamicMBeanSupport implements ServiceMBean {
    private Log log;

    private String service;
    private boolean active;

    private DCPSInfoRepo instance;
    private Thread instanceThread;

    @Constructor
    public DCPSInfoRepoService() {
        // DCPSInfoRepo Dynamic Attributes

        // DCPS Dynamic Attributes

        // ORB Dynamic Attributes
    }

    @KeyProperty
    public void setService(String service) {
        this.service = service;
    }

    @Attribute
    public boolean isActive() {
        return active;
    }

    @Operation
    public void start() throws Exception {
        if (isActive()) {
            throw new IllegalStateException(service + " already started!");
        }

        verify();

        log = LogFactory.getLog(service);
        if (log.isInfoEnabled()) {
            log.info("Starting " + service);
        }

        instance = new DCPSInfoRepo(null); //TODO args

        instanceThread = new Thread(instance, "DCPSInfoRepo");
        instanceThread.start();

        active = true;
    }

    @Operation
    public void stop() throws Exception {
        if (!isActive()) {
            throw new IllegalStateException(service + " already stopped!");
        }

        if (log.isInfoEnabled()) {
            log.info("Stopping " + service);
        }

        instance.shutdown();
        instanceThread.join();

        instance = null;
        instanceThread = null;

        log = null;

        active = false;
    }
}
