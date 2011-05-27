/*
 * (c) Copyright Object Computing, Incorporated. 2005,2010. All rights reserved.
 */
package org.opendds.modeling.diagram.dcpslib.edit.parts;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ListCompartmentEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.CanonicalEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.CreationEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.DragDropEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.EditPolicyRoles;
import org.eclipse.gmf.runtime.notation.View;
import org.opendds.modeling.diagram.dcpslib.edit.policies.TopicDataTypeCanonicalEditPolicy;
import org.opendds.modeling.diagram.dcpslib.edit.policies.TopicDataTypeItemSemanticEditPolicy;
import org.opendds.modeling.diagram.dcpslib.part.Messages;
import org.opendds.modeling.model.topics.TopicsPackage;

/**
 * @generated
 */
public class TopicDataTypeEditPart extends ListCompartmentEditPart {

	/**
	 * @generated
	 */
	public static final int VISUAL_ID = 7050;

	/**
	 * @generated
	 */
	public TopicDataTypeEditPart(View view) {
		super(view);
	}

	/**
	 * @generated
	 */
	protected boolean hasModelChildrenChanged(Notification evt) {
		return false;
	}

	/**
	 * @generated
	 */
	public String getCompartmentName() {
		return Messages.TopicDataTypeEditPart_title;
	}

	/**
	 * @generated
	 */
	protected void createDefaultEditPolicies() {
		super.createDefaultEditPolicies();
		installEditPolicy(EditPolicyRoles.SEMANTIC_ROLE,
				new TopicDataTypeItemSemanticEditPolicy());
		installEditPolicy(EditPolicyRoles.CREATION_ROLE,
				new CreationEditPolicy());
		installEditPolicy(EditPolicyRoles.DRAG_DROP_ROLE,
				new DragDropEditPolicy());
		installEditPolicy(EditPolicyRoles.CANONICAL_ROLE,
				new TopicDataTypeCanonicalEditPolicy());
	}

	/**
	 * @generated
	 */
	protected void setRatio(Double ratio) {
		// nothing to do -- parent layout does not accept Double constraints as ratio
		// super.setRatio(ratio); 
	}

	/*
	 * Since the Topic's datatype can be changed outside diagram manipulation
	 * through a context menu, make sure the datatype compartment gets refreshed.
	 * generated NOT
	 */
	@Override
	protected void handleNotificationEvent(Notification notification) {
		int type = notification.getEventType();
		Object feature = notification.getFeature();
		if (TopicsPackage.eINSTANCE.getTopic_Datatype().equals(feature)
				&& (type == Notification.SET)) {
			CanonicalEditPolicy canonicalEditPolicy = (CanonicalEditPolicy) getEditPolicy(EditPolicyRoles.CANONICAL_ROLE);
			canonicalEditPolicy.refresh();
		}
		super.handleNotificationEvent(notification);
	}

}
