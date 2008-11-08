/*
 * $Id$
 */

package org.opendds.jms.resource;

import javax.resource.ResourceException;
import javax.resource.spi.ActivationSpec;
import javax.resource.spi.BootstrapContext;
import javax.resource.spi.ResourceAdapter;
import javax.resource.spi.ResourceAdapterInternalException;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.transaction.xa.XAResource;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ResourceAdapterImpl implements ResourceAdapter {

    public void start(BootstrapContext context) throws ResourceAdapterInternalException {}

    public void stop() {}

    public void endpointActivation(MessageEndpointFactory endpointFactory,
                                   ActivationSpec activationSpec) throws ResourceException {}

    public void endpointDeactivation(MessageEndpointFactory endpointFactory,
                                     ActivationSpec activationSpec) {}

    public XAResource[] getXAResources(ActivationSpec[] activationSpecs) throws ResourceException {
        return null;
    }
}
