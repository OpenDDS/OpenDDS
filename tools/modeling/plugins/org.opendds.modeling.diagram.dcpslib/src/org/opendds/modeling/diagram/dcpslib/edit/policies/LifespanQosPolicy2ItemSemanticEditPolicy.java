/*
 * (c) Copyright Object Computing, Incorporated. 2005,2010. All rights reserved.
 */
package org.opendds.modeling.diagram.dcpslib.edit.policies;

import java.util.Iterator;

import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.gef.commands.Command;
import org.eclipse.gmf.runtime.common.core.command.ICompositeCommand;
import org.eclipse.gmf.runtime.diagram.core.commands.DeleteCommand;
import org.eclipse.gmf.runtime.emf.commands.core.command.CompositeTransactionalCommand;
import org.eclipse.gmf.runtime.emf.type.core.commands.DestroyElementCommand;
import org.eclipse.gmf.runtime.emf.type.core.requests.CreateElementRequest;
import org.eclipse.gmf.runtime.emf.type.core.requests.DestroyElementRequest;
import org.eclipse.gmf.runtime.notation.Node;
import org.eclipse.gmf.runtime.notation.View;
import org.opendds.modeling.common.gmf.OpenDDSLibHelper;
import org.opendds.modeling.diagram.dcpslib.edit.commands.Period4CreateCommand;
import org.opendds.modeling.diagram.dcpslib.edit.parts.LifespanQosPolicyDuration2EditPart;
import org.opendds.modeling.diagram.dcpslib.edit.parts.Period4EditPart;
import org.opendds.modeling.diagram.dcpslib.part.OpenDDSDcpsLibVisualIDRegistry;
import org.opendds.modeling.diagram.dcpslib.providers.OpenDDSDcpsLibElementTypes;

/**
 * @generated
 */
public class LifespanQosPolicy2ItemSemanticEditPolicy extends
		OpenDDSDcpsLibBaseItemSemanticEditPolicy {

	/**
	 * @generated
	 */
	public LifespanQosPolicy2ItemSemanticEditPolicy() {
		super(OpenDDSDcpsLibElementTypes.LifespanQosPolicy_3042);
	}

	/**
	 * @generated
	 */
	protected Command getCreateCommand(CreateElementRequest req) {
		if (OpenDDSDcpsLibElementTypes.Period_3008 == req.getElementType()) {
			return getGEFWrapper(new Period4CreateCommand(req));
		}
		return super.getCreateCommand(req);
	}

	/**
	 * Do not really destroy the element since the compartment holds non-containment
	 * references while GMF expects the compartment to hold contained references.
	 * Therefore a DestroyReferenceCommand is returned instead of a DestroyElementCommand.
	 * @generated NOT
	 */
	protected Command getDestroyElementCommand(DestroyElementRequest req) {
		CompositeTransactionalCommand cmd = new CompositeTransactionalCommand(
				getEditingDomain(), null);
		cmd.setTransactionNestingEnabled(false);
		cmd.add(com.ociweb.gmf.edit.commands.RequestToCommandConverter
				.destroyElementRequestToDestroyReferenceCommand(req, getHost(),
						getEditingDomain()));
		return getGEFWrapper(cmd);
	}

	/**
	 * @generated
	 */
	private void addDestroyChildNodesCommand(ICompositeCommand cmd) {
		View view = (View) getHost().getModel();
		for (Iterator nit = view.getChildren().iterator(); nit.hasNext();) {
			Node node = (Node) nit.next();
			switch (OpenDDSDcpsLibVisualIDRegistry.getVisualID(node)) {
			case Period4EditPart.VISUAL_ID:
				cmd.add(new DestroyElementCommand(new DestroyElementRequest(
						getEditingDomain(), node.getElement(), false))); // directlyOwned: true
				// don't need explicit deletion of node as parent's view deletion would clean child views as well 
				// cmd.add(new org.eclipse.gmf.runtime.diagram.core.commands.DeleteCommand(getEditingDomain(), node));
				break;
			case LifespanQosPolicyDuration2EditPart.VISUAL_ID:
				for (Iterator cit = node.getChildren().iterator(); cit
						.hasNext();) {
					Node cnode = (Node) cit.next();
					// For the OpenDDS Modeling SDK, elements behind compartment children may not necessarily be in the same
					// library as the element behind the compartment's parent (e.g. a DataReader's shared policies).
					// In this case avoid destroying the child.
					if (!OpenDDSLibHelper.areElementsInSameLib(
							view.getElement(), cnode.getElement())) {
						break;
					}
					switch (OpenDDSDcpsLibVisualIDRegistry.getVisualID(cnode)) {
					}
				}
				break;
			}
		}
	}

}
