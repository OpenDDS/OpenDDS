/*
 * $Id$
 */

package org.opendds.jms.qos;

import java.util.Properties;

import DDS.DataWriterQos;
import DDS.DataWriterQosHolder;

import org.opendds.jms.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DataWriterQosPolicy implements QosPolicy<DataWriterQos> {
    private PropertiesHelper helper;

    public DataWriterQosPolicy(String value) {
        this(PropertiesHelper.valueOf(value));
    }

    public DataWriterQosPolicy(Properties properties) {
        helper = new PropertiesHelper(properties);
    }

    public void setQos(DataWriterQosHolder holder) {
        setQos(holder.value);
    }

    public void setQos(DataWriterQos qos) {
    }
}
