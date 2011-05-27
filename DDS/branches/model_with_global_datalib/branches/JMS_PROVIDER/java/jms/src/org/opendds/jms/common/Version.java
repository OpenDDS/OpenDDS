/*
 * $Id$
 */

package org.opendds.jms.common;

import java.util.Properties;

import org.opendds.jms.common.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Version {
    private static Version instance;

    public synchronized static Version getInstance() {
        if (instance == null) {
            instance = new Version(PropertiesHelper.forName("version.properties"));
        }
        return instance;
    }

    private PropertiesHelper helper;

    protected Version(Properties properties) {
        helper = new PropertiesHelper(properties);
    }

    public String getProductName() {
        return "OpenDDS";
    }

    public String getDDSVersion() {
        return helper.require("version.dds").getValue();
    }

    public int getDDSMajorVersion() {
        return helper.require("version.dds.major").asInt();
    }

    public int getDDSMinorVersion() {
        return helper.require("version.dds.minor").asInt();
    }

    public String getJMSVersion() {
        return helper.require("version.jms").getValue();
    }

    public int getJMSMajorVersion() {
        return helper.require("version.jms.major").asInt();
    }

    public int getJMSMinorVersion() {
        return helper.require("version.jms.minor").asInt();
    }

    @Override
    public String toString() {
        return String.format("%s v%s", getProductName(), getDDSVersion());
    }
}
