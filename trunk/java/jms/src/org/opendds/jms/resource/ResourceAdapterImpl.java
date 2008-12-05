/*
 * $Id$
 */

package org.opendds.jms.resource;

import java.io.IOException;

import javax.resource.spi.ActivationSpec;
import javax.resource.spi.BootstrapContext;
import javax.resource.spi.ResourceAdapter;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.transaction.xa.XAResource;

import org.opendds.jms.common.Version;
import org.opendds.jms.common.io.Files;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.common.util.NativeLoader;
import org.opendds.jms.common.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ResourceAdapterImpl implements ResourceAdapter {
    private static Logger logger = Logger.getLogger(ResourceAdapterImpl.class);

    static {
        PropertiesHelper.Property property;

        PropertiesHelper helper =
            PropertiesHelper.getSystemPropertiesHelper();

        property = helper.find("opendds.native.load");
        if (property.exists() && property.asBoolean()) {
            property = helper.require("opendds.native.dir");

            String dirName = property.getValue();
            if (!Files.isLibraryPathSet(dirName)) {
                logger.warn("%s is not set in java.library.path!", dirName);
            }

            try {
                NativeLoader loader = new NativeLoader(dirName);
                loader.loadLibraries();

            } catch (IOException e) {
                throw new IllegalStateException(e);
            }
        }
    }

    private BootstrapContext context;

    public void start(BootstrapContext context) {
        this.context = context;

        logger.info("Starting %s", Version.getInstance());
    }

    public void stop() {}

    public void endpointActivation(MessageEndpointFactory endpointFactory,
                                   ActivationSpec activationSpec) {}

    public void endpointDeactivation(MessageEndpointFactory endpointFactory,
                                     ActivationSpec activationSpec) {}

    public XAResource[] getXAResources(ActivationSpec[] activationSpecs) {
        return null; // transactions not supported
    }
}
