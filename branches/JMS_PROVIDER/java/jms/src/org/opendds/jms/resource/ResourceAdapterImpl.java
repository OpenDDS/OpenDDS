/*
 * $Id$
 */

package org.opendds.jms.resource;

import java.io.IOException;

import javax.resource.spi.ActivationSpec;
import javax.resource.spi.BootstrapContext;
import javax.resource.spi.ResourceAdapter;
import javax.resource.spi.ResourceAdapterInternalException;
import javax.resource.spi.endpoint.MessageEndpointFactory;
import javax.transaction.xa.XAResource;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.opendds.jms.Version;
import org.opendds.jms.loader.NativeLoader;
import org.opendds.jms.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ResourceAdapterImpl implements ResourceAdapter {
    private static Log log = LogFactory.getLog(ResourceAdapterImpl.class);

    private boolean loadNativeLibraries;
    private String nativeDirectory;

    public Boolean getLoadNativeLibraries() {
        return loadNativeLibraries;
    }

    public void setLoadNativeLibraries(Boolean loadNativeLibraries) {
        this.loadNativeLibraries = loadNativeLibraries;
    }

    public String getNativeDirectory() {
        return nativeDirectory;
    }

    public void setNativeDirectory(String nativeDirectory) {
        this.nativeDirectory = Strings.expand(nativeDirectory);
    }

    public void start(BootstrapContext context) throws ResourceAdapterInternalException {
        if (log.isInfoEnabled()) {
            Version version = Version.getInstance();
            log.info(String.format("Starting OpenDDS version %s", version.getDDSVersion()));
        }

        if (loadNativeLibraries) {
            try {
                NativeLoader loader = new NativeLoader(nativeDirectory);
                loader.loadLibraries();

            } catch (IOException e) {
                throw new ResourceAdapterInternalException(e);
            }
        }
    }

    public void stop() {}

    public void endpointActivation(MessageEndpointFactory endpointFactory,
                                   ActivationSpec activationSpec) {}

    public void endpointDeactivation(MessageEndpointFactory endpointFactory,
                                     ActivationSpec activationSpec) {}

    public XAResource[] getXAResources(ActivationSpec[] activationSpecs) {
        return null; // Transactions not supported!
    }
}
