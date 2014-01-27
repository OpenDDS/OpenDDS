package org.opendds.modeling.sdk.model.GeneratorSpecification;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.xmi.impl.XMIResourceFactoryImpl;

public class GeneratorResourceFactory extends XMIResourceFactoryImpl {

	public GeneratorResourceFactory() {
		super();
	}

	/**
	 * Creates a {@link GeneratorResource}.
	 * 
	 * @return a new {@link GeneratorResource}
	 */
	@Override
	public Resource createResource(URI uri) {
		return new GeneratorResource(uri);
	}

}
