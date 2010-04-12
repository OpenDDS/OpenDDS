package org.opendds.modeling.diagram.application.part;

import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.gmf.runtime.notation.Diagram;
import org.eclipse.gmf.runtime.notation.View;
import org.opendds.modeling.diagram.application.edit.parts.ApplicationModelEditPart;
import org.opendds.modeling.diagram.application.edit.parts.ApplicationTargetEditPart;

/**
 * This registry is used to determine which type of visual object should be
 * created for the corresponding Diagram, Node, ChildNode or Link represented by
 * a domain model object.
 * 
 * @generated
 */
public class OpenDDSVisualIDRegistry {

	/**
	 * @generated
	 */
	private static final String DEBUG_KEY = "org.opendds.modeling.diagram.application/debug/visualID"; //$NON-NLS-1$

	/**
	 * @generated
	 */
	public static int getVisualID(View view) {
		if (view instanceof Diagram) {
			if (ApplicationModelEditPart.MODEL_ID.equals(view.getType())) {
				return ApplicationModelEditPart.VISUAL_ID;
			} else {
				return -1;
			}
		}
		return org.opendds.modeling.diagram.application.part.OpenDDSVisualIDRegistry
				.getVisualID(view.getType());
	}

	/**
	 * @generated
	 */
	public static String getModelID(View view) {
		View diagram = view.getDiagram();
		while (view != diagram) {
			EAnnotation annotation = view.getEAnnotation("Shortcut"); //$NON-NLS-1$
			if (annotation != null) {
				return annotation.getDetails().get("modelID"); //$NON-NLS-1$
			}
			view = (View) view.eContainer();
		}
		return diagram != null ? diagram.getType() : null;
	}

	/**
	 * @generated
	 */
	public static int getVisualID(String type) {
		try {
			return Integer.parseInt(type);
		} catch (NumberFormatException e) {
			if (Boolean.TRUE.toString().equalsIgnoreCase(
					Platform.getDebugOption(DEBUG_KEY))) {
				OpenDDSDiagramEditorPlugin.getInstance().logError(
						"Unable to parse view type as a visualID number: "
								+ type);
			}
		}
		return -1;
	}

	/**
	 * @generated
	 */
	public static String getType(int visualID) {
		return String.valueOf(visualID);
	}

	/**
	 * @generated
	 */
	public static int getDiagramVisualID(EObject domainElement) {
		if (domainElement == null) {
			return -1;
		}
		if (OpenDDS.OpenDDSPackage.eINSTANCE.getApplicationModel()
				.isSuperTypeOf(domainElement.eClass())
				&& isDiagram((OpenDDS.ApplicationModel) domainElement)) {
			return ApplicationModelEditPart.VISUAL_ID;
		}
		return -1;
	}

	/**
	 * @generated
	 */
	public static int getNodeVisualID(View containerView, EObject domainElement) {
		if (domainElement == null) {
			return -1;
		}
		String containerModelID = org.opendds.modeling.diagram.application.part.OpenDDSVisualIDRegistry
				.getModelID(containerView);
		if (!ApplicationModelEditPart.MODEL_ID.equals(containerModelID)) {
			return -1;
		}
		int containerVisualID;
		if (ApplicationModelEditPart.MODEL_ID.equals(containerModelID)) {
			containerVisualID = org.opendds.modeling.diagram.application.part.OpenDDSVisualIDRegistry
					.getVisualID(containerView);
		} else {
			if (containerView instanceof Diagram) {
				containerVisualID = ApplicationModelEditPart.VISUAL_ID;
			} else {
				return -1;
			}
		}
		switch (containerVisualID) {
		case ApplicationModelEditPart.VISUAL_ID:
			if (OpenDDS.OpenDDSPackage.eINSTANCE.getApplicationTarget()
					.isSuperTypeOf(domainElement.eClass())) {
				return ApplicationTargetEditPart.VISUAL_ID;
			}
			break;
		}
		return -1;
	}

	/**
	 * @generated
	 */
	public static boolean canCreateNode(View containerView, int nodeVisualID) {
		String containerModelID = org.opendds.modeling.diagram.application.part.OpenDDSVisualIDRegistry
				.getModelID(containerView);
		if (!ApplicationModelEditPart.MODEL_ID.equals(containerModelID)) {
			return false;
		}
		int containerVisualID;
		if (ApplicationModelEditPart.MODEL_ID.equals(containerModelID)) {
			containerVisualID = org.opendds.modeling.diagram.application.part.OpenDDSVisualIDRegistry
					.getVisualID(containerView);
		} else {
			if (containerView instanceof Diagram) {
				containerVisualID = ApplicationModelEditPart.VISUAL_ID;
			} else {
				return false;
			}
		}
		switch (containerVisualID) {
		case ApplicationModelEditPart.VISUAL_ID:
			if (ApplicationTargetEditPart.VISUAL_ID == nodeVisualID) {
				return true;
			}
			break;
		}
		return false;
	}

	/**
	 * @generated
	 */
	public static int getLinkWithClassVisualID(EObject domainElement) {
		if (domainElement == null) {
			return -1;
		}
		return -1;
	}

	/**
	 * User can change implementation of this method to handle some specific
	 * situations not covered by default logic.
	 * 
	 * @generated
	 */
	private static boolean isDiagram(OpenDDS.ApplicationModel element) {
		return true;
	}

}
