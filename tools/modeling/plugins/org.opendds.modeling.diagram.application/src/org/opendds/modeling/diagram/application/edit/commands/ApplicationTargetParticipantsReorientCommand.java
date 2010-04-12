package org.opendds.modeling.diagram.application.edit.commands;

import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.gmf.runtime.common.core.command.CommandResult;
import org.eclipse.gmf.runtime.emf.type.core.commands.EditElementCommand;
import org.eclipse.gmf.runtime.emf.type.core.requests.ReorientReferenceRelationshipRequest;
import org.eclipse.gmf.runtime.emf.type.core.requests.ReorientRelationshipRequest;
import org.eclipse.gmf.runtime.emf.type.core.requests.ReorientRequest;
import org.opendds.modeling.diagram.application.edit.policies.OpenDDSBaseItemSemanticEditPolicy;

/**
 * @generated
 */
public class ApplicationTargetParticipantsReorientCommand extends
		EditElementCommand {

	/**
	 * @generated
	 */
	private final int reorientDirection;

	/**
	 * @generated
	 */
	private final EObject referenceOwner;

	/**
	 * @generated
	 */
	private final EObject oldEnd;

	/**
	 * @generated
	 */
	private final EObject newEnd;

	/**
	 * @generated
	 */
	public ApplicationTargetParticipantsReorientCommand(
			ReorientReferenceRelationshipRequest request) {
		super(request.getLabel(), null, request);
		reorientDirection = request.getDirection();
		referenceOwner = request.getReferenceOwner();
		oldEnd = request.getOldRelationshipEnd();
		newEnd = request.getNewRelationshipEnd();
	}

	/**
	 * @generated
	 */
	@Override
	public boolean canExecute() {
		if (false == referenceOwner instanceof OpenDDS.ApplicationTarget) {
			return false;
		}
		if (reorientDirection == ReorientRequest.REORIENT_SOURCE) {
			return canReorientSource();
		}
		if (reorientDirection == ReorientRequest.REORIENT_TARGET) {
			return canReorientTarget();
		}
		return false;
	}

	/**
	 * @generated
	 */
	protected boolean canReorientSource() {
		if (!(oldEnd instanceof OpenDDS.DomainParticipant && newEnd instanceof OpenDDS.ApplicationTarget)) {
			return false;
		}
		return OpenDDSBaseItemSemanticEditPolicy.LinkConstraints
				.canExistApplicationTargetParticipants_4001(getNewSource(),
						getOldTarget());
	}

	/**
	 * @generated
	 */
	protected boolean canReorientTarget() {
		if (!(oldEnd instanceof OpenDDS.DomainParticipant && newEnd instanceof OpenDDS.DomainParticipant)) {
			return false;
		}
		return OpenDDSBaseItemSemanticEditPolicy.LinkConstraints
				.canExistApplicationTargetParticipants_4001(getOldSource(),
						getNewTarget());
	}

	/**
	 * @generated
	 */
	@Override
	protected CommandResult doExecuteWithResult(IProgressMonitor monitor,
			IAdaptable info) throws ExecutionException {
		if (!canExecute()) {
			throw new ExecutionException(
					"Invalid arguments in reorient link command"); //$NON-NLS-1$
		}
		if (reorientDirection == ReorientRequest.REORIENT_SOURCE) {
			return reorientSource();
		}
		if (reorientDirection == ReorientRequest.REORIENT_TARGET) {
			return reorientTarget();
		}
		throw new IllegalStateException();
	}

	/**
	 * @generated
	 */
	protected CommandResult reorientSource() throws ExecutionException {
		getOldSource().getParticipants().remove(getOldTarget());
		getNewSource().getParticipants().add(getOldTarget());
		return CommandResult.newOKCommandResult(referenceOwner);
	}

	/**
	 * @generated
	 */
	protected CommandResult reorientTarget() throws ExecutionException {
		getOldSource().getParticipants().remove(getOldTarget());
		getOldSource().getParticipants().add(getNewTarget());
		return CommandResult.newOKCommandResult(referenceOwner);
	}

	/**
	 * @generated
	 */
	protected OpenDDS.ApplicationTarget getOldSource() {
		return (OpenDDS.ApplicationTarget) referenceOwner;
	}

	/**
	 * @generated
	 */
	protected OpenDDS.ApplicationTarget getNewSource() {
		return (OpenDDS.ApplicationTarget) newEnd;
	}

	/**
	 * @generated
	 */
	protected OpenDDS.DomainParticipant getOldTarget() {
		return (OpenDDS.DomainParticipant) oldEnd;
	}

	/**
	 * @generated
	 */
	protected OpenDDS.DomainParticipant getNewTarget() {
		return (OpenDDS.DomainParticipant) newEnd;
	}
}
