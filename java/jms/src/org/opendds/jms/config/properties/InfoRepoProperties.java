/*
 * $Id$
 */

package org.opendds.jms.config.properties;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface InfoRepoProperties {
    String BIT_LISTEN_ADDRESS = "BitListenAddress";
    String IOR_FILE = "IORFile";
    String NOBITS = "NOBITS";
    String VERBOSE_TRANSPORT_LOGGING = "VerboseTransportLogging";
    String PERSISTENT_FILE = "PersistentFile";
    String RESURRECT_FROM_FILE = "ResurrectFromFile";
    String FEDERATOR_CONFIG = "FederatorConfig";
    String FEDERATION_ID = "FederationId";
    String FEDERATE_WITH = "FederateWith";

    String getBitListenAddress();

    void setBitListenAddress(String value);

    String getIORFile();

    void setIORFile(String value);

    Boolean getNOBITS();

    void setNOBITS(Boolean value);

    Boolean getVerboseTransportLogging();

    void setVerboseTransportLogging(Boolean value);

    String getPersistentFile();

    void setPersistentFile(String value);

    Boolean getResurrectFromFile();

    void setResurrectFromFile(Boolean value);

    String getFederatorConfig();

    void setFederatorConfig(String value);

    String getFederationId();

    void setFederationId(String value);

    String getFederateWith();

    void setFederateWith(String value);
}
