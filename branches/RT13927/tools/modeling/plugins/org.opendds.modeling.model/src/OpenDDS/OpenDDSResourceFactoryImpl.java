/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import java.util.Map;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.impl.ResourceFactoryImpl;
import org.eclipse.emf.ecore.xmi.XMIResource;
import org.eclipse.emf.ecore.xmi.XMLResource;

/**
 * <!-- begin-user-doc --> The <b>Resource Factory</b> associated with
 * the package. <!-- end-user-doc -->
 * 
 * @see OpenDDS.OpenDDSResourceImpl
 * @generated
 */
public class OpenDDSResourceFactoryImpl extends ResourceFactoryImpl {
    /**
     * Creates an instance of the resource factory. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public OpenDDSResourceFactoryImpl() {
        super();
    }

    /**
     * Creates an instance of the resource.
     * 
     * @generated NOT
     */
    @Override
    public Resource createResource(URI uri) {
        XMIResource resource = new OpenDDSResourceImpl(uri);

        Map<Object, Object> saveOptions = resource.getDefaultSaveOptions();

        saveOptions.put(XMLResource.OPTION_ENCODING, "UTF-8");
        saveOptions.put(XMLResource.OPTION_KEEP_DEFAULT_CONTENT, true);
        saveOptions.put(XMIResource.OPTION_USE_XMI_TYPE, true);

        return resource;
    }

} // OpenDDSResourceFactoryImpl
