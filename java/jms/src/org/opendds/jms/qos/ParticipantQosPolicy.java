/*
 * $Id$
 */

package org.opendds.jms.qos;

import java.util.Properties;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import DDS.DomainParticipantQos;

import org.opendds.jms.common.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ParticipantQosPolicy implements QosPolicy<DomainParticipantQos> {
    private static Log log = LogFactory.getLog(ParticipantQosPolicy.class);

    private Properties properties;

    public ParticipantQosPolicy(String value) {
        this(PropertiesHelper.valueOf(value));
    }

    public ParticipantQosPolicy(Properties properties) {
        this.properties = properties;
    }

    public void setQos(DomainParticipantQos qos) {
        if (log.isDebugEnabled()) {
            log.debug(String.format("Configuring %s %s", qos, PropertiesHelper.valueOf(properties)));
        }
        
        PropertiesHelper.Property property;

        PropertiesHelper helper = new PropertiesHelper(properties);

        // USER_DATA QosPolicy
        property = helper.find("USER_DATA.value");
        if (property.exists()) {
            qos.user_data.value = property.asBytes();
        }

        // ENTITY_FACTORY QosPolicy (reserved)
        property = helper.find("ENTITY_FACTORY.autoenable_created_entities");
        if (property.exists()) {
            log.warn("ENTITY_FACTORY QosPolicy is reserved for internal use!");
        }
    }
}
