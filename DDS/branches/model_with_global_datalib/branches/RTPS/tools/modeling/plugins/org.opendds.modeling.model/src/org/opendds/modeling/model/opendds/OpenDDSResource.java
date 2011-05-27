package org.opendds.modeling.model.opendds;

import java.io.IOException;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

import org.eclipse.emf.ecore.xmi.XMLResource;
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
	
	@SuppressWarnings("unchecked")
	@Override
	public void doSave(OutputStream outputStream, Map<?, ?> options) throws IOException {
		HashMap<Object, Object> hm = (HashMap<Object, Object>)options;
		hm.put(XMLResource.OPTION_KEEP_DEFAULT_CONTENT, Boolean.TRUE);
		super.doSave(outputStream, options);
	}
}
