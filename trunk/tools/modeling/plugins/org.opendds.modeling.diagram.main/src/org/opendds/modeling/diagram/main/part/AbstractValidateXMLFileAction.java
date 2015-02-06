package org.opendds.modeling.diagram.main.part;

import java.io.IOException;
import java.io.InputStream;
import java.io.Reader;
import java.net.URL;
import java.util.List;

import javax.xml.transform.Source;
import javax.xml.transform.stream.StreamSource;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.console.ConsolePlugin;
import org.eclipse.ui.console.IConsole;
import org.eclipse.ui.console.IConsoleConstants;
import org.eclipse.ui.console.IConsoleManager;
import org.eclipse.ui.console.IConsoleView;
import org.eclipse.ui.console.MessageConsole;
import org.eclipse.ui.console.MessageConsoleStream;
import org.osgi.framework.Bundle;
import org.w3c.dom.ls.LSInput;
import org.w3c.dom.ls.LSResourceResolver;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;

import com.ociweb.xml.util.XMLUtil;

/**
 * @generated NOT
 */
public abstract class AbstractValidateXMLFileAction implements IObjectActionDelegate {
	private static final String VALIDATION_PLUGINNAME = "org.opendds.modeling.validation";
	private static final String CONSOLE_NAME = "XML Validation";
	private static final String DIALOG_WINDOW_NAME = "OpenDDS XML Validation";
	
	private IWorkbenchPart targetPart;
	private IFile selectedFile;

	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart) {
		this.targetPart = targetPart;
	}
	
	abstract String getSchemaFilename();

	@Override
	public void run(IAction action) {
		try {
			Path p = new Path(XMLUtil.XSD_DIR + "/" + getSchemaFilename());
			Bundle bundle = Platform.getBundle(VALIDATION_PLUGINNAME);
			URL xsdURL = FileLocator.find(bundle, p, null);
			Source xml = new StreamSource(selectedFile.getContents());
			Source xsd = new StreamSource(xsdURL.openStream());
			List<SAXParseException> errors = XMLUtil.validate(xsd, xml, new ValidationLSResourceResolver());
			if (!errors.isEmpty()) {
				MessageConsole myConsole = findConsole(CONSOLE_NAME);
				MessageConsoleStream out = myConsole.newMessageStream();
				MessageDialog.openError(getShell(), DIALOG_WINDOW_NAME, errors.size() +
						" errors were encountered. See the console view for details");
				for (SAXParseException se : errors) {
					String msg = "Line " + se.getLineNumber() + " Column " + se.getColumnNumber() + ": " + se.getMessage();
					out.println(msg);
				}
				OpenDDSDiagramEditorPlugin.getInstance().logError("OpenDDS XML Validation failed for " + selectedFile.getName());
				IWorkbenchPage page = targetPart.getSite().getPage();
				String id = IConsoleConstants.ID_CONSOLE_VIEW;
				try {
					IConsoleView view = (IConsoleView) page.showView(id);
					view.display(myConsole);
				} catch (PartInitException e) {
					OpenDDSDiagramEditorPlugin.getInstance().logError("Opening " + CONSOLE_NAME + 
							" console for validation results failed",e);
				}
			} else {
				MessageDialog.openInformation(getShell(), "OpenDDS XML Validation", 
						"Validation completed with no XML Schema violations detected");
			}
		} catch (CoreException e) {
			MessageDialog.openError(getShell(), DIALOG_WINDOW_NAME, e.getMessage());
			OpenDDSDiagramEditorPlugin.getInstance().logError("OpenDDS XML Validation failed",e);
		} catch (SAXException e) {
			MessageDialog.openError(getShell(), DIALOG_WINDOW_NAME, e.getMessage());
			OpenDDSDiagramEditorPlugin.getInstance().logError("OpenDDS XML Validation failed",e);
		} catch (IOException e) {
			MessageDialog.openError(getShell(), DIALOG_WINDOW_NAME, e.getMessage());
			OpenDDSDiagramEditorPlugin.getInstance().logError("OpenDDS XML Validation failed",e);
		}
	}

	private Shell getShell() {
		return targetPart.getSite().getShell();
	}

	@Override
	public void selectionChanged(IAction action, ISelection selection) {
		this.selectedFile = null;
		action.setEnabled(false);
		if (selection instanceof IStructuredSelection == false
				|| selection.isEmpty()) {
			return;
		}
		this.selectedFile = (IFile) ((IStructuredSelection) selection)
				.getFirstElement();
		action.setEnabled(true);
	}
	
	private MessageConsole findConsole(String name) {
		ConsolePlugin plugin = ConsolePlugin.getDefault();
		IConsoleManager conMan = plugin.getConsoleManager();
		IConsole[] existing = conMan.getConsoles();
		for (int i = 0; i < existing.length; i++)
			if (name.equals(existing[i].getName()))
				return (MessageConsole) existing[i];
		// no console found, so create a new one
		MessageConsole myConsole = new MessageConsole(name, null);
		conMan.addConsoles(new IConsole[] { myConsole });
		return myConsole;
	}
			
	static final class ValidationLSResourceResolver implements LSResourceResolver {
		@Override
		public LSInput resolveResource(String type, String namespaceURI,
				String publicId, String systemId, String baseURI) {
			ValidationLSInput lsInput = new ValidationLSInput();
			lsInput.setPublicId(publicId);
			lsInput.setSystemId(systemId);
			lsInput.setBaseURI(baseURI);
			return lsInput;
		}
	}
	
	static final class ValidationLSInput implements LSInput {
		private String baseURI;
		private InputStream byteStream;
		private boolean certifiedText;
		private Reader characterStream;
		private String encoding;
		private String publicId;
		private String stringData;
		private String systemId;

		@Override
		public InputStream getByteStream() {
			if (!this.systemId.endsWith(".xsd")) {
				return null;
			}
			Path p = new Path(XMLUtil.XSD_DIR + "/" + this.systemId);
			Bundle bundle = Platform.getBundle(VALIDATION_PLUGINNAME);
			URL xsdURL = FileLocator.find(bundle, p, null);
			try {
				return xsdURL.openStream();
			} catch (IOException e) {
				return null;
			}
		}

		@Override
		public String getBaseURI() { return baseURI; }
		@Override
		public boolean getCertifiedText() { return certifiedText; }
		@Override
		public Reader getCharacterStream() { return characterStream; }
		@Override
		public String getEncoding() { return encoding; }
		@Override
		public String getPublicId() { return publicId; }
		@Override
		public String getStringData() { return stringData; }
		@Override
		public String getSystemId() { return systemId; }
		@Override
		public void setBaseURI(String baseURI) { this.baseURI = baseURI; }
		@Override
		public void setByteStream(InputStream byteStream) { this.byteStream = byteStream; }
		@Override
		public void setCertifiedText(boolean certifiedText) { this.certifiedText = certifiedText; }
		@Override
		public void setCharacterStream(Reader characterStream) { this.characterStream = characterStream; }
		@Override
		public void setEncoding(String encoding) { this.encoding = encoding; }
		@Override
		public void setPublicId(String publicId) { this.publicId = publicId; }
		@Override
		public void setStringData(String stringData) { this.stringData = stringData; }
		@Override
		public void setSystemId(String systemId) { this.systemId = systemId; }
	}
}


