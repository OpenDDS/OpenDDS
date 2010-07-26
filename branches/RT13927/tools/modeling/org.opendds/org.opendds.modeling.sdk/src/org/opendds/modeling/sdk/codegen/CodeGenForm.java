package org.opendds.modeling.sdk.codegen;

import java.io.ByteArrayInputStream;
import java.io.InputStream;

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
import org.eclipse.ui.texteditor.DocumentProviderRegistry;
import org.eclipse.ui.texteditor.IDocumentProvider;

public class CodeGenForm extends FormEditor implements IResourceChangeListener {
	private static final int INPUTFORM_INDEX = 0;
	private static final int OUTPUTFORM_INDEX = 1;
	private static final int INSTANCEFORM_INDEX = 2;
	private static final int XMLEDITOR_INDEX = 3;

	/** Editor for model source specification. */
	private InputsForm inputsForm;
	
	/** Editor for model target output specification. */
 	private OutputsForm outputsForm;
	
	/** Editor for defining and customizing instance specifications. */
 	private InstanceForm instanceForm;

	/** The XML text editor for the resource. */
	private TextEditor xmlEditor;
	
	/** Manage the specification contents while editing. */
	private GeneratorManager manager;

	/** The editor's explicit document provider. */
	private IDocumentProvider provider;
	
	public CodeGenForm() {
		super();
		ResourcesPlugin.getWorkspace().addResourceChangeListener(this);
		manager = new GeneratorManager();
	}
	public GeneratorManager getManager() {
		return manager;
	}
	/**
	 * Create the input specification page.
	 */
	void createInputsForm() {
		try {
			inputsForm = new InputsForm(this,"org.opendds.modeling.sdk.forms.input","Model Input");
			addPage(INPUTFORM_INDEX,inputsForm);
		} catch (PartInitException e) {
			ErrorDialog.openError(
					getSite().getShell(),
					"Error creating nested input specification form",
					e.getMessage(),
					e.getStatus());
		}
	}
	/**
	 * Create the target output specification page.
	 */
	void createOutputsForm() {
		try {
			outputsForm = new OutputsForm(this,"org.opendds.modeling.sdk.forms.output","Model Output");
			addPage(OUTPUTFORM_INDEX,outputsForm);
		} catch (PartInitException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	/**
	 * Create the instance definition and customization page.
	 */
	void createInstanceForm() {
		try {
			instanceForm = new InstanceForm(this,"org.opendds.modeling.sdk.forms.instance","Instance Customization");
			addPage(INSTANCEFORM_INDEX,instanceForm);
		} catch (PartInitException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	/**
	 * Creates the page with the XML text editor showing the contents of the resource.
	 */
	void createXmlEditor() {
		try {
			xmlEditor = new TextEditor();
			addPage(XMLEDITOR_INDEX, xmlEditor, getEditorInput());
			setPageText(XMLEDITOR_INDEX, xmlEditor.getTitle());
		} catch (PartInitException e) {
			ErrorDialog.openError(
				getSite().getShell(),
				"Error creating nested XML text editor",
				e.getMessage(),
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

	protected void initializeProvider() {
		provider = DocumentProviderRegistry
		             .getDefault()
		             .getDocumentProvider(getEditorInput());
		if( provider == null) {
			System.out.println("Failed to get a provider");
			ErrorDialog.openError(
					getSite().getShell(),
					"Failed to obtain the document provider.",
					null,
					null);
			return;
		}
	}
	
	protected void reloadManager() {
		try {
			InputStream in = new ByteArrayInputStream(
					               provider
					                 .getDocument(getEditorInput())
					                 .get()
					                 .getBytes()
					             );
			manager.unmarshal(in);
//			manager.marshal(System.out);
		} catch (JAXBException e) {
			ErrorDialog.openError(
					getSite().getShell(),
					"Error parsing XML specification",
					e.getMessage(),
					null);
		}		
	}

	@Override
	protected void addPages() {
		createInputsForm();
		createOutputsForm();
		createInstanceForm();
		createXmlEditor();
		updateTitle();
		initializeProvider();	// After XML editor is created for now to ensure provider has been registered.
		reloadManager();		// Must be *after* provider is initialized.
	}

	public void dispose() {
		ResourcesPlugin.getWorkspace().removeResourceChangeListener(this);
		super.dispose();
	}

	@Override
	public void doSave(IProgressMonitor monitor) {
		getEditor(XMLEDITOR_INDEX).doSave(monitor);

	}

	@Override
	public void doSaveAs() {
		IEditorPart editor = getEditor(XMLEDITOR_INDEX);
		editor.doSaveAs();
		setPageText(XMLEDITOR_INDEX, editor.getTitle());
		setInput(editor.getEditorInput());

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
