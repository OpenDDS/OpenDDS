package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.net.MalformedURLException;
import java.net.URL;
import java.net.URI;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IFileEditorInput;

public class SdkGeneratorFactory {
	private static final String PLUGINNAME = "org.opendds.modeling.sdk.model";
	
	public static SdkGenerator createSdkGenerator(Shell parent, IEditorPart editor) {
		IFileProvider fileProvider = createFileProvider(editor);
		IErrorHandler errorHandler = createErrorHandler(parent);
		
		return createSdkGenerator( fileProvider, errorHandler);
	}
	
	public static SdkGenerator createSdkGenerator(IFileProvider provider, IErrorHandler handler) {
		SdkGenerator generator = SdkGenerator.create(provider, handler);
		generator.setGeneratorModel(new EmfGeneratorModel());
		return generator;
	}
	
	public static ParsedModelFile createParsedModelFile(Shell parent, IEditorPart editor) {
		IFileProvider fileProvider = createFileProvider(editor);
		IErrorHandler errorHandler = createErrorHandler(parent);
		
		return ParsedModelFile.create( fileProvider, errorHandler);
	}
		
	public static IFileProvider createFileProvider(final IEditorPart editor) {
		return new IFileProvider() {
			@Override
			public URL fromWorkspace(String fileName) throws MalformedURLException {
				return fromWorkspace(fileName, false);
			}
			@Override
			public URL fromWorkspace(String fileName, boolean directory) throws MalformedURLException {
				URI uri = getResource(fileName, directory).getLocationURI();
				if (uri != null) {
					return uri.toURL();
				} else {
					return null;
				}
			}
			private IResource getResource(String file, boolean directory) {
				IPath path = new Path(file);
				if (path.isAbsolute() || editor == null) {
					IWorkspaceRoot workspace = ResourcesPlugin.getWorkspace().getRoot();
					return directory ? workspace.getFolder(path) : workspace.getFile(path);
				} else {
					IEditorInput input = editor.getEditorInput();
					IFileEditorInput fei = (IFileEditorInput) input;
					IFile activeFile = fei.getFile();
					IContainer parent = activeFile.getParent();
					return directory ? parent.getFolder(path) : parent.getFile(path);
				}
			}
			@Override
			public URL fromBundle(String fileName) {
				return FileLocator.find(Platform.getBundle(PLUGINNAME), new Path(fileName), null);
			}
			@Override
			public void refresh(String targetFolder) {
				try {
					getResource(targetFolder, true).refreshLocal(IFile.DEPTH_ONE, null);
				} catch (CoreException e) {
					throw new RuntimeException(e);
				}
			}
		};
	}
	
	public static IErrorHandler createErrorHandler( final Shell shell) {
		return new IErrorHandler() {
			@Override
			public void error(Severity sev, String title, String message,
					Throwable exception) {
				int stat_sev;
				switch (sev) {
					case ERROR: stat_sev = IStatus.ERROR; break;
					case WARNING: stat_sev = IStatus.WARNING; break;
					case INFO: stat_sev = IStatus.INFO; break;
					default: stat_sev = IStatus.OK;
				}
				ErrorDialog.openError(shell, title,
					null /* use the message from Status object */,
					new Status(stat_sev, PLUGINNAME, message, exception));
			}
		};
	}

}
