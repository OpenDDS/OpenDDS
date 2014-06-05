/*
 * (c) Copyright Object Computing, Incorporated.  2005,2010.  All rights reserved.
 */
package org.opendds.modeling.diagram.datalib.edit.parts;

import org.eclipse.draw2d.FlowLayout;
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
import org.eclipse.gmf.runtime.diagram.ui.figures.ResizableCompartmentFigure;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateUnspecifiedTypeConnectionRequest;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateViewAndElementRequest;
import org.eclipse.gmf.runtime.emf.type.core.IElementType;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.gmf.tooling.runtime.edit.policies.reparent.CreationEditPolicyWithCustomReparent;
import org.opendds.modeling.diagram.datalib.edit.policies.CompartmentEditPolicy;
import org.opendds.modeling.diagram.datalib.edit.policies.StructKeysCanonicalEditPolicy;
import org.opendds.modeling.diagram.datalib.edit.policies.StructKeysItemSemanticEditPolicy;
import org.opendds.modeling.diagram.datalib.part.Messages;
import org.opendds.modeling.diagram.datalib.part.OpenDDSDataLibVisualIDRegistry;
import org.opendds.modeling.diagram.datalib.providers.OpenDDSDataLibElementTypes;
import org.opendds.modeling.model.types.TypesPackage;

/**
 * @generated
 */
public class StructKeysEditPart extends ListCompartmentEditPart {

	/**
	 * @generated
	 */
	public static final int VISUAL_ID = 7004;

	/**
	 * @generated
	 */
	public StructKeysEditPart(View view) {
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
		return Messages.StructKeysEditPart_title;
	}

	/**
	 * Taken from recipe "HowTo reorder children in a GMF compartment with Drag & Drop" at
	 * <a href="http://wiki.eclipse.org/GMF/Recipes">GMF/Recipes</a>.
	 * @generated NOT
	 */
	public IFigure createFigure() {
		ResizableCompartmentFigure rcf = (ResizableCompartmentFigure) super
				.createFigure();
		FlowLayout layout = new FlowLayout();
		layout.setMajorSpacing(getMapMode().DPtoLP(5));
		layout.setMinorSpacing(getMapMode().DPtoLP(5));
		layout.setHorizontal(false);

		rcf.getContentPane().setLayoutManager(layout);

		rcf.setTitleVisibility(false);

		return rcf;
	}

	/**
	 * Enhanced based on drag & drop recipe.
	 * @generated NOT
	 */
	protected void createDefaultEditPolicies() {
		super.createDefaultEditPolicies();
		installEditPolicy(EditPolicyRoles.SEMANTIC_ROLE,
				new StructKeysItemSemanticEditPolicy());
		installEditPolicy(EditPolicyRoles.CREATION_ROLE,
				new CreationEditPolicy());
		installEditPolicy(EditPolicyRoles.DRAG_DROP_ROLE,
				new DragDropEditPolicy());
		installEditPolicy(EditPolicyRoles.CANONICAL_ROLE,
				new StructKeysCanonicalEditPolicy());

		// Added for Drag and Drop support
		installEditPolicy(EditPolicy.LAYOUT_ROLE, new CompartmentEditPolicy(
				TypesPackage.Literals.STRUCT__FIELDS));
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
			if (type == OpenDDSDataLibElementTypes.Key_3004) {
				return this;
			}
			return getParent().getTargetEditPart(request);
		}
		if (request instanceof CreateUnspecifiedTypeConnectionRequest) {
			return getParent().getTargetEditPart(request);
		}
		return super.getTargetEditPart(request);
	}

}
