/*
 * $Id$
 */

package org.opendds.jms.jmx;

import DDS.DomainParticipantFactory;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface ParticipantFactoryServiceMBean
    extends ServiceMBean, DCPSAttributes, ORBAttributes {

    DomainParticipantFactory getDomainParticipantFactory();
}
