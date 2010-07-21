package org.opendds.modeling.sdk.codegen;


import java.io.StringWriter;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.StringTokenizer;

import org.eclipse.core.resources.IMarker;
import org.eclipse.core.resources.IResourceChangeEvent;
import org.eclipse.core.resources.IResourceChangeListener;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FontDialog;
import org.eclipse.ui.*;
import org.eclipse.ui.editors.text.TextEditor;
import org.eclipse.ui.part.FileEditorInput;
import org.eclipse.ui.part.MultiPageEditorPart;
import org.eclipse.ui.ide.IDE;

/**
 * The code generation specification editor.
 * 
 * This editor has 4 pages:
 * <ul>
 * <li>page 0 specifies the model input files
 * <li>page 1 specifies the model target output directory or project
 * <li>page 2 defines and customizes any instances of the model
 * <li>page 3 is the raw XML format for the specification file
 * </ul>
 */
public class CodeGenEditor extends MultiPageEditorPart implements IResourceChangeListener{
	
	/** Editor for model source specification. */
	private InputsForm inputsForm;
	
	/** Editor for model target output specification. */
 	private OutputsForm outputsForm;
	
	/** Editor for defining and customizing instance specifications. */
 	private InstanceForm instanceForm;

	/** The XML text editor for the resource. */
	private TextEditor xmlEditor;
	
	/**
	 * Creates a code generation specification editor.
	 */
	public CodeGenEditor() {
		super();
		ResourcesPlugin.getWorkspace().addResourceChangeListener(this);
	}
	/**
	 * Create the input specification page.
	 */
	void createInputsForm() {
		inputsForm = new InputsForm(getContainer());
		int index = addPage(inputsForm);
		setPageText(index, "Model Input");
	}
	/**
	 * Create the target output specification page.
	 */
	void createOutputsForm() {
		outputsForm = new OutputsForm(getContainer());
		int index = addPage(outputsForm);
		setPageText(index, "Code Output");
	}
	/**
	 * Create the instance definition and customization page.
	 */
	void createInstanceForm() {
		instanceForm = new InstanceForm(getContainer());
		int index = addPage(instanceForm);
		setPageText(index, "Instance Customization");
	}
	/**
	 * Creates the page with the XML text editor showing the contents of the resource.
	 */
	void createXmlEditor() {
		try {
			xmlEditor = new TextEditor();
			int index = addPage(xmlEditor, getEditorInput());
			setPageText(index, xmlEditor.getTitle());
		} catch (PartInitException e) {
			ErrorDialog.openError(
				getSite().getShell(),
				"Error creating nested XML text editor",
				null,
				e.getStatus());
		}
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
	 * Creates the pages of the multi-page editor.
	 */
	protected void createPages() {
		createInputsForm();
		createOutputsForm();
		createInstanceForm();
		createXmlEditor();
		updateTitle();
	}
	public void setFocus() {
		int index = getActivePage();
		switch(index) {
		case 0:
			inputsForm.setFocus();
			break;
		case 1:
			outputsForm.setFocus();
			break;
		case 2:
			instanceForm.setFocus();
			break;
		case 3:
			xmlEditor.setFocus();
			break;
		}
	}
	/**
	 * The <code>MultiPageEditorPart</code> implementation of this 
	 * <code>IWorkbenchPart</code> method disposes all nested editors.
	 * Subclasses may extend.
	 */
	public void dispose() {
		ResourcesPlugin.getWorkspace().removeResourceChangeListener(this);
		super.dispose();
	}
	/**
	 * Saves the multi-page editor's document.
	 */
	public void doSave(IProgressMonitor monitor) {
		getEditor(3).doSave(monitor);
	}
	/**
	 * Saves the multi-page editor's document as another file.
	 * Also updates the text for page 3's tab, and updates this multi-page editor's input
	 * to correspond to the nested editor's.
	 */
	public void doSaveAs() {
		IEditorPart editor = getEditor(3);
		editor.doSaveAs();
		setPageText(3, editor.getTitle());
		setInput(editor.getEditorInput());
	}
	/* (non-Javadoc)
	 * Method declared on IEditorPart
	 */
	public void gotoMarker(IMarker marker) {
		setActivePage(3);
		IDE.gotoMarker(getEditor(3), marker);
	}
	/**
	 * The <code>MultiPageEditorExample</code> implementation of this method
	 * checks that the input is an instance of <code>IFileEditorInput</code>.
	 */
	public void init(IEditorSite site, IEditorInput editorInput)
		throws PartInitException {
		if (!(editorInput instanceof IFileEditorInput))
			throw new PartInitException("Invalid Input: Must be IFileEditorInput");
		super.init(site, editorInput);
	}
	/* (non-Javadoc)
	 * Method declared on IEditorPart.
	 */
	public boolean isSaveAsAllowed() {
		return true;
	}
	/**
	 * Closes all project files on project close.
	 */
	public void resourceChanged(final IResourceChangeEvent event){
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
