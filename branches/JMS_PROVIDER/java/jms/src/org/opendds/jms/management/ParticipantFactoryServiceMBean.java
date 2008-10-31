/*
 * $Id$
 */

package org.opendds.jms.management;

import DDS.DomainParticipantFactory;

import org.opendds.jms.config.properties.DCPSProperties;
import org.opendds.jms.config.properties.ORBProperties;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface ParticipantFactoryServiceMBean
    extends ServiceMBean, DCPSProperties, ORBProperties {

    DomainParticipantFactory getInstance();
}
