/*
 * $Id$
 */

package org.opendds.jms.jmx.config.spi.impl;

import java.util.List;

import org.opendds.jms.jmx.InfoRepoAttributes;
import org.opendds.jms.jmx.config.AttributeWriter;
import org.opendds.jms.jmx.config.Attributes;
import org.opendds.jms.jmx.config.SvcConfDirective;
import org.opendds.jms.jmx.config.spi.AttributeFormat;
import org.opendds.jms.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class InfoRepoAttributeFormat extends AttributeFormat {

    protected void format(Attributes attributes, List<String> args) {
        AttributeWriter writer = new AttributeWriter(attributes);

        writer.writeIfSet("-a",
            InfoRepoAttributes.BIT_LISTEN_ADDRESS_ATTR);

        writer.writeIfSet("-o",
            InfoRepoAttributes.IOR_FILE_ATTR);

        writer.writeIfSet("-NOBITS",
            InfoRepoAttributes.NOBITS_ATTR);

        writer.writeIfSet("-z",
            InfoRepoAttributes.VERBOSE_TRANSPORT_LOGGING_ATTR);

        writer.writeIfSet("-r",
            InfoRepoAttributes.RESURRECT_FROM_FILE_ATTR);

        writer.writeIfSet("-FederatorConfig",
            InfoRepoAttributes.FEDERATOR_CONFIG_ATTR);

        writer.writeIfSet("-FederationId",
            InfoRepoAttributes.FEDERATION_ID_ATTR);

        writer.writeIfSet("-FederateWith",
            InfoRepoAttributes.FEDERATE_WITH_ATTR);

        //

        String persistentFile =
            attributes.get(InfoRepoAttributes.PERSISTENT_FILE_ATTR);

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
