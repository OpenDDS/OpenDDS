/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.resource;

import javax.resource.spi.ActivationSpec;
import javax.resource.spi.BootstrapContext;
import javax.resource.spi.ResourceAdapter;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.transaction.xa.XAResource;

import org.opendds.jms.common.Version;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.common.util.NativeLoader;

/**
 * @author  Steven Stallion
 */
public class ResourceAdapterImpl implements ResourceAdapter {
    private static Logger logger = Logger.getLogger(ResourceAdapterImpl.class);

    private BootstrapContext context;

    public synchronized  void start(BootstrapContext context) {
        this.context = context;

        logger.info("Starting %s", Version.getInstance());
        NativeLoader.bootstrap(); // load native libraries
    }

    public synchronized void stop() {}

    public void endpointActivation(MessageEndpointFactory endpointFactory,
                                   ActivationSpec activationSpec) {

        throw new UnsupportedOperationException();
    }

    public void endpointDeactivation(MessageEndpointFactory endpointFactory,
                                     ActivationSpec activationSpec) {

        throw new UnsupportedOperationException();
    }

    public XAResource[] getXAResources(ActivationSpec[] activationSpecs) {
        return null; // transactions not supported
    }
}
