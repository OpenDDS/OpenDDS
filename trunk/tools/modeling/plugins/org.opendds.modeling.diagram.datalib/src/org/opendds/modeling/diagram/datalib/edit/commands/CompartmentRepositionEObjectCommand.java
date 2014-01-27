package org.opendds.modeling.diagram.datalib.edit.commands;

import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.transaction.TransactionalEditingDomain;
import org.eclipse.gef.EditPart;
import org.eclipse.gmf.runtime.common.core.command.CommandResult;
import org.eclipse.gmf.runtime.diagram.core.util.ViewUtil;
import org.eclipse.gmf.runtime.emf.commands.core.commands.RepositionEObjectCommand;
import org.eclipse.gmf.runtime.notation.View;

/**
 * Taken from recipe "HowTo reorder children in a GMF compartment with Drag & Drop" at
 * <a href="http://wiki.eclipse.org/GMF/Recipes">GMF/Recipes</a>.
 * @generated NOT
 */
public class CompartmentRepositionEObjectCommand extends RepositionEObjectCommand {

	EditPart childToMove = null;
	int newIndex = 0;

	public CompartmentRepositionEObjectCommand(
			TransactionalEditingDomain editingDomain, String label,
			EList elements, EObject element, int displacement) {
		super(editingDomain, label, elements, element, displacement);
	}

	public CompartmentRepositionEObjectCommand(EditPart childToMove,
			TransactionalEditingDomain editingDomain, String label,
			EList elements, EObject element, int displacement, int newIndex) {
		super(editingDomain, label, elements, element, displacement);

		this.childToMove = childToMove;
		this.newIndex = newIndex;
	}

	@Override
	public CommandResult doExecuteWithResult(
			IProgressMonitor progressMonitor, IAdaptable info)
			throws ExecutionException {
		CommandResult rs = super.doExecuteWithResult(progressMonitor, info);

		EditPart compartment = childToMove.getParent();

		ViewUtil.repositionChildAt((View)compartment.getModel(), (View)childToMove.getModel(), newIndex);
		compartment.refresh();

		return rs;
	}
}
