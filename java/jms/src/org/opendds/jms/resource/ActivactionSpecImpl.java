/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.resource;

import java.io.Serializable;

import javax.resource.ResourceException;
import javax.resource.spi.ActivationSpec;
import javax.resource.spi.InvalidPropertyException;
import javax.resource.spi.ResourceAdapter;

/**
 * @author  Steven Stallion
 */
public class ActivactionSpecImpl implements ActivationSpec, Serializable {
    private ResourceAdapterImpl adapter;

    public ResourceAdapter getResourceAdapter() {
        return adapter;
    }

    public void setResourceAdapter(ResourceAdapter adapter) throws ResourceException {
        if (adapter != null) {
            throw new IllegalStateException("ResourceAdapter already associated!");
        }

        if (!(adapter instanceof ResourceAdapterImpl)) {
            throw new IllegalArgumentException();
        }
        this.adapter = (ResourceAdapterImpl) adapter;
    }

    public void validate() throws InvalidPropertyException {}
}
