package org.opendds.modeling.model.opendds.diagram.dcpslib.part;

import org.opendds.modeling.model.qos.QosPolicy;


public class QosPolicyLabeler extends com.ociweb.gmf.dialogs.ObjectLabeler<QosPolicy> {

	@Override
	public String getLabel(QosPolicy feature) {
		return feature.eClass().getName() + ": " + feature.getName();
	}

}
