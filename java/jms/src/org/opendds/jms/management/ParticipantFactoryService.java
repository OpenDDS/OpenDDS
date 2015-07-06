/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management;

import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Constructor;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.KeyProperty;
import org.opendds.jms.management.annotation.Operation;
import org.opendds.jms.management.argument.DCPSArguments;
import org.opendds.jms.management.argument.DynamicArguments;
import org.opendds.jms.management.argument.ORBArguments;

import DDS.DomainParticipantFactory;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.DCPS.TheServiceParticipant;

/**
 * @author  Steven Stallion
 */
@Description("OpenDDS DomainParticipantFactory MBean")
public class ParticipantFactoryService extends DynamicMBeanSupport implements ServiceMBean {
    private Logger logger;

    private String service;
    private boolean started;
    private DomainParticipantFactory instance;

    private DynamicArguments arguments = new DynamicArguments(this);

    @Constructor
    public ParticipantFactoryService() {
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

    @Attribute
    public DomainParticipantFactory getInstance() {
        return instance;
    }

    @Operation
    public void start() throws Exception {
        if (isStarted()) {
            throw new IllegalStateException(name + " is already started!");
        }

        verify();

        logger = Logger.getLogger(service);
        logger.info("Initializing %s", service);

        if (logger.isDebugEnabled()) {
            logger.debug("Initializing with %s", Strings.asIdentity(arguments));
        }
        instance = TheParticipantFactory.WithArgs(arguments.toStringSeq());

        if (instance == null) {
            throw new IllegalStateException("Unable to create DomainParticipantFactory; please check logs");
        }

        started = true;
    }

    @Operation
    public void stop() throws Exception {
        if (!isStarted()) {
            throw new IllegalStateException(name + " is already stopped!");
        }

        TheServiceParticipant.shutdown();

        instance = null;
        logger = null;

        started = false;
    }
}
