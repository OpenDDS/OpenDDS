package org.opendds.modeling.sdk.codegen;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import javax.xml.bind.JAXBException;

import org.eclipse.core.resources.IResourceChangeEvent;
import org.eclipse.core.resources.IResourceChangeListener;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.editors.text.TextEditor;
import org.eclipse.ui.forms.editor.FormEditor;
import org.eclipse.ui.part.FileEditorInput;

public class CodeGenEditor extends FormEditor implements IResourceChangeListener {
	private int XMLEDITOR_INDEX;
	
	/**
	 * Used to indicated that the Code Generator specification data is newer
	 * (has been modified since the last synchronization) than the data
	 * retained in the XML text editor. 
	 */
	private boolean isModified;
	
 	/**
 	 * Form to present some of the Code Generator specification information
 	 * for viewing and modification.  This form also contains controls to
 	 * allow users to actually generate code from the model.
 	 */
 	private OutputsForm outputsForm;
	
 	/**
 	 * Form to present portions of the Code Generator specification
 	 * information for viewing and modification.  This form is intended to
 	 * allow specification of instance customizations and is likely to
 	 * become the transport parameter specification editor.
 	 */
 	private InstanceForm instanceForm;

	/**
	 * XML Text Editor used to allow the user to directly edit the Code
	 * Generator specification information.  This is set as the editor that
	 * is attached to the IEditorInput for the multi-page editor and will
	 * be synchronized with the data presented (and edited) in the forms
	 * on the other pages.
	 */
	private TextEditor xmlEditor;
	
	/**
	 * Code Generator specification data manager.  This encapsulates the JAXB
	 * interfaces used to synchronize the data with the XML format.
	 */
	private GeneratorManager manager;
	
	/**
	 * Listener used to propagate changes in the Code Generator specification
	 * data in the Forms to the XML text editor.
	 */
	private final IManagerListener managerListener = new IManagerListener() {
		@Override
		public void modelChanged(GeneratorManager manager) {
			dataModified();
		}
	};

	/**
	 * Used to suppress actions while we are still initializing the editor.
	 */
	private boolean isInitializing = true;
 	
	public CodeGenEditor() {
		super();
		ResourcesPlugin.getWorkspace().addResourceChangeListener(this);
		manager = new GeneratorManager();
		manager.addManagerListener(managerListener);
	}

	/**
	 * getManager - access the underlying form input data.
	 * 
	 * @return GeneratorManager
	 */
	public GeneratorManager getManager() {
		return manager;
	}
	
	@Override
	protected void pageChange(int newPageIndex) {
		// Editor lifecycle management for the form data and text editor.
		if(newPageIndex != XMLEDITOR_INDEX) {
			// The new page is a form.
			if( isDirty()) {
				// The XML editor contains changes, so synchronize the
				// manager data with the editor.  After synchronization,
				// the modification indicator is cleared.
				loadContentFromXMLEditor();
				isModified = false;
			}
		} else if(isModified) {
			// The new page is the XML text editor and the manager data
			// has been changed, so synchronize the editor with the
			// new manager data.  After synchronization, the modification
			// indicator is cleared.
			loadEditorFromContent();
			isModified = false;
		}
		super.pageChange(newPageIndex);
	}
	
	/**
	 * Called to indicate that the Code Generator specification data has
	 * changed.  This means that the data should be propagated to the XML
	 * text editor before it is viewed or saved next.
	 */
	public void dataModified() {
		if(isInitializing) {
			return;
		}
		boolean wasDirty = isDirty();
		isModified = true;
		if(!wasDirty) {
			firePropertyChange(IEditorPart.PROP_DIRTY);
		}
	}
	
	@Override
	protected void handlePropertyChange( int propertyId) {
		if(propertyId == IEditorPart.PROP_DIRTY) {
			isModified = isDirty();
		}
		super.handlePropertyChange(propertyId);
	}
	
	@Override
	public boolean isDirty() {
		// Include our Form data when determining if the complete multi-page
		// editor has been modified.
		return isModified || super.isDirty();
	}

	@Override
	protected void addPages() {
		outputsForm  = new OutputsForm( this,"org.opendds.modeling.sdk.forms.output",  "Generate");
		instanceForm = new InstanceForm(this,"org.opendds.modeling.sdk.forms.instance","Customize");
		xmlEditor    = new TextEditor();

		try {
			addPage(outputsForm);
			addPage(instanceForm);
			XMLEDITOR_INDEX = addPage(xmlEditor, getEditorInput());
		} catch (PartInitException e) {
			ErrorDialog.openError(
					getSite().getShell(),
					"Error adding form pages",
					e.getMessage(),
					e.getStatus());
		}
		initializeContent();
		updateTitle();
		setPageText(XMLEDITOR_INDEX, xmlEditor.getTitle());
	}

	/**
	 * Updates the title by the resource name.
	 */
	void updateTitle() {
		IEditorInput input = getEditorInput();
		setPartName( input.getName());
		setTitleToolTip(input.getToolTipText());
	}

	/**
	 * Synchronize the Form data with the Editor data on the GUI thread.
	 */
	private void initializeContent() {
		getContainer().getDisplay().asyncExec( new Runnable() {
			@Override
			public void run() {
				isInitializing = true;
				loadContentFromXMLEditor();
				isInitializing = false;
			}
		});
	}
	
	/**
	 * Synchronize the Form data with the XML text editor data.
	 */
	protected void loadContentFromXMLEditor() {
		try {
			InputStream in = new ByteArrayInputStream(
					xmlEditor.getDocumentProvider()
					         .getDocument(xmlEditor.getEditorInput())
					         .get()
					         .getBytes());
			manager.unmarshal(in);
			outputsForm.dataChanged();
		} catch (JAXBException e) {
			ErrorDialog.openError(
					getSite().getShell(),
					"Error parsing XML specification",
					e.getMessage(),
					null);
		}		
	}
	
	/**
	 * Synchronize the XML text editor data with the Form data.
	 */
	protected void loadEditorFromContent() {
		OutputStream result = new ByteArrayOutputStream();
		manager.marshal(result);
		xmlEditor.getDocumentProvider()
			.getDocument(xmlEditor.getEditorInput())
			.set(result.toString());
	}

	public void dispose() {
		ResourcesPlugin.getWorkspace().removeResourceChangeListener(this);
		super.dispose();
	}

	@Override
	public void doSave(IProgressMonitor monitor) {
		if( getActivePage() != XMLEDITOR_INDEX && isModified) {
			loadEditorFromContent();
		}
		isModified = false;
		getEditor(XMLEDITOR_INDEX).doSave(monitor);
	}

	@Override
	public void doSaveAs() {
		if( getActivePage() != XMLEDITOR_INDEX && isModified) {
			loadEditorFromContent();
		}
		isModified = false;
		IEditorPart editor = getEditor(XMLEDITOR_INDEX);
		editor.doSaveAs();
		setInput(editor.getEditorInput());
		updateTitle();
	}

	@Override
	public boolean isSaveAsAllowed() {
		return true;
	}

	@Override
	public void resourceChanged(final IResourceChangeEvent event) {
		if(event.getType() == IResourceChangeEvent.PRE_CLOSE){
			Display.getDefault().asyncExec(new Runnable(){
				public void run(){
					IWorkbenchPage[] pages = getSite().getWorkbenchWindow().getPages();
					for (int i = 0; i<pages.length; i++){
						if(((FileEditorInput)xmlEditor.getEditorInput()).getFile().getProject().equals(event.getResource())){
							IEditorPart editorPart = pages[i].findEditor(xmlEditor.getEditorInput());
							pages[i].closeEditor(editorPart,true);
						}
					}
				}            
			});
		}
		
	}

}
