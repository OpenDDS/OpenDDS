package org.opendds.modeling.model.opendds.diagram.datalib.providers;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
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

import org.opendds.modeling.model.opendds.diagram.datalib.edit.parts.Field2EditPart;
import org.opendds.modeling.model.opendds.diagram.datalib.edit.parts.Field3EditPart;
import org.opendds.modeling.model.opendds.diagram.datalib.edit.parts.FieldEditPart;
import org.opendds.modeling.model.opendds.diagram.datalib.part.OpenDDSDataLibVisualIDRegistry;
import org.opendds.modeling.model.types.DataLib;
import org.opendds.modeling.model.types.Field;
import org.opendds.modeling.model.types.TypesPackage;
import org.opendds.modeling.model.types.Type;

import com.ociweb.emf.util.ObjectsFinder;



/**
 * Allows Fields to be specified using "name: Type" form to avoid excessive
 * Field figures in a diagram.
 * Based on code given in Eclipse Modeling Project book, section 4.6.7 Custom Parsers.
 * @generated NOT
 *
 */
public class FieldParserProvider extends AbstractProvider implements IParserProvider {

	private final class FieldParser implements ISemanticParser {
		public IContentAssistProcessor getCompletionProcessor(IAdaptable element) {
			return null;
		}

		@Override
		public String getEditString(IAdaptable element, int flags) {
			Field field = getField(element);
			return field.getName() != null ? field.getName() + " : " + (field.getType() != null ? getTypeName(field.getType()) : "") : "";
		}

		@Override
		public ICommand getParseCommand(IAdaptable element, final String newString, int flags) {
			int index = newString.indexOf(":");
			final String name;
			final String typeName;
			if (index == 0) {
				name = "";
				typeName = newString.substring(index + 1);
			} else if (index > 0) {
				name = newString.substring(0, index).trim();
				typeName = newString.substring(index + 1).trim();
			} else if (index == -1 && newString.length() > 0) {
				name = newString;
				typeName = "";
			} else {
				name = "";
				typeName = "";
			}

			final Field field = getField(element);
			final Type type = findType(typeName, field);

			TransactionalEditingDomain editingDomain = TransactionUtil.getEditingDomain(field);
			return new AbstractTransactionalCommand(editingDomain, "", Collections.singletonList(WorkspaceSynchronizer.getFile(field.eResource()))) {

				@Override
				protected CommandResult doExecuteWithResult(IProgressMonitor monitor, IAdaptable info) throws ExecutionException {
					if (newString.length() == 0) {
						return CommandResult.newErrorCommandResult("Invalid input");
					}
					field.setName(name);
					field.setType(type);
					return CommandResult.newOKCommandResult();
				}

			};
		}

		private Type findType(final String typeName, final Field field) {
			Type type = null;
			if (typeName.length() > 0) {
				DataLib dataLib = (DataLib) ObjectsFinder.findObjectInContainmentTree(field, TypesPackage.eINSTANCE.getDataLib());
				type = findInDataLib(dataLib, typeName);
			}
			return type;
		}

		private Type findInDataLib(DataLib lib, String typeName) {
			for (Type element : lib.getTypes()) {
				if (typeName.equals(getTypeName(element))) {
					return element;
				}
				if (element instanceof DataLib) {
					return findInDataLib((DataLib) element, typeName);
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
						&& (emfNotification.getFeature() == TypesPackage.eINSTANCE.getField_Type());
			}
			return false;
		}

		@Override
		public IParserEditStatus isValidEditString(IAdaptable element, String editString) {
			return ParserEditStatus.EDITABLE_STATUS;
		}

		private Field getField(IAdaptable adaptable) {
			return (Field) adaptable.getAdapter(EObject.class);
		}

		@Override
		public boolean areSemanticElementsAffected(EObject listener, Object notification) {
			if (notification instanceof Notification) {
				Notification emfNotification = (Notification) notification;
				return !emfNotification.isTouch()
						&& (emfNotification.getFeature() == TypesPackage.eINSTANCE.getField() || emfNotification.getFeature() == TypesPackage.eINSTANCE.getType());
			}
			return false;
		}

		@Override
		public List<EObject> getSemanticElementsBeingParsed(EObject element) {
			List<EObject> result = new ArrayList<EObject>();
			if (element instanceof Field) {
				result.add(element);
			}
			return result;
		}
		
		/**
		 * Determine the name for allowable Types to be referenced by a Field.
		 * For primitive types (Simple, String, WString), return the name of the EClass itself.
		 * @param type
		 * @return
		 */
		private String getTypeName(Type type) {
			String name = null;
			EClass typeClass = type.eClass();
			
			// Determine if type has an attribute named "name". If so, use that.
			for (Iterator iter = typeClass.getEAllAttributes().iterator(); iter.hasNext(); ) {
				EAttribute attribute = (EAttribute) iter.next();
				if (attribute.getName() == "name") {
					Object nameAttribute = type.eGet(attribute);
					Class iClass = attribute.getEAttributeType().getInstanceClass();
					if (String.class.equals(iClass)) {
						return (String) nameAttribute;
					}
				}
			}
			
			EPackage typesPackage = EPackage.Registry.INSTANCE.getEPackage(TypesPackage.eNS_URI);

			// Determine if type is a "basic" type (does not contain a name attribute). If so, use name of type.
			String[] basicClassNames = {"Simple", "String", "WString"};
			for (String basicClassName: basicClassNames) {
				EClass basicClass = (EClass) typesPackage.getEClassifier(basicClassName);
				if (basicClass.isSuperTypeOf(typeClass)) {
					return typeClass.getName();
				}
			}

			return name;
		}
	}

	private IParser myParser;

	@Override
	public IParser getParser(IAdaptable hint) {
		if (myParser == null) {
			myParser = new FieldParser();
		}
		return myParser;
	}

	@Override
	public boolean provides(IOperation operation) {
		if (operation instanceof GetParserOperation) {
			IAdaptable hint = ((GetParserOperation) operation).getHint();
			String visualID = (String) hint.getAdapter(String.class);
			int id = OpenDDSDataLibVisualIDRegistry.getVisualID(visualID);
			// The edit part could be for a Struct, Union, or Branch.
			boolean editPartMatch =
				id == FieldEditPart.VISUAL_ID ||
				id == Field2EditPart.VISUAL_ID ||
				id == Field3EditPart.VISUAL_ID;
			return  editPartMatch && hint.getAdapter(EObject.class) instanceof Field;
		}
		return false;
	}
}
