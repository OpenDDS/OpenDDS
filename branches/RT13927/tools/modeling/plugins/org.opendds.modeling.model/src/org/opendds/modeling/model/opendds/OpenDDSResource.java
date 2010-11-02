package org.opendds.modeling.model.opendds;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.xmi.impl.XMIResourceImpl;

public class OpenDDSResource extends XMIResourceImpl {

	public OpenDDSResource() {
		super();
	}

	public OpenDDSResource(URI uri) {
		super(uri);
	}

	/**
	 * Use UUIDs instead of URI fragment paths. This allow model libraries to be
	 * extended without making fragment paths invalid.
	 * 
	 * @return <code>true</code>, always
	 */
	@Override
	protected boolean useUUIDs() {
		return true;
	}
	
}
