package org.opendds.modeling.diagram.datalib.providers;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.transaction.TransactionalEditingDomain;
import org.eclipse.emf.transaction.util.TransactionUtil;
import org.eclipse.emf.workspace.util.WorkspaceSynchronizer;
import org.eclipse.gmf.runtime.common.core.command.CommandResult;
import org.eclipse.gmf.runtime.common.core.command.ICommand;
import org.eclipse.gmf.runtime.common.core.service.AbstractProvider;
import org.eclipse.gmf.runtime.common.core.service.IOperation;
import org.eclipse.gmf.runtime.common.ui.services.parser.GetParserOperation;
import org.eclipse.gmf.runtime.common.ui.services.parser.IParser;
import org.eclipse.gmf.runtime.common.ui.services.parser.IParserEditStatus;
import org.eclipse.gmf.runtime.common.ui.services.parser.IParserProvider;
import org.eclipse.gmf.runtime.common.ui.services.parser.ParserEditStatus;
import org.eclipse.gmf.runtime.emf.commands.core.command.AbstractTransactionalCommand;
import org.eclipse.gmf.runtime.emf.ui.services.parser.ISemanticParser;
import org.eclipse.jface.text.contentassist.IContentAssistProcessor;
import org.opendds.modeling.diagram.datalib.edit.parts.KeyEditPart;
import org.opendds.modeling.diagram.datalib.part.OpenDDSDataLibVisualIDRegistry;
import org.opendds.modeling.model.types.Field;
import org.opendds.modeling.model.types.Key;
import org.opendds.modeling.model.types.Struct;
import org.opendds.modeling.model.types.TypesPackage;



/**
 * Allows Struct Keys to be specified using "name: Field" form to avoid excessive
 * Key figures in a diagram.
 * Based on code given in Eclipse Modeling Project book, section 4.6.7 Custom Parsers.
 * @generated NOT
 *
 */
public class StructKeyParserProvider extends AbstractProvider implements IParserProvider {

	private final class KeyParser implements ISemanticParser {
		public IContentAssistProcessor getCompletionProcessor(IAdaptable element) {
			return null;
		}

		@Override
		public String getEditString(IAdaptable element, int flags) {
			Key key = getKey(element);
			String editString = "";
			if (key.getField() != null && key.getField().getName() != null) {
				editString = key.getField().getName();
			}
			return editString;
		}

		@Override
		public ICommand getParseCommand(IAdaptable element, final String newString, int flags) {
			final String fieldName = newString;

			final Key key = getKey(element);
			final Field field = findField(fieldName, key);

			TransactionalEditingDomain editingDomain = TransactionUtil.getEditingDomain(key);
			return new AbstractTransactionalCommand(editingDomain, "", Collections.singletonList(WorkspaceSynchronizer.getFile(key.eResource()))) {

				@Override
				protected CommandResult doExecuteWithResult(IProgressMonitor monitor, IAdaptable info) throws ExecutionException {
					if (newString.length() == 0) {
						return CommandResult.newErrorCommandResult("Invalid input");
					}
					key.setField(field);
					return CommandResult.newOKCommandResult();
				}

			};
		}

		private Field findField(final String fieldName, final Key key) {
			if (fieldName.length() > 0) {
				Struct struct = (Struct) key.eContainer();
				for (Field structField: struct.getFields()) {
					if (structField.getName().equals(fieldName)) {
						return structField;
					}
				}
			}
			return null;
		}

		@Override
		public String getPrintString(IAdaptable element, int flags) {
			String printString = getEditString(element, flags);
			return printString.length() == 0 ? "<...>" : printString;
		}

		@Override
		public boolean isAffectingEvent(Object event, int flags) {
			if (event instanceof Notification) {
				Notification emfNotification = (Notification) event;
				return !emfNotification.isTouch()
				&& (emfNotification.getFeature() == TypesPackage.eINSTANCE.getKey_Field());
			}
			return false;
		}

		@Override
		public IParserEditStatus isValidEditString(IAdaptable element, String editString) {
			return ParserEditStatus.EDITABLE_STATUS;
		}

		private Key getKey(IAdaptable adaptable) {
			return (Key) adaptable.getAdapter(EObject.class);
		}

		@Override
		public boolean areSemanticElementsAffected(EObject listener, Object notification) {
			if (notification instanceof Notification) {
				Notification emfNotification = (Notification) notification;
				return !emfNotification.isTouch()
				&& (emfNotification.getFeature() == TypesPackage.eINSTANCE.getKey() || emfNotification.getFeature() == TypesPackage.eINSTANCE.getField());
			}
			return false;
		}

		@Override
		public List<EObject> getSemanticElementsBeingParsed(EObject element) {
			List<EObject> result = new ArrayList<EObject>();
			if (element instanceof Key) {
				result.add(element);
			}
			return result;
		}
	}

	private IParser myParser;

	@Override
	public IParser getParser(IAdaptable hint) {
		if (myParser == null) {
			myParser = new KeyParser();
		}
		return myParser;
	}

	@Override
	public boolean provides(IOperation operation) {
		if (operation instanceof GetParserOperation) {
			IAdaptable hint = ((GetParserOperation) operation).getHint();
			String visualID = (String) hint.getAdapter(String.class);
			return KeyEditPart.VISUAL_ID == OpenDDSDataLibVisualIDRegistry.getVisualID(visualID) && hint.getAdapter(EObject.class) instanceof Key;
		}
		return false;
	}
}

