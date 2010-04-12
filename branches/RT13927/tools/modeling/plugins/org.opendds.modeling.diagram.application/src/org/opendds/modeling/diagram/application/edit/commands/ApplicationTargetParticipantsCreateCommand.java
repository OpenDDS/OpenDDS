package org.opendds.modeling.diagram.application.edit.commands;

import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.gmf.runtime.common.core.command.CommandResult;
import org.eclipse.gmf.runtime.emf.type.core.commands.EditElementCommand;
import org.eclipse.gmf.runtime.emf.type.core.requests.CreateRelationshipRequest;
import org.opendds.modeling.diagram.application.edit.policies.OpenDDSBaseItemSemanticEditPolicy;

/**
 * @generated
 */
public class ApplicationTargetParticipantsCreateCommand extends
		EditElementCommand {

	/**
	 * @generated
	 */
	private final EObject source;

	/**
	 * @generated
	 */
	private final EObject target;

	/**
	 * @generated
	 */
	public ApplicationTargetParticipantsCreateCommand(
			CreateRelationshipRequest request, EObject source, EObject target) {
		super(request.getLabel(), null, request);
		this.source = source;
		this.target = target;
	}

	/**
	 * @generated
	 */
	@Override
	public boolean canExecute() {
		if (source == null && target == null) {
			return false;
		}
		if (source != null
				&& false == source instanceof OpenDDS.ApplicationTarget) {
			return false;
		}
		if (target != null
				&& false == target instanceof OpenDDS.DomainParticipant) {
			return false;
		}
		if (getSource() == null) {
			return true; // link creation is in progress; source is not defined
							// yet
		}
		// target may be null here but it's possible to check constraint
		return OpenDDSBaseItemSemanticEditPolicy.LinkConstraints
				.canCreateApplicationTargetParticipants_4001(getSource(),
						getTarget());
	}

	/**
	 * @generated
	 */
	@Override
	protected CommandResult doExecuteWithResult(IProgressMonitor monitor,
			IAdaptable info) throws ExecutionException {
		if (!canExecute()) {
			throw new ExecutionException(
					"Invalid arguments in create link command"); //$NON-NLS-1$
		}

		if (getSource() != null && getTarget() != null) {
			getSource().getParticipants().add(getTarget());
		}
		return CommandResult.newOKCommandResult();

	}

	/**
	 * @generated
	 */
	@Override
	protected void setElementToEdit(EObject element) {
		throw new UnsupportedOperationException();
	}

	/**
	 * @generated
	 */
	protected OpenDDS.ApplicationTarget getSource() {
		return (OpenDDS.ApplicationTarget) source;
	}

	/**
	 * @generated
	 */
	protected OpenDDS.DomainParticipant getTarget() {
		return (OpenDDS.DomainParticipant) target;
	}
}
