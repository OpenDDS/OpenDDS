/*
 * $Id$
 */

package org.opendds.config.props;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface InfoRepoProperties {
    String BIT_LISTEN_ADDRESS_ATTR = "BitListenAddress";
    String IOR_FILE_ATTR = "IORFile";
    String NOBITS_ATTR = "NOBITS";
    String VERBOSE_TRANSPORT_LOGGING_ATTR = "VerboseTransportLogging";
    String PERSISTENT_FILE_ATTR = "PersistentFile";
    String RESURRECT_FROM_FILE_ATTR = "ResurrectFromFile";
    String FEDERATOR_CONFIG_ATTR = "FederatorConfig";
    String FEDERATION_ID_ATTR = "FederationId";
    String FEDERATE_WITH_ATTR = "FederateWith";

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
