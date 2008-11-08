/*
 * $Id$
 */

package org.opendds.jms.resource;

import javax.resource.ResourceException;
import javax.resource.spi.ActivationSpec;
import javax.resource.spi.InvalidPropertyException;
import javax.resource.spi.ResourceAdapter;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ActivactionSpecImpl implements ActivationSpec {

    public ResourceAdapter getResourceAdapter() {
        return null;
    }

    public void setResourceAdapter(ResourceAdapter adapter) throws ResourceException {}

    public void validate() throws InvalidPropertyException {}
}
