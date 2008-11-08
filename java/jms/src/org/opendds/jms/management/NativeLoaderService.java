/*
 * $Id$
 */

package org.opendds.jms.management;

import java.io.File;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.opendds.jms.loader.NativeLoader;
import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.KeyProperty;
import org.opendds.jms.management.annotation.Operation;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
@Description("OpenDDS Native Loader MBean")
public class NativeLoaderService extends DynamicMBeanSupport implements ServiceMBean {
    private Log log;

    private String service;

    private boolean active;
    private String nativeDirectory;

    private NativeLoader loader;

    @Attribute(readOnly = true)
    public String getService() {
        return service;
    }

    @KeyProperty
    public void setService(String service) {
        this.service = service;
    }

    @Attribute(required = true)
    public String getNativeDirectory() {
        return nativeDirectory;
    }

    public void setNativeDirectory(String nativeDirectory) {
        this.nativeDirectory = nativeDirectory;
    }

    @Attribute
    public NativeLoader getLoader() {
        return loader;
    }

    @Attribute
    public File[] getLoadedLibraries() {
        return loader.getLoadedLibraries();
    }

    @Attribute
    public boolean isActive() {
        return active;
    }

    @Operation
    public void start() throws Exception {
        if (isActive()) {
            throw new IllegalStateException(service + " already started!");
        }

        verify();

        log = LogFactory.getLog(service);
        if (log.isInfoEnabled()) {
            log.info("Loading native libraries from: " + nativeDirectory);
        }

        loader = new NativeLoader(nativeDirectory);
        loader.loadLibraries();

        active = true;
    }

    @Operation
    public void stop() throws Exception {
        if (!isActive()) {
            throw new IllegalStateException(service + " already stopped!");
        }

        loader = null;
        log = null;

        active = false;
    }
}
