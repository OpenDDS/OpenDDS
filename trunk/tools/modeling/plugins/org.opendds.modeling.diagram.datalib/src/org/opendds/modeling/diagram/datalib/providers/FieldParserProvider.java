package org.opendds.modeling.diagram.datalib.providers;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.util.BasicEList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.TreeIterator;
import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;
import org.eclipse.emf.transaction.TransactionalEditingDomain;
import org.eclipse.emf.transaction.util.TransactionUtil;
import org.eclipse.emf.workspace.util.WorkspaceSynchronizer;
import org.eclipse.gmf.runtime.common.core.command.CommandResult;
import org.eclipse.gmf.runtime.common.core.command.ICommand;
import org.eclipse.gmf.runtime.common.core.command.ICompositeCommand;
import org.eclipse.gmf.runtime.common.core.service.AbstractProvider;
import org.eclipse.gmf.runtime.common.core.service.IOperation;
import org.eclipse.gmf.runtime.common.ui.services.parser.GetParserOperation;
import org.eclipse.gmf.runtime.common.ui.services.parser.IParser;
import org.eclipse.gmf.runtime.common.ui.services.parser.IParserEditStatus;
import org.eclipse.gmf.runtime.common.ui.services.parser.IParserProvider;
import org.eclipse.gmf.runtime.common.ui.services.parser.ParserEditStatus;
import org.eclipse.gmf.runtime.emf.commands.core.command.AbstractTransactionalCommand;
import org.eclipse.gmf.runtime.emf.commands.core.command.CompositeTransactionalCommand;
import org.eclipse.gmf.runtime.emf.ui.services.parser.ISemanticParser;
import org.eclipse.jface.text.contentassist.IContentAssistProcessor;

import org.opendds.modeling.model.opendds.OpenDDSModel;
import org.opendds.modeling.common.gmf.BasicTypeIdentifier;
import org.opendds.modeling.diagram.datalib.edit.parts.Field2EditPart;
import org.opendds.modeling.diagram.datalib.edit.parts.Field3EditPart;
import org.opendds.modeling.diagram.datalib.edit.parts.FieldEditPart;
import org.opendds.modeling.diagram.datalib.part.OpenDDSDataLibVisualIDRegistry;
import org.opendds.modeling.model.types.Field;
import org.opendds.modeling.model.types.TypesPackage;
import org.opendds.modeling.model.types.Type;


/**
 * Allows Fields to be specified using "name: Type" form to avoid excessive
 * Field figures in a diagram.
 * Based on code given in Eclipse Modeling Project book, section 4.6.7 Custom Parsers.
 * @generated NOT
 *
 */
public class FieldParserProvider extends AbstractProvider implements IParserProvider {

	private final class FieldParser implements ISemanticParser {

		private ICommand basicTypesCreateCommand = null;
		private String basicTypeCreatedName = null;

		/**
		 * Because the Ecore will be modified to add basic types, an EMF Command must
		 * be used since all Ecore changes must done through Commands managed by an EditingDomain
		 * (see for example the EMF book, section 3.3.3).
		 * This Command must be executed before FieldSetCommand so that FieldSetCommand
		 * can use the results of this command for setting the Field.
		 */
		private final class BasicTypesCreateCommand extends AbstractTransactionalCommand {

			private OpenDDSModel openDDSModel;

			private BasicTypesCreateCommand(TransactionalEditingDomain domain,
					String label, List affectedFiles, OpenDDSModel openDDSModel) {
				super(domain, label, affectedFiles);
				this.openDDSModel = openDDSModel;
			}

			@Override
			protected CommandResult doExecuteWithResult(
					IProgressMonitor monitor, IAdaptable info)
					throws ExecutionException {
				Type dataTypeCreated = BasicTypesSupplier.create(openDDSModel, basicTypeCreatedName);
				return CommandResult.newOKCommandResult(dataTypeCreated);
			}
		}

		private final class FieldSetCommand extends
				AbstractTransactionalCommand {
			private final String name;
			private final String newString;
			private Type type;
			private final Field field;

			private FieldSetCommand(TransactionalEditingDomain domain,
					String label, List affectedFiles, String name,
					String newString, Type type, Field field) {
				super(domain, label, affectedFiles);
				this.name = name;
				this.newString = newString;
				this.type = type;
				this.field = field;
			}

			@Override
			protected CommandResult doExecuteWithResult(IProgressMonitor monitor, IAdaptable info) throws ExecutionException {
				if (newString.length() == 0) {
					return CommandResult.newErrorCommandResult("Invalid input");
				}
				field.setName(name);
				if (type == null && basicTypesCreateCommand != null) {
					CommandResult createResult = basicTypesCreateCommand.getCommandResult();
					type = (Type) createResult.getReturnValue();
				}
				field.setType(type);
				return CommandResult.newOKCommandResult();
			}
		}

		public IContentAssistProcessor getCompletionProcessor(IAdaptable element) {
			return null;
		}

		@Override
		public String getEditString(IAdaptable element, int flags) {
			Field field = getField(element);
			String editString = "";
			if (field.getName() != null) {
				editString = field.getName() + " : ";
			}

			Type fieldType = field.getType();
			if (fieldType != null) {
				String fieldName = "";
				if (BasicTypeIdentifier.isBasic(fieldType.eClass())) {
					fieldName = fieldType.eClass().getName();
				}
				else {
					fieldName = getTypeName(field.getType());
				}
				editString += fieldName;
			}
			return editString;
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
			ICompositeCommand cc = new CompositeTransactionalCommand(editingDomain, "Field Set");

			if (basicTypesCreateCommand != null) {
				cc.add(basicTypesCreateCommand);
			}

			FieldSetCommand fieldSetCommand = new FieldSetCommand(editingDomain, "", Collections.singletonList(WorkspaceSynchronizer.getFile(field.eResource())), name, newString, type,
					field);
			cc.add(fieldSetCommand);

			return cc;
		}

		private Type findType(final String typeName, final Field field) {
			basicTypesCreateCommand = null;
			basicTypeCreatedName = null;
			if (typeName.length() == 0) {
				return null;
			}

			// For basic types, always use a type instance in OpenDDSModel.basicTypes
			if (BasicTypesSupplier.getTypeClass(typeName) != null) {
				EObject rootContainer = com.ociweb.emf.util.ObjectsFinder.findRootContainerObject(field);
				assert rootContainer instanceof OpenDDSModel;
				OpenDDSModel openDDSModel = (OpenDDSModel) rootContainer;
				Type basicType = BasicTypesSupplier.getType(openDDSModel, typeName);
				if (basicType == null) {
					TransactionalEditingDomain editingDomain = TransactionUtil.getEditingDomain(field);
					basicTypesCreateCommand = new BasicTypesCreateCommand(editingDomain, null, null, openDDSModel);
					basicTypeCreatedName = typeName;
				}
				else {
					return basicType;
				}
			}
			else {
				EList<Type> reachableTypes = getReachableTypes(field);
				for (Type typeCandidate: reachableTypes) {
					if (typeName.equals(getTypeName(typeCandidate))) {
						return typeCandidate;
					}
				}
			}
			return null;
		}

		/**
		 * Search across loaded resources for candidate types
		 */
		private EList<Type> getReachableTypes(EObject obj) {
			EList<Type> types = new BasicEList<Type>();

			Resource resource = obj.eResource();
			if (resource != null) {
				ResourceSet resourceSet = resource.getResourceSet();
				if (resourceSet != null) {
					for (TreeIterator<?> i = resourceSet.getAllContents(); i.hasNext(); ) {
						Object child = i.next();
						if (child instanceof Type) {
							types.add((Type) child);
						}
					}
				}
				else
				{
					for (EObject eObject : resource.getContents()) {
						if (eObject instanceof Type) {
							types.add((Type) eObject);
						}

					}
				}
			}
			return types;
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
		 * @param type
		 * @return
		 */
		private String getTypeName(EObject type) {
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
