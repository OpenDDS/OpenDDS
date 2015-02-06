/*
 * (c) Copyright Object Computing, Incorporated. 2005,2010. All rights reserved.
 */
package org.opendds.modeling.diagram.dcpslib.edit.parts;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.RectangleFigure;
import org.eclipse.draw2d.Shape;
import org.eclipse.draw2d.StackLayout;
import org.eclipse.draw2d.ToolbarLayout;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EcorePackage;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.EditPolicy;
import org.eclipse.gef.Request;
import org.eclipse.gef.editpolicies.LayoutEditPolicy;
import org.eclipse.gmf.runtime.diagram.core.edithelpers.CreateElementRequestAdapter;
import org.eclipse.gmf.runtime.diagram.ui.editparts.IGraphicalEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ITextAwareEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ShapeNodeEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.ConstrainedToolbarLayoutEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.CreationEditPolicy;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.EditPolicyRoles;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateViewAndElementRequest;
import org.eclipse.gmf.runtime.draw2d.ui.figures.ConstrainedToolbarLayout;
import org.eclipse.gmf.runtime.draw2d.ui.figures.WrappingLabel;
import org.eclipse.gmf.runtime.emf.type.core.IElementType;
import org.eclipse.gmf.runtime.gef.ui.figures.DefaultSizeNodeFigure;
import org.eclipse.gmf.runtime.gef.ui.figures.NodeFigure;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.gmf.tooling.runtime.edit.policies.reparent.CreationEditPolicyWithCustomReparent;
import org.eclipse.swt.graphics.Color;
import org.opendds.modeling.diagram.dcpslib.edit.policies.DomainParticipantItemSemanticEditPolicy;
import org.opendds.modeling.diagram.dcpslib.edit.policies.OpenDDSDcpsLibTextSelectionEditPolicy;
import org.opendds.modeling.diagram.dcpslib.part.OpenDDSDcpsLibVisualIDRegistry;
import org.opendds.modeling.diagram.dcpslib.providers.OpenDDSDcpsLibElementTypes;

/**
 * @generated
 */
public class DomainParticipantEditPart extends ShapeNodeEditPart {

	/**
	 * @generated
	 */
	public static final int VISUAL_ID = 2001;

	/**
	 * @generated
	 */
	protected IFigure contentPane;

	/**
	 * @generated
	 */
	protected IFigure primaryShape;

	/**
	 * @generated
	 */
	public DomainParticipantEditPart(View view) {
		super(view);
	}

	/**
	 * @generated
	 */
	protected void createDefaultEditPolicies() {
		installEditPolicy(EditPolicyRoles.CREATION_ROLE,
				new CreationEditPolicyWithCustomReparent(
						OpenDDSDcpsLibVisualIDRegistry.TYPED_INSTANCE));
		super.createDefaultEditPolicies();
		installEditPolicy(EditPolicyRoles.SEMANTIC_ROLE,
				new DomainParticipantItemSemanticEditPolicy());
		installEditPolicy(EditPolicy.LAYOUT_ROLE, createLayoutEditPolicy());
		// XXX need an SCR to runtime to have another abstract superclass that would let children add reasonable editpolicies
		// removeEditPolicy(org.eclipse.gmf.runtime.diagram.ui.editpolicies.EditPolicyRoles.CONNECTION_HANDLES_ROLE);
	}

	/**
	 * @generated
	 */
	protected LayoutEditPolicy createLayoutEditPolicy() {

		ConstrainedToolbarLayoutEditPolicy lep = new ConstrainedToolbarLayoutEditPolicy() {

			protected EditPolicy createChildEditPolicy(EditPart child) {
				if (child.getEditPolicy(EditPolicy.PRIMARY_DRAG_ROLE) == null) {
					if (child instanceof ITextAwareEditPart) {
						return new OpenDDSDcpsLibTextSelectionEditPolicy();
					}
				}
				return super.createChildEditPolicy(child);
			}
		};
		return lep;
	}

	/**
	 * @generated
	 */
	protected IFigure createNodeShape() {
		return primaryShape = new DomainParticipantFigure();
	}

	/**
	 * @generated
	 */
	public DomainParticipantFigure getPrimaryShape() {
		return (DomainParticipantFigure) primaryShape;
	}

	/**
	 * @generated
	 */
	protected boolean addFixedChild(EditPart childEditPart) {
		if (childEditPart instanceof DomainParticipantNameEditPart) {
			((DomainParticipantNameEditPart) childEditPart)
					.setLabel(getPrimaryShape()
							.getFigureDomainParticipantNameFigure());
			return true;
		}
		if (childEditPart instanceof DomainParticipantName2EditPart) {
			((DomainParticipantName2EditPart) childEditPart)
					.setLabel(getPrimaryShape()
							.getFigureDomainParticipantStereotypeFigure());
			return true;
		}
		if (childEditPart instanceof DomainParticipantTransportConfigEditPart) {
			((DomainParticipantTransportConfigEditPart) childEditPart)
					.setLabel(getPrimaryShape()
							.getFigureDomainParticipantTransportConfigFigure());
			return true;
		}
		return false;
	}

	/**
	 * @generated
	 */
	protected boolean removeFixedChild(EditPart childEditPart) {
		if (childEditPart instanceof DomainParticipantNameEditPart) {
			return true;
		}
		if (childEditPart instanceof DomainParticipantName2EditPart) {
			return true;
		}
		if (childEditPart instanceof DomainParticipantTransportConfigEditPart) {
			return true;
		}
		return false;
	}

	/**
	 * @generated
	 */
	protected void addChildVisual(EditPart childEditPart, int index) {
		if (addFixedChild(childEditPart)) {
			return;
		}
		super.addChildVisual(childEditPart, -1);
	}

	/**
	 * @generated
	 */
	protected void removeChildVisual(EditPart childEditPart) {
		if (removeFixedChild(childEditPart)) {
			return;
		}
		super.removeChildVisual(childEditPart);
	}

	/**
	 * @generated
	 */
	protected IFigure getContentPaneFor(IGraphicalEditPart editPart) {
		return getContentPane();
	}

	/**
	 * @generated
	 */
	protected NodeFigure createNodePlate() {
		DefaultSizeNodeFigure result = new DefaultSizeNodeFigure(40, 40);
		return result;
	}

	/**
	 * Creates figure for this edit part.
	 * 
	 * Body of this method does not depend on settings in generation model
	 * so you may safely remove <i>generated</i> tag and modify it.
	 * 
	 * @generated
	 */
	protected NodeFigure createNodeFigure() {
		NodeFigure figure = createNodePlate();
		figure.setLayoutManager(new StackLayout());
		IFigure shape = createNodeShape();
		figure.add(shape);
		contentPane = setupContentPane(shape);
		return figure;
	}

	/**
	 * Default implementation treats passed figure as content pane.
	 * Respects layout one may have set for generated figure.
	 * @param nodeShape instance of generated figure class
	 * @generated
	 */
	protected IFigure setupContentPane(IFigure nodeShape) {
		if (nodeShape.getLayoutManager() == null) {
			ConstrainedToolbarLayout layout = new ConstrainedToolbarLayout();
			layout.setSpacing(5);
			nodeShape.setLayoutManager(layout);
		}
		return nodeShape; // use nodeShape itself as contentPane
	}

	/**
	 * @generated
	 */
	public IFigure getContentPane() {
		if (contentPane != null) {
			return contentPane;
		}
		return super.getContentPane();
	}

	/**
	 * @generated
	 */
	protected void setForegroundColor(Color color) {
		if (primaryShape != null) {
			primaryShape.setForegroundColor(color);
		}
	}

	/**
	 * @generated
	 */
	protected void setBackgroundColor(Color color) {
		if (primaryShape != null) {
			primaryShape.setBackgroundColor(color);
		}
	}

	/**
	 * @generated
	 */
	protected void setLineWidth(int width) {
		if (primaryShape instanceof Shape) {
			((Shape) primaryShape).setLineWidth(width);
		}
	}

	/**
	 * @generated
	 */
	protected void setLineType(int style) {
		if (primaryShape instanceof Shape) {
			((Shape) primaryShape).setLineStyle(style);
		}
	}

	/**
	 * @generated
	 */
	public EditPart getPrimaryChildEditPart() {
		return getChildBySemanticHint(OpenDDSDcpsLibVisualIDRegistry
				.getType(DomainParticipantNameEditPart.VISUAL_ID));
	}

	/**
	 * @generated NOT
	 */
	public EditPart getTargetEditPart(Request request) {
		if (request instanceof CreateViewAndElementRequest) {
			CreateElementRequestAdapter adapter = ((CreateViewAndElementRequest) request)
					.getViewAndElementDescriptor()
					.getCreateElementRequestAdapter();
			IElementType type = (IElementType) adapter
					.getAdapter(IElementType.class);
			if (type == OpenDDSDcpsLibElementTypes.EfQosPolicy_3012) {
				return getChildBySemanticHint(OpenDDSDcpsLibVisualIDRegistry
						.getType(DomainParticipantQoSPoliciesCustomEditPart.VISUAL_ID));
			}
			if (type == OpenDDSDcpsLibElementTypes.UdQosPolicy_3013) {
				return getChildBySemanticHint(OpenDDSDcpsLibVisualIDRegistry
						.getType(DomainParticipantQoSPoliciesCustomEditPart.VISUAL_ID));
			}
			if (type == OpenDDSDcpsLibElementTypes.EfQosPolicy_3017) {
				return getChildBySemanticHint(OpenDDSDcpsLibVisualIDRegistry
						.getType(DomainParticipantQoSPoliciesSharedEditPart.VISUAL_ID));
			}
			if (type == OpenDDSDcpsLibElementTypes.UdQosPolicy_3021) {
				return getChildBySemanticHint(OpenDDSDcpsLibVisualIDRegistry
						.getType(DomainParticipantQoSPoliciesSharedEditPart.VISUAL_ID));
			}
			// Custom code begin
			// Extend the region in the DomainParticipant which can be clicked no
			// to add a pub or sub include the top portion of the DP figure containing
			// the stereotype and name label.
			// (If list layout were true for the pub/sub compartment this code would already
			// be here. A bug in GMF 2.2.2?)
			if (type == OpenDDSDcpsLibElementTypes.Publisher_3001) {
				return getChildBySemanticHint(OpenDDSDcpsLibVisualIDRegistry
						.getType(DomainParticipantDefaultCompartmentEditPart.VISUAL_ID));
			}
			if (type == OpenDDSDcpsLibElementTypes.Subscriber_3002) {
				return getChildBySemanticHint(OpenDDSDcpsLibVisualIDRegistry
						.getType(DomainParticipantDefaultCompartmentEditPart.VISUAL_ID));
			}
			// Custom code end
		}
		return super.getTargetEditPart(request);
	}

	/**
	 * @generated
	 */
	protected void handleNotificationEvent(Notification event) {
		if (event.getNotifier() == getModel()
				&& EcorePackage.eINSTANCE.getEModelElement_EAnnotations()
						.equals(event.getFeature())) {
			handleMajorSemanticChange();
		} else {
			super.handleNotificationEvent(event);
		}
	}

	/**
	 * @generated
	 */
	public class DomainParticipantFigure extends RectangleFigure {

		/**
		 * @generated
		 */
		private WrappingLabel fFigureDomainParticipantStereotypeFigure;
		/**
		 * @generated
		 */
		private WrappingLabel fFigureDomainParticipantNameFigure;
		/**
		 * @generated
		 */
		private WrappingLabel fFigureDomainParticipantTransportConfigFigure;

		/**
		 * @generated
		 */
		public DomainParticipantFigure() {

			ToolbarLayout layoutThis = new ToolbarLayout();
			layoutThis.setStretchMinorAxis(false);
			layoutThis.setMinorAlignment(ToolbarLayout.ALIGN_TOPLEFT);

			layoutThis.setSpacing(5);
			layoutThis.setVertical(true);

			this.setLayoutManager(layoutThis);

			this.setLineWidth(1);
			createContents();
		}

		/**
		 * @generated
		 */
		private void createContents() {

			fFigureDomainParticipantStereotypeFigure = new WrappingLabel();
			fFigureDomainParticipantStereotypeFigure.setText("<...>");

			this.add(fFigureDomainParticipantStereotypeFigure);

			fFigureDomainParticipantNameFigure = new WrappingLabel();
			fFigureDomainParticipantNameFigure.setText("<...>");

			this.add(fFigureDomainParticipantNameFigure);

			fFigureDomainParticipantTransportConfigFigure = new WrappingLabel();
			fFigureDomainParticipantTransportConfigFigure.setText("<...>");

			this.add(fFigureDomainParticipantTransportConfigFigure);

		}

		/**
		 * @generated
		 */
		private boolean myUseLocalCoordinates = false;

		/**
		 * @generated
		 */
		protected boolean useLocalCoordinates() {
			return myUseLocalCoordinates;
		}

		/**
		 * @generated
		 */
		protected void setUseLocalCoordinates(boolean useLocalCoordinates) {
			myUseLocalCoordinates = useLocalCoordinates;
		}

		/**
		 * @generated
		 */
		public WrappingLabel getFigureDomainParticipantStereotypeFigure() {
			return fFigureDomainParticipantStereotypeFigure;
		}

		/**
		 * @generated
		 */
		public WrappingLabel getFigureDomainParticipantNameFigure() {
			return fFigureDomainParticipantNameFigure;
		}

		/**
		 * @generated
		 */
		public WrappingLabel getFigureDomainParticipantTransportConfigFigure() {
			return fFigureDomainParticipantTransportConfigFigure;
		}

	}

}
