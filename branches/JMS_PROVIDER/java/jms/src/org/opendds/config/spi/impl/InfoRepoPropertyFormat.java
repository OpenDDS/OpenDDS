/*
 * $Id$
 */

package org.opendds.config.spi.impl;

import java.util.List;

import org.opendds.config.props.InfoRepoProperties;
import org.opendds.config.PropertyWriter;
import org.opendds.config.Configuration;
import org.opendds.config.SvcConfDirective;
import org.opendds.config.spi.PropertyFormat;
import org.opendds.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class InfoRepoPropertyFormat extends PropertyFormat {

    protected void format(Configuration config, List<String> args) {
        PropertyWriter writer = new PropertyWriter(config);

        writer.writeIfSet("-a",
            InfoRepoProperties.BIT_LISTEN_ADDRESS_ATTR);

        writer.writeIfSet("-o",
            InfoRepoProperties.IOR_FILE_ATTR);

        writer.writeIfSet("-NOBITS",
            InfoRepoProperties.NOBITS_ATTR);

        writer.writeIfSet("-z",
            InfoRepoProperties.VERBOSE_TRANSPORT_LOGGING_ATTR);

        writer.writeIfSet("-r",
            InfoRepoProperties.RESURRECT_FROM_FILE_ATTR);

        writer.writeIfSet("-FederatorConfig",
            InfoRepoProperties.FEDERATOR_CONFIG_ATTR);

        writer.writeIfSet("-FederationId",
            InfoRepoProperties.FEDERATION_ID_ATTR);

        writer.writeIfSet("-FederateWith",
            InfoRepoProperties.FEDERATE_WITH_ATTR);

        //

        String persistentFile =
            config.get(InfoRepoProperties.PERSISTENT_FILE_ATTR);

        if (!Strings.isEmpty(persistentFile)) {
            SvcConfDirective directive = new SvcConfDirective();

            directive.setServiceName("PersistenceUpdaterSvc");
            directive.addOption("-file");
            directive.addOption(persistentFile);

            writer.write(directive);
        }

        writer.writeTo(args);
    }
}
