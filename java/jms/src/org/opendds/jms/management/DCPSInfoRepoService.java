/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management;

import org.opendds.jms.DCPSInfoRepo;
import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Constructor;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.KeyProperty;
import org.opendds.jms.management.annotation.Operation;
import org.opendds.jms.management.argument.DCPSArguments;
import org.opendds.jms.management.argument.DynamicArguments;
import org.opendds.jms.management.argument.InfoRepoArguments;
import org.opendds.jms.management.argument.ORBArguments;

/**
 * @author  Steven Stallion
 */
@Description("OpenDDS DCPSInfoRepo MBean")
public class DCPSInfoRepoService extends DynamicMBeanSupport implements ServiceMBean {
    private Logger logger;

    private boolean started;
    private String service;
    private DCPSInfoRepo instance;
    private Thread instanceThread;

    private DynamicArguments arguments = new DynamicArguments(this);

    @Constructor
    public DCPSInfoRepoService() {
        arguments.register(new InfoRepoArguments());
        arguments.register(new DCPSArguments());
        arguments.register(new ORBArguments());
    }

    @Attribute(readOnly = true)
    public String getService() {
        return service;
    }

    @KeyProperty
    public void setService(String service) {
        this.service = service;
    }

    @Attribute
    public boolean isStarted() {
        return started;
    }

    @Operation
    public void start() throws Exception {
        if (isStarted()) {
            throw new IllegalStateException(name + " is already started!");
        }

        verify();

        logger = Logger.getLogger(service);
        logger.info("Starting %s", service);

        if (logger.isDebugEnabled()) {
            logger.debug("Initializing with %s", Strings.asIdentity(arguments));
        }
        instance = new DCPSInfoRepo(arguments.toArray());

        instanceThread = new Thread(instance, "DCPSInfoRepo");
        instanceThread.start();

        started = true;
    }

    @Operation
    public void stop() throws Exception {
        if (!isStarted()) {
            throw new IllegalStateException(name + " is already stopped!");
        }

        logger.info("Stopping %s", service);

        instance.shutdown();
        instanceThread.join();

        instance = null;
        instanceThread = null;

        logger = null;

        started = false;
    }
}
