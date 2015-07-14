/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management.argument;

import java.util.List;

import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.management.DynamicMBeanSupport;

/**
 * @author  Steven Stallion
 */
public class InfoRepoArguments implements DynamicArgumentProvider {
    public static final String BIT_LISTEN_ADDRESS = "BitListenAddress";
    public static final String IOR_FILE = "IORFile";
    public static final String NOBITS = "NOBITS";
    public static final String VERBOSE_TRANSPORT_LOGGING = "VerboseTransportLogging";
    public static final String PERSISTENT_FILE = "PersistentFile";
    public static final String RESURRECT_FROM_FILE = "ResurrectFromFile";
    public static final String FEDERATOR_CONFIG = "FederatorConfig";
    public static final String FEDERATION_ID = "FederationId";
    public static final String FEDERATE_WITH = "FederateWith";
    public static final String REASSOCIATE_DELAY = "ReassociateDelay";

    private DynamicMBeanSupport instance;

    public void setInstance(DynamicMBeanSupport instance) {
        this.instance = instance;
    }

    public void registerAttributes() {
        instance.registerAttribute(BIT_LISTEN_ADDRESS, String.class);
        instance.registerAttribute(IOR_FILE, String.class);
        instance.registerAttribute(NOBITS, Boolean.class);
        instance.registerAttribute(VERBOSE_TRANSPORT_LOGGING, Boolean.class);
        instance.registerAttribute(PERSISTENT_FILE, String.class);
        instance.registerAttribute(RESURRECT_FROM_FILE, Boolean.class);
        instance.registerAttribute(FEDERATOR_CONFIG, String.class);
        instance.registerAttribute(FEDERATION_ID, String.class);
        instance.registerAttribute(FEDERATE_WITH, String.class);
        instance.registerAttribute(REASSOCIATE_DELAY, Integer.class);
    }

    public void addArgs(List<String> args) throws Exception {
        ArgumentWriter writer = new ArgumentWriter(instance);

        writer.writeIfSet("-a", BIT_LISTEN_ADDRESS);
        writer.writeIfSet("-o", IOR_FILE);
        writer.writeIfSet("-NOBITS", NOBITS);
        writer.writeIfSet("-z", VERBOSE_TRANSPORT_LOGGING);
        writer.writeIfSet("-r", RESURRECT_FROM_FILE);
        writer.writeIfSet("-FederatorConfig", FEDERATOR_CONFIG);
        writer.writeIfSet("-FederationId", FEDERATION_ID);
        writer.writeIfSet("-FederateWith", FEDERATE_WITH);
        writer.writeIfSet("-ReassociateDelay", REASSOCIATE_DELAY);

        String persistentFile =
            (String) instance.getAttribute(PERSISTENT_FILE);

        if (!Strings.isEmpty(persistentFile)) {
            SvcConfDirective directive = new SvcConfDirective();

            directive.setServiceName("PersistenceUpdaterSvc");
            directive.addOptions("-file", persistentFile);

            directive.writeTo(writer);
        }

        writer.writeTo(args);
    }
}
