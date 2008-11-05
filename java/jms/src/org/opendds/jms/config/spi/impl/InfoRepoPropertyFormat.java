/*
 * $Id$
 */

package org.opendds.jms.config.spi.impl;

import java.util.List;

import org.opendds.jms.config.Configuration;
import org.opendds.jms.config.PropertyWriter;
import org.opendds.jms.management.argv.SvcConfDirective;
import org.opendds.jms.config.properties.InfoRepoProperties;
import org.opendds.jms.config.spi.PropertyFormat;
import org.opendds.jms.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class InfoRepoPropertyFormat extends PropertyFormat {

    protected void format(Configuration config, List<String> args) {
        PropertyWriter writer = new PropertyWriter(config);

        writer.writeIfSet("-a",
            InfoRepoProperties.BIT_LISTEN_ADDRESS);

        writer.writeIfSet("-o",
            InfoRepoProperties.IOR_FILE);

        writer.writeIfSet("-NOBITS",
            InfoRepoProperties.NOBITS);

        writer.writeIfSet("-z",
            InfoRepoProperties.VERBOSE_TRANSPORT_LOGGING);

        writer.writeIfSet("-r",
            InfoRepoProperties.RESURRECT_FROM_FILE);

        writer.writeIfSet("-FederatorConfig",
            InfoRepoProperties.FEDERATOR_CONFIG);

        writer.writeIfSet("-FederationId",
            InfoRepoProperties.FEDERATION_ID);

        writer.writeIfSet("-FederateWith",
            InfoRepoProperties.FEDERATE_WITH);

        //

        String persistentFile =
            config.get(InfoRepoProperties.PERSISTENT_FILE);

        if (!Strings.isEmpty(persistentFile)) {
            SvcConfDirective directive = new SvcConfDirective();

            directive.setServiceName("PersistenceUpdaterSvc");
            directive.addOption("-file");
            directive.addOption(persistentFile);

            directive.writeTo(writer);
        }

        writer.writeTo(args);
    }
}
