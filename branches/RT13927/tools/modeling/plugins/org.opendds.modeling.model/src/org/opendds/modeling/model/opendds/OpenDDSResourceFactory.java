package org.opendds.modeling.model.opendds;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.xmi.impl.XMIResourceFactoryImpl;

public class OpenDDSResourceFactory extends XMIResourceFactoryImpl {

	public OpenDDSResourceFactory() {
		super();
	}

	/**
	 * Creates a {@link OpenDDSResource}.
	 * 
	 * @return a new {@link OpenDDSResource}
	 */
	@Override
	public Resource createResource(URI uri) {
		return new OpenDDSResource(uri);
	}
}
