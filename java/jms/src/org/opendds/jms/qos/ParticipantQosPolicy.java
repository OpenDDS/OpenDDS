/*
 * $Id$
 */

package org.opendds.jms.qos;

import java.util.Properties;

import DDS.DomainParticipantQos;

import org.opendds.jms.common.util.Logger;
import org.opendds.jms.common.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ParticipantQosPolicy implements QosPolicy<DomainParticipantQos> {
    private static Logger logger = Logger.getLogger(ParticipantQosPolicy.class);

    private Properties properties;

    public ParticipantQosPolicy(String value) {
        this(PropertiesHelper.valueOf(value));
    }

    public ParticipantQosPolicy(Properties properties) {
        this.properties = properties;
    }

    public void setQos(DomainParticipantQos qos) {
        assert qos != null;
        
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
            logger.warn("ENTITY_FACTORY QosPolicy is reserved for internal use!");
        }
    }

    @Override
    public String toString() {
        return properties.toString();
    }
}
