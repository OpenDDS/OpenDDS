/*
 * $Id$
 */

package org.opendds.jms.qos;

import java.util.Properties;

import DDS.PresentationQosPolicyAccessScopeKind;
import DDS.PublisherQos;

import org.opendds.jms.common.util.Logger;
import org.opendds.jms.common.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class PublisherQosPolicy implements QosPolicy<PublisherQos> {
    private static Logger logger = Logger.getLogger(PublisherQosPolicy.class);

    private Properties properties;

    public PublisherQosPolicy(String value) {
        this(PropertiesHelper.valueOf(value));
    }

    public PublisherQosPolicy(Properties properties) {
        this.properties = properties;
    }

    public void setQos(PublisherQos qos) {
        assert qos != null;
        
        PropertiesHelper.Property property;
        PropertiesHelper helper = new PropertiesHelper(properties);

        // GROUP_DATA QosPolicy
        property = helper.find("GROUP_DATA.value");
        if (property.exists()) {
            qos.group_data.value = property.asBytes();
        }

        // PRESENTATION QosPolicy
        property = helper.find("PRESENTATION.access_scope");
        if (property.exists()) {
            if (property.equals("INSTANCE")) {
                qos.presentation.access_scope =
                    PresentationQosPolicyAccessScopeKind.INSTANCE_PRESENTATION_QOS;

            } else if (property.equals("TOPIC")) {
                qos.presentation.access_scope =
                    PresentationQosPolicyAccessScopeKind.TOPIC_PRESENTATION_QOS;

            } else if (property.equals("GROUP")) {
                qos.presentation.access_scope =
                    PresentationQosPolicyAccessScopeKind.GROUP_PRESENTATION_QOS;
            }
        }

        property = helper.find("PRESENTATION.coherent_access");
        if (property.exists()) {
            qos.presentation.coherent_access = property.asBoolean();
        }

        property = helper.find("PRESENTATION.ordered_access");
        if (property.exists()) {
            qos.presentation.ordered_access = property.asBoolean();
        }

        // PARTITION QosPolicy (reserved)
        property = helper.find("PARTITION.name");
        if (property.exists()) {
            logger.warn("PARTITION QosPolicy is reserved for internal use!");
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
