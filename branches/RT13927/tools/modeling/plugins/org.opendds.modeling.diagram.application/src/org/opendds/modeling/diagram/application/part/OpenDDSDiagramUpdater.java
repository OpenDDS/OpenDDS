package org.opendds.modeling.diagram.application.part;

import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import org.eclipse.gmf.runtime.notation.View;
import org.opendds.modeling.diagram.application.edit.parts.ApplicationModelEditPart;
import org.opendds.modeling.diagram.application.edit.parts.ApplicationTargetEditPart;
import org.opendds.modeling.diagram.application.edit.parts.ApplicationTargetParticipantsEditPart;
import org.opendds.modeling.diagram.application.providers.OpenDDSElementTypes;

/**
 * @generated
 */
public class OpenDDSDiagramUpdater {

	/**
	 * @generated
	 */
	public static List getSemanticChildren(View view) {
		switch (OpenDDSVisualIDRegistry.getVisualID(view)) {
		case ApplicationModelEditPart.VISUAL_ID:
			return getApplicationModel_1000SemanticChildren(view);
		}
		return Collections.EMPTY_LIST;
	}

	/**
	 * @generated
	 */
	public static List getApplicationModel_1000SemanticChildren(View view) {
		if (!view.isSetElement()) {
			return Collections.EMPTY_LIST;
		}
		OpenDDS.ApplicationModel modelElement = (OpenDDS.ApplicationModel) view
				.getElement();
		List result = new LinkedList();
		for (Iterator it = modelElement.getApplications().iterator(); it
				.hasNext();) {
			OpenDDS.ApplicationTarget childElement = (OpenDDS.ApplicationTarget) it
					.next();
			int visualID = OpenDDSVisualIDRegistry.getNodeVisualID(view,
					childElement);
			if (visualID == ApplicationTargetEditPart.VISUAL_ID) {
				result.add(new OpenDDSNodeDescriptor(childElement, visualID));
				continue;
			}
		}
		return result;
	}

	/**
	 * @generated
	 */
	public static List getContainedLinks(View view) {
		switch (OpenDDSVisualIDRegistry.getVisualID(view)) {
		case ApplicationModelEditPart.VISUAL_ID:
			return getApplicationModel_1000ContainedLinks(view);
		case ApplicationTargetEditPart.VISUAL_ID:
			return getApplicationTarget_2001ContainedLinks(view);
		}
		return Collections.EMPTY_LIST;
	}

	/**
	 * @generated
	 */
	public static List getIncomingLinks(View view) {
		switch (OpenDDSVisualIDRegistry.getVisualID(view)) {
		case ApplicationTargetEditPart.VISUAL_ID:
			return getApplicationTarget_2001IncomingLinks(view);
		}
		return Collections.EMPTY_LIST;
	}

	/**
	 * @generated
	 */
	public static List getOutgoingLinks(View view) {
		switch (OpenDDSVisualIDRegistry.getVisualID(view)) {
		case ApplicationTargetEditPart.VISUAL_ID:
			return getApplicationTarget_2001OutgoingLinks(view);
		}
		return Collections.EMPTY_LIST;
	}

	/**
	 * @generated
	 */
	public static List getApplicationModel_1000ContainedLinks(View view) {
		return Collections.EMPTY_LIST;
	}

	/**
	 * @generated
	 */
	public static List getApplicationTarget_2001ContainedLinks(View view) {
		OpenDDS.ApplicationTarget modelElement = (OpenDDS.ApplicationTarget) view
				.getElement();
		List result = new LinkedList();
		result
				.addAll(getOutgoingFeatureModelFacetLinks_ApplicationTarget_Participants_4001(modelElement));
		return result;
	}

	/**
	 * @generated
	 */
	public static List getApplicationTarget_2001IncomingLinks(View view) {
		return Collections.EMPTY_LIST;
	}

	/**
	 * @generated
	 */
	public static List getApplicationTarget_2001OutgoingLinks(View view) {
		OpenDDS.ApplicationTarget modelElement = (OpenDDS.ApplicationTarget) view
				.getElement();
		List result = new LinkedList();
		result
				.addAll(getOutgoingFeatureModelFacetLinks_ApplicationTarget_Participants_4001(modelElement));
		return result;
	}

	/**
	 * @generated
	 */
	private static Collection getOutgoingFeatureModelFacetLinks_ApplicationTarget_Participants_4001(
			OpenDDS.ApplicationTarget source) {
		Collection result = new LinkedList();
		for (Iterator destinations = source.getParticipants().iterator(); destinations
				.hasNext();) {
			OpenDDS.DomainParticipant destination = (OpenDDS.DomainParticipant) destinations
					.next();
			result.add(new OpenDDSLinkDescriptor(source, destination,
					OpenDDSElementTypes.ApplicationTargetParticipants_4001,
					ApplicationTargetParticipantsEditPart.VISUAL_ID));
		}
		return result;
	}

}
