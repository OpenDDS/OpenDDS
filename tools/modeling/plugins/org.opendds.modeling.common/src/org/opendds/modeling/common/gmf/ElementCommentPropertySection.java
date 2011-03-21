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
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.views.properties.tabbed.ITabbedPropertyConstants;
import org.eclipse.ui.views.properties.tabbed.TabbedPropertySheetPage;
import org.opendds.modeling.model.core.Comment;
import org.opendds.modeling.model.core.CommentFormat;
import org.opendds.modeling.model.core.CorePackage;
import org.opendds.modeling.model.core.Element;

/**
 * Based on org.eclipse.gmf.runtime.diagram.ui.properties.sections.DiagramGeneralSection
 * @generated NOT
 */
public class ElementCommentPropertySection extends
		AbstractBasicTextPropertySection {

	static final CommentFormat defaultCommentFormat = getDefaultCommentFormat();
	static final String defaultCommentFormatString = defaultCommentFormat.getName();

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

	private CLabel formatLabel;

	private CCombo formatCombo;

	private synchronized void setCommentFormat() {
		if (formatCombo != null) {
			Element element = (Element) getEObject();
			CommentFormat format = CommentFormat.get(formatCombo
					.getSelectionIndex());
			Comment comment = element.getComment();
			if (comment == null) {
				comment = createComment(element, format);
			} else {
				TransactionalEditingDomain editingDomain = getEditingDomain();
				EStructuralFeature formatRef = comment.eClass().getEStructuralFeature("format");
				SetCommand command = new SetCommand(editingDomain, comment, formatRef, format);
				assert command.canExecute();
				editingDomain.getCommandStack().execute(command);
			}
		}
	}

	@Override
	public void doCreateControls(Composite parent,
			TabbedPropertySheetPage aTabbedPropertySheetPage) {
		super.doCreateControls(parent, aTabbedPropertySheetPage);
		doCreateFormatCombo();
	}

	private void doCreateFormatCombo() {
		FormData data;

		// Text label for combo box
		formatLabel = getWidgetFactory().createCLabel(getSectionComposite(), "Format:");
		data = new FormData();
		data.top = new FormAttachment(getTextWidget(), ITabbedPropertyConstants.VSPACE, SWT.BOTTOM);
		data.bottom = new FormAttachment(100, 0);
		formatLabel.setLayoutData(data);

		// Combo box
		formatCombo = getWidgetFactory().createCCombo(getSectionComposite());
		CommentFormat[] formatValues = CommentFormat.values();
		int i = 0;
		String[] formats = new String[formatValues.length];
		for (CommentFormat formatValue : formatValues) {
			formats[i++] = formatValue.name();
		}

		// Attempt was made to use CCombo.setText() to set the default
		// combo box option, but this did not work. So instead, we'll force
		// the issue by making the default format the first one added to
		// the combo box.
		formatCombo.add(defaultCommentFormatString.toLowerCase());
		for (String format : formats) {
			if (! format.equals(defaultCommentFormatString)) {
				formatCombo.add(format.toLowerCase());
			}
		}

		data = new FormData();
		data.left = new FormAttachment(formatLabel, 0, SWT.RIGHT);
		data.bottom = new FormAttachment(100, 0);
		formatCombo.setLayoutData(data);
		formatCombo.pack();

		formatCombo.addFocusListener(new FocusListener() {

			@Override
			public void focusLost(FocusEvent e) {
				setCommentFormat();
			}

			@Override
			public void focusGained(FocusEvent e) {
			}
		});

	}

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
	protected void setPropertyValue(Control control) {
		if (control == getTextWidget())
			super.setPropertyValue(control);
		else
			setCommentFormat();

	}

	@Override
	protected void setPropertyValue(EObject object,
			Object value) {
		Element element = (Element) getEObject();
		Comment comment = element.getComment();
		if (comment == null) {
			comment = createComment(element, defaultCommentFormat);
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
		data.bottom = new FormAttachment(80, 0);
		text.setLayoutData(data);
		if (isReadOnly()) {
			text.setEditable(false);
		}
		return text;
	}

	@Override
	protected void refreshUI() {
		super.refreshUI();
		Element element = (Element) getEObject();
		Comment comment = element.getComment();
		int enumValue = 0;
		if (comment != null) {
			enumValue = comment.getFormat().getValue();
		}
		formatCombo.select(enumValue);
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

	private Comment createComment(Element element, CommentFormat format) {
		// The comment is an optional reference, so create one
		// to have a place to put the value in.
		Comment comment = element.getComment();
		EPackage ePackage = EPackage.Registry.INSTANCE
		.getEPackage(CorePackage.eNS_URI);
		EFactory factory = ePackage.getEFactoryInstance();
		EClass commentClass = (EClass) ePackage.getEClassifier("Comment");
		comment = (Comment) factory.create(commentClass);
		comment.setFormat(format);

		// Need to add the comment reference to the element inside a transaction.
		TransactionalEditingDomain editingDomain = getEditingDomain();
		EStructuralFeature commentRef = element.eClass().getEStructuralFeature("comment");
		SetCommand command = new SetCommand(editingDomain, element,
				commentRef, comment);
		assert command.canExecute();
		editingDomain.getCommandStack().execute(command);
		return comment;

	}

	/**
	 * Get the default comment format as a String.
	 * This is the default value specified in the
	 * Comment Ecore model.
	 */
	static private CommentFormat getDefaultCommentFormat() {
		// Get the CommentFormat from a Comment that has just
		// been constructed, which has the default format.
		// (Couldn't find an EMF API to get more directly.)
		EPackage ePackage = EPackage.Registry.INSTANCE
		.getEPackage(CorePackage.eNS_URI);
		EFactory factory = ePackage.getEFactoryInstance();
		EClass commentClass = (EClass) ePackage.getEClassifier("Comment");
		Comment comment = (Comment) factory.create(commentClass);
		return comment.getFormat();
	}
}
