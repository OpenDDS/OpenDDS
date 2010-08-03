package org.opendds.modeling.sdk.codegen;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import javax.xml.bind.JAXBException;

import org.eclipse.core.resources.IResourceChangeEvent;
import org.eclipse.core.resources.IResourceChangeListener;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.jface.dialogs.IPageChangedListener;
import org.eclipse.jface.dialogs.PageChangedEvent;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.editors.text.TextEditor;
import org.eclipse.ui.forms.editor.FormEditor;
import org.eclipse.ui.part.FileEditorInput;

public class CodeGenForm extends FormEditor implements IResourceChangeListener {
	private int XMLEDITOR_INDEX;
	
	/** Editor for model target output specification. */
 	private OutputsForm outputsForm;
	
	/** Editor for defining and customizing instance specifications. */
 	private InstanceForm instanceForm;

	/** The XML text editor for the resource. */
	private TextEditor xmlEditor;
	
	/** Manage the specification contents while editing. */
	private GeneratorManager manager;
	
	private List<IDataChangedListener> dataChangedListeners;
 	
	public CodeGenForm() {
		super();
		ResourcesPlugin.getWorkspace().addResourceChangeListener(this);
		manager = new GeneratorManager();
		dataChangedListeners = new ArrayList<IDataChangedListener>();

		// Propagate changes from the XML editor to the forms.
		addPageChangedListener(new IPageChangedListener() {
			@Override
			public void pageChanged(PageChangedEvent event) {
				if(xmlEditor.isDirty()) {
					initializeContent();
				}
			}
		});
	}
	
	@Override
	protected void pageChange(int newPageIndex) {
		if(newPageIndex == XMLEDITOR_INDEX && manager.isUpdated()) {
			OutputStream result = new ByteArrayOutputStream();
			manager.marshal(result);
			xmlEditor.getDocumentProvider()
				.getDocument(xmlEditor.getEditorInput())
				.set(result.toString());
		}
		super.pageChange(newPageIndex);
	}
	
	public void addDataChangedListener( IDataChangedListener dataChangedListener) {
		if( !dataChangedListeners.contains(dataChangedListener)) {
			this.dataChangedListeners.add( dataChangedListener);
		}
	}
	
	public void removeDataChangedListener( IDataChangedListener dataChangedListener) {
		this.dataChangedListeners.remove( dataChangedListener);
	}

	public GeneratorManager getManager() {
		return manager;
	}

	@Override
	protected void addPages() {
		outputsForm = new OutputsForm(this,"org.opendds.modeling.sdk.forms.output","Generate");
		instanceForm = new InstanceForm(this,"org.opendds.modeling.sdk.forms.instance","Customize");
		xmlEditor = new TextEditor();

		addDataChangedListener(outputsForm);
		addDataChangedListener(instanceForm);

		updateTitle();
		initializeContent();

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

	private void initializeContent() {
		getContainer().getDisplay().asyncExec( new Runnable() {
			@Override
			public void run() {
				loadContentFromXMLEditor();
			}
		});
	}
	
	protected void loadContentFromXMLEditor() {
		try {
			InputStream in = new ByteArrayInputStream(
					xmlEditor.getDocumentProvider()
					         .getDocument(xmlEditor.getEditorInput())
					         .get()
					         .getBytes());
			manager.unmarshal(in);
			fireInputChanged();
		} catch (JAXBException e) {
			ErrorDialog.openError(
					getSite().getShell(),
					"Error parsing XML specification",
					e.getMessage(),
					null);
		}		
	}

	private void fireInputChanged() {
		for( IDataChangedListener listener: dataChangedListeners) {
			listener.dataChanged();
		}
	}

	public void dispose() {
		ResourcesPlugin.getWorkspace().removeResourceChangeListener(this);
		super.dispose();
	}

	@Override
	public void doSave(IProgressMonitor monitor) {
		getEditor(XMLEDITOR_INDEX).doSave(monitor);
		initializeContent();
	}

	@Override
	public void doSaveAs() {
		IEditorPart editor = getEditor(XMLEDITOR_INDEX);
		editor.doSaveAs();
		setInput(editor.getEditorInput());
		updateTitle();
		initializeContent();
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
