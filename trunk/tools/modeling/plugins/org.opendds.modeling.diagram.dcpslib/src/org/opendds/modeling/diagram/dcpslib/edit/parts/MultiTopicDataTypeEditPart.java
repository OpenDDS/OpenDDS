/*
 * (c) Copyright Object Computing, Incorporated. 2005,2010. All rights reserved.
 */
package org.opendds.modeling.diagram.dcpslib.edit.parts;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.Request;
import org.eclipse.gmf.runtime.diagram.core.edithelpers.CreateElementRequestAdapter;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ListCompartmentEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.CanonicalEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.CreationEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.DragDropEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.EditPolicyRoles;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateUnspecifiedTypeConnectionRequest;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateViewAndElementRequest;
import org.eclipse.gmf.runtime.emf.type.core.IElementType;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.gmf.tooling.runtime.edit.policies.reparent.CreationEditPolicyWithCustomReparent;
import org.opendds.modeling.diagram.dcpslib.edit.policies.MultiTopicDataTypeCanonicalEditPolicy;
import org.opendds.modeling.diagram.dcpslib.edit.policies.MultiTopicDataTypeItemSemanticEditPolicy;
import org.opendds.modeling.diagram.dcpslib.part.Messages;
import org.opendds.modeling.diagram.dcpslib.part.OpenDDSDcpsLibVisualIDRegistry;
import org.opendds.modeling.diagram.dcpslib.providers.OpenDDSDcpsLibElementTypes;
import org.opendds.modeling.model.topics.TopicsPackage;

/**
 * @generated
 */
public class MultiTopicDataTypeEditPart extends ListCompartmentEditPart {

	/**
	 * @generated
	 */
	public static final int VISUAL_ID = 7049;

	/**
	 * @generated
	 */
	public MultiTopicDataTypeEditPart(View view) {
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
		return Messages.MultiTopicDataTypeEditPart_title;
	}

	/**
	 * @generated
	 */
	protected void createDefaultEditPolicies() {
		super.createDefaultEditPolicies();
		installEditPolicy(EditPolicyRoles.SEMANTIC_ROLE,
				new MultiTopicDataTypeItemSemanticEditPolicy());
		installEditPolicy(EditPolicyRoles.CREATION_ROLE,
				new CreationEditPolicyWithCustomReparent(
						OpenDDSDcpsLibVisualIDRegistry.TYPED_INSTANCE));
		installEditPolicy(EditPolicyRoles.DRAG_DROP_ROLE,
				new DragDropEditPolicy());
		installEditPolicy(EditPolicyRoles.CANONICAL_ROLE,
				new MultiTopicDataTypeCanonicalEditPolicy());
	}

	/**
	 * @generated
	 */
	protected void setRatio(Double ratio) {
		// nothing to do -- parent layout does not accept Double constraints as ratio
		// super.setRatio(ratio); 
	}

	/**
	 * @generated
	 */
	public EditPart getTargetEditPart(Request request) {
		if (request instanceof CreateViewAndElementRequest) {
			CreateElementRequestAdapter adapter = ((CreateViewAndElementRequest) request)
					.getViewAndElementDescriptor()
					.getCreateElementRequestAdapter();
			IElementType type = (IElementType) adapter
					.getAdapter(IElementType.class);
			if (type == OpenDDSDcpsLibElementTypes.Struct_3058) {
				return this;
			}
			return getParent().getTargetEditPart(request);
		}
		if (request instanceof CreateUnspecifiedTypeConnectionRequest) {
			return getParent().getTargetEditPart(request);
		}
		return super.getTargetEditPart(request);
	}

	/*
	 * Since the MultiTopic's datatype can be changed outside diagram manipulation
	 * through a context menu, make sure the datatype compartment gets refreshed.
	 * generated NOT
	 */
	@Override
	protected void handleNotificationEvent(Notification notification) {
		int type = notification.getEventType();
		Object feature = notification.getFeature();
		if (TopicsPackage.eINSTANCE.getMultiTopic_Datatype().equals(feature)
				&& (type == Notification.SET)) {
			CanonicalEditPolicy canonicalEditPolicy = (CanonicalEditPolicy) getEditPolicy(EditPolicyRoles.CANONICAL_ROLE);
			canonicalEditPolicy.refresh();
		}
		super.handleNotificationEvent(notification);
	}

}
