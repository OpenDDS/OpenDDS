/*
 * (c) Copyright Object Computing, Incorporated. 2005,2010. All rights reserved.
 */
package org.opendds.modeling.diagram.dcpslib.edit.parts;

import org.eclipse.draw2d.IFigure;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.EditPolicy;
import org.eclipse.gef.Request;
import org.eclipse.gmf.runtime.diagram.core.edithelpers.CreateElementRequestAdapter;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ListCompartmentEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.CreationEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.DragDropEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.EditPolicyRoles;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.ResizableCompartmentEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.figures.ResizableCompartmentFigure;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateUnspecifiedTypeConnectionRequest;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateViewAndElementRequest;
import org.eclipse.gmf.runtime.emf.type.core.IElementType;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.gmf.tooling.runtime.edit.policies.reparent.CreationEditPolicyWithCustomReparent;
import org.opendds.modeling.diagram.dcpslib.edit.policies.TopicQoSPoliciesCustomCanonicalEditPolicy;
import org.opendds.modeling.diagram.dcpslib.edit.policies.TopicQoSPoliciesCustomItemSemanticEditPolicy;
import org.opendds.modeling.diagram.dcpslib.part.Messages;
import org.opendds.modeling.diagram.dcpslib.part.OpenDDSDcpsLibVisualIDRegistry;
import org.opendds.modeling.diagram.dcpslib.providers.OpenDDSDcpsLibElementTypes;

/**
 * @generated
 */
public class TopicQoSPoliciesCustomEditPart extends ListCompartmentEditPart {

	/**
	 * @generated
	 */
	public static final int VISUAL_ID = 7051;

	/**
	 * @generated
	 */
	public TopicQoSPoliciesCustomEditPart(View view) {
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
		return Messages.TopicQoSPoliciesCustomEditPart_title;
	}

	/**
	 * @generated
	 */
	public IFigure createFigure() {
		ResizableCompartmentFigure result = (ResizableCompartmentFigure) super
				.createFigure();
		result.setTitleVisibility(false);
		return result;
	}

	/**
	 * @generated
	 */
	protected void createDefaultEditPolicies() {
		super.createDefaultEditPolicies();
		installEditPolicy(EditPolicy.PRIMARY_DRAG_ROLE,
				new ResizableCompartmentEditPolicy());
		installEditPolicy(EditPolicyRoles.SEMANTIC_ROLE,
				new TopicQoSPoliciesCustomItemSemanticEditPolicy());
		installEditPolicy(EditPolicyRoles.CREATION_ROLE,
				new CreationEditPolicyWithCustomReparent(
						OpenDDSDcpsLibVisualIDRegistry.TYPED_INSTANCE));
		installEditPolicy(EditPolicyRoles.DRAG_DROP_ROLE,
				new DragDropEditPolicy());
		installEditPolicy(EditPolicyRoles.CANONICAL_ROLE,
				new TopicQoSPoliciesCustomCanonicalEditPolicy());
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
			if (type == OpenDDSDcpsLibElementTypes.DeadlineQosPolicy_3022) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.DestinationOrderQosPolicy_3023) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.DsQosPolicy_3024) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.DurabilityQosPolicy_3035) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.HistoryQosPolicy_3025) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.LbQosPolicy_3026) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.LifespanQosPolicy_3027) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.LivelinessQosPolicy_3028) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.OwnershipQosPolicy_3030) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.ReliabilityQosPolicy_3031) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.RlQosPolicy_3032) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.TdQosPolicy_3051) {
				return this;
			}
			if (type == OpenDDSDcpsLibElementTypes.TpQosPolicy_3033) {
				return this;
			}
			return getParent().getTargetEditPart(request);
		}
		if (request instanceof CreateUnspecifiedTypeConnectionRequest) {
			return getParent().getTargetEditPart(request);
		}
		return super.getTargetEditPart(request);
	}

	/**
	 * Filter out policies that are not owned by a DcpsLib.
	 * @generated NOT
	 */
	@Override
	protected java.util.List getModelChildren() {
		return CompartmentChildrenFinder.filterPoliciesNotOwnedByDcpsLib(
				((View) getModel()).getElement(), super.getModelChildren());
	}

}
