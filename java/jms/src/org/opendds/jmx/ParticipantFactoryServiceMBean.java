/*
 * $Id$
 */

package org.opendds.jmx;

import DDS.DomainParticipantFactory;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface ParticipantFactoryServiceMBean
    extends ServiceMBean, DCPSAttributes, ORBAttributes {

    DomainParticipantFactory getInstance();
}
