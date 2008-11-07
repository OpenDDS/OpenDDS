/*
 * $
 */

package org.opendds.jms.resource;

import javax.resource.spi.ResourceAdapter;
import javax.resource.spi.BootstrapContext;
import javax.resource.spi.ResourceAdapterInternalException;
import javax.resource.spi.ActivationSpec;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.resource.ResourceException;
import javax.transaction.xa.XAResource;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ResourceAdapterImpl implements ResourceAdapter {

    public void start(BootstrapContext bootstrapContext) throws ResourceAdapterInternalException {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void stop() {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void endpointActivation(MessageEndpointFactory messageEndpointFactory, ActivationSpec activationSpec) throws ResourceException {
        
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public void endpointDeactivation(MessageEndpointFactory messageEndpointFactory, ActivationSpec activationSpec) {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public XAResource[] getXAResources(ActivationSpec[] activationSpecs) throws ResourceException {
        return new XAResource[0];  //To change body of implemented methods use File | Settings | File Templates.
    }
}
