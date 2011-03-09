package org.opendds.modeling.common.gmf;


import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EFactory;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.edit.command.SetCommand;
import org.eclipse.emf.transaction.TransactionalEditingDomain;
import org.eclipse.gef.EditPart;
import org.eclipse.gmf.runtime.common.core.util.StringStatics;
import org.eclipse.gmf.runtime.common.ui.util.StatusLineUtil;
import org.eclipse.gmf.runtime.diagram.ui.properties.sections.AbstractBasicTextPropertySection;
import org.eclipse.gmf.runtime.diagram.ui.properties.views.TextChangeHelper;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.PlatformUI;
import org.opendds.modeling.model.core.Comment;
import org.opendds.modeling.model.core.CorePackage;
import org.opendds.modeling.model.core.Element;

/**
 * Based on code given in Eclipse Modeling Project, 4.4, page 142.
 * @generated NOT
 */
public class ElementCommentPropertySection extends
		AbstractBasicTextPropertySection {

	/**
	 * A modified version of the TextChangeHelper in
	 * org.eclipse.gmf.runtime.diagram.ui.properties.sections.AbstractBasicTextPropertySection
	 * that does not treat carriage returns as changing the text.
	 * This was causing CRs to be ignored when entered by the user.
	 * @generated NOT
	 */
	class MultilineTextChangeHelper extends TextChangeHelper {
		boolean textModified = false;

		@Override
		public void handleEvent(Event event) {
			switch (event.type) {
			case SWT.KeyDown:
				textModified = true;
				break;
			case SWT.FocusOut:
				textChanged((Control) event.widget);
				break;
			}
		}

		@Override
		public void textChanged(Control control) {
			if (textModified) {
				// clear error message
				IWorkbenchPart part = PlatformUI.getWorkbench()
						.getActiveWorkbenchWindow().getActivePage()
						.getActivePart();
				StatusLineUtil.outputErrorMessage(part, StringStatics.BLANK);

				setPropertyValue(control);
				textModified = false;
			}
		}
	}

	private MultilineTextChangeHelper multilineListener = new MultilineTextChangeHelper();

	@Override
	protected String getPropertyChangeCommandName() {
		return "ElementDescriptionChangeCommand";
	}

	@Override
	protected String getPropertyNameLabel() {
		return "";
	}

	@Override
	protected String getPropertyValueString() {
		Element element = (Element) getEObject();
		Comment comment = element.getComment();
		return comment != null && comment.getBody() != null ? comment.getBody()
				: "";
	}

	@Override
	protected void setPropertyValue(EObject object,
			Object value) {
		Element element = (Element) getEObject();
		Comment comment = element.getComment();
		if (element.getComment() == null) {
			// The comment is an optional reference, so create one
			// to have a place to put the value in.
			EPackage ePackage = EPackage.Registry.INSTANCE
					.getEPackage(CorePackage.eNS_URI);
			EFactory factory = ePackage.getEFactoryInstance();
			EClass commentClass = (EClass) ePackage.getEClassifier("Comment");
			EStructuralFeature commentRef = element.eClass()
					.getEStructuralFeature("comment");
			comment = (Comment) factory.create(commentClass);

			// Need to add the comment reference to the element inside a transaction.
			TransactionalEditingDomain editingDomain = getEditingDomain();
			SetCommand command = new SetCommand(editingDomain, element,
					commentRef, comment);
			assert command.canExecute();
			editingDomain.getCommandStack().execute(command);
		}
		comment.setBody((String) value);
	}

	@Override
	protected Text createTextWidget(Composite parent) {
		Text text = getWidgetFactory().createText(parent, StringStatics.BLANK,
				SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.WRAP);
		FormData data = new FormData();
		data.left = new FormAttachment(0, 0);
		data.right = new FormAttachment(100, 0);
		data.top = new FormAttachment(0, 0);
		data.bottom = new FormAttachment(100, 0);
		data.height = 100;
		data.width = 100;
		text.setLayoutData(data);
		if (isReadOnly()) {
			text.setEditable(false);
		}
		return text;
	}

	@Override
	protected EObject unwrap(Object object) {
		if (object instanceof Element) {
			return (EObject) object;
		}
		if (object instanceof EditPart) {
			Object model = ((EditPart) object).getModel();
			return model instanceof View ? ((View) model).getElement() : null;
		}
		if (object instanceof View) {
			return ((View) object).getElement();
		}
		if (object instanceof IAdaptable) {
			View view = (View) ((IAdaptable) object).getAdapter(View.class);
			if (view != null) {
				return view.getElement();
			}
		}
		return null;
	}

	/*
	 * Provide a listener that does not treat carriage return
	 * @see org.eclipse.gmf.runtime.diagram.ui.properties.sections.AbstractBasicTextPropertySection#getListener()
	 */
	@Override
	protected TextChangeHelper getListener() {
		return multilineListener;
	}

}
