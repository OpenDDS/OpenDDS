package org.opendds.modeling.diagram.datalib.edit.policies;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.transaction.TransactionalEditingDomain;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.EditPolicy;
import org.eclipse.gef.Request;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.requests.CreateRequest;
import org.eclipse.gmf.runtime.diagram.core.commands.AddCommand;
import org.eclipse.gmf.runtime.diagram.ui.commands.ICommandProxy;
import org.eclipse.gmf.runtime.diagram.ui.editparts.IGraphicalEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editpolicies.ResizableEditPolicyEx;
import org.eclipse.gmf.runtime.emf.commands.core.commands.RepositionEObjectCommand;
import org.eclipse.gmf.runtime.emf.core.util.EObjectAdapter;
import org.eclipse.gmf.runtime.notation.View;
import org.opendds.modeling.diagram.datalib.edit.commands.CompartmentRepositionEObjectCommand;


/**
 * Taken from recipe "HowTo reorder children in a GMF compartment with Drag & Drop" at
 * <a href="http://wiki.eclipse.org/GMF/Recipes">GMF/Recipes</a>.
 * @generated NOT
 */
public class CompartmentEditPolicy extends org.eclipse.gef.editpolicies.FlowLayoutEditPolicy {

	private EStructuralFeature feature = null;

	@Override
	protected Command createAddCommand(EditPart child, EditPart after) {
		int index = getHost().getChildren().indexOf(after);
		TransactionalEditingDomain editingDomain = ((IGraphicalEditPart) getHost()).getEditingDomain();
		AddCommand command = new AddCommand(editingDomain, new EObjectAdapter((View)getHost().getModel()),
				new EObjectAdapter((View)child.getModel()), index);
		return new ICommandProxy(command);
	}

	@Override
	protected EditPolicy createChildEditPolicy(EditPart child) {
		ResizableEditPolicyEx policy = new ResizableEditPolicyEx();
		policy.setResizeDirections(0);
		return policy;
	}

	@Override
	protected Command createMoveChildCommand(EditPart child, EditPart after) {

		int newIndex;
		int displacement;

		int childIndex = getHost().getChildren().indexOf(child);
		int afterIndex = getHost().getChildren().indexOf(after);

		if(afterIndex == -1) {
			newIndex = getHost().getChildren().size()-1;
			displacement = newIndex - childIndex;
		} else {
			newIndex = afterIndex;
			displacement = afterIndex - childIndex;
			if (childIndex <= afterIndex) {
				newIndex--;
				displacement--;
			}
		}


		TransactionalEditingDomain editingDomain = ((IGraphicalEditPart) getHost()).getEditingDomain();

		RepositionEObjectCommand command = new CompartmentRepositionEObjectCommand(child, editingDomain, "",
				(EList)((View)child.getParent().getModel()).getElement().eGet(feature),
				((View)child.getModel()).getElement(),
				displacement, newIndex);

		//TODO ev. reintroduce target feedback (actual problem: line is not deleted after dropping)
		eraseLayoutTargetFeedback(null);

		return new ICommandProxy(command);
	}

	@Override
	protected Command getCreateCommand(CreateRequest request) {
		return null;
	}

	@Override
	protected Command getDeleteDependantCommand(Request request) {
		return null;
	}

	@Override
	protected Command getOrphanChildrenCommand(Request request) {
		return null;
	}

	/**
	 * @param feature has to be an EList
	 */
	public CompartmentEditPolicy(EStructuralFeature feature) {
		super();
		this.feature = feature;
	}

}

