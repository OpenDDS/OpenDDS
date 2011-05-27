package org.opendds.modeling.diagram.dcpslib.part;

import org.eclipse.emf.ecore.EObject;
import org.opendds.modeling.model.types.DataLib;
import org.opendds.modeling.model.types.Struct;


public class StructLabeler extends com.ociweb.gmf.dialogs.ObjectLabeler<Struct> {

	@Override
	public String getLabel(Struct struct) {
		String structName = "";
		
		// Qualify by owner since a data library may be in the same resource
		// as the element using the Struct.
		EObject owner = struct.eContainer();
		if (owner instanceof DataLib ) {
			structName = ((DataLib) owner).getName() + ".";
		}
		structName += struct.getName();
		return structName;
	}

}
