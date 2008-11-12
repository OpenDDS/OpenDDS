/*
 * $Id$
 */

package org.opendds.jms;

import java.util.Properties;

import org.opendds.jms.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Version {
    private static Version instance;

    public synchronized static Version getInstance() {
        if (instance == null) {
            instance = new Version(
                PropertiesHelper.forResource("version.properties"));
        }
        return instance;
    }

    private PropertiesHelper helper;

    protected Version(Properties properties) {
        helper = new PropertiesHelper(properties);
    }

    public String getDDSVersion() {
        return helper.requireProperty("version.dds");
    }

    public int getDDSMajorVersion() {
        return helper.requireIntProperty("version.dds.major");
    }

    public int getDDSMinorVersion() {
        return helper.requireIntProperty("version.dds.minor");
    }

    public String getJCAVersion() {
        return helper.requireProperty("version.jca");
    }

    public int getJCAMajorVersion() {
        return helper.requireIntProperty("version.jca.major");
    }

    public int getJCAMinorVersion() {
        return helper.requireIntProperty("version.jca.minor");
    }

    public String getJMSVersion() {
        return helper.requireProperty("version.jms");
    }

    public int getJMSMajorVersion() {
        return helper.requireIntProperty("version.jms.major");
    }

    public int getJMSMinorVersion() {
        return helper.requireIntProperty("version.jms.minor");
    }
}
