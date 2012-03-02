package org.opendds.modeling.diagram.dcpslib.edit.commands;


import java.util.HashMap;
import java.util.Map;

import org.eclipse.gmf.runtime.emf.type.core.requests.CreateElementRequest;
import com.ociweb.gmf.edit.commands.AddReferenceCommand;

/**
 * @generated NOT
 */
public class TopicTypeReferCommand extends AddReferenceCommand {

	private static final Map<String, String> classNameToRefName =
		new HashMap<String, String>();

	static {
		classNameToRefName.put("Struct", "datatype");
	}


	public TopicTypeReferCommand(CreateElementRequest req) {
		super(req, classNameToRefName);
	}

}
