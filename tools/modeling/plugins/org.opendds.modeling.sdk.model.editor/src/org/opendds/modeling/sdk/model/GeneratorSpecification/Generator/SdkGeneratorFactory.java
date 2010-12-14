package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.net.MalformedURLException;
import java.net.URL;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.jface.window.Window;

public class SdkGeneratorFactory {
	private static final String PLUGINNAME = "org.opendds.modeling.sdk.model";
	
	public static SdkGenerator createSdkGenerator( Window parent) {
		SdkGenerator.FileProvider fileProvider = createFileProvider();
		SdkGenerator.ErrorHandler errorHandler = createErrorHandler( parent);
		
		return createSdkGenerator( fileProvider, errorHandler);
	}
	
	public static SdkGenerator createSdkGenerator( SdkGenerator.FileProvider provider, SdkGenerator.ErrorHandler handler) {
		return SdkGenerator.create( provider, handler);
	}
	
	public static ParsedModelFile createParsedModelFile( Window parent) {
		SdkGenerator.FileProvider fileProvider = createFileProvider();
		SdkGenerator.ErrorHandler errorHandler = createErrorHandler( parent);
		
		return ParsedModelFile.create( fileProvider, errorHandler);
	}
	
	public static ParsedModelFile createParsedModelFile( SdkGenerator.FileProvider provider, SdkGenerator.ErrorHandler handler) {
		return ParsedModelFile.create( provider, handler);
	}
	
	public static SdkGenerator.FileProvider createFileProvider() {
		return new SdkGenerator.FileProvider() {
			@Override
			public URL fromWorkspace(String fileName) throws MalformedURLException {
				IWorkspaceRoot workspace = ResourcesPlugin.getWorkspace().getRoot();
				return workspace.getFile(new Path(fileName)).getLocationURI().toURL();
			}
			@Override
			public URL fromBundle(String fileName) {
				return FileLocator.find(Platform.getBundle(PLUGINNAME), new Path(fileName), null);
			}
			@Override
			public void refresh(String targetFolder) {
				IWorkspaceRoot workspace = ResourcesPlugin.getWorkspace().getRoot();
				try {
					workspace.getFolder(new Path(targetFolder)).refreshLocal(IFile.DEPTH_ONE, null);
				} catch (CoreException e) {
					throw new RuntimeException(e);
				}
			}
		};
	}
	
	public static SdkGenerator.ErrorHandler createErrorHandler( final Window parent) {
		return new SdkGenerator.ErrorHandler() {
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
				ErrorDialog.openError(parent.getShell(), title,
					null /* use the message from Status object */,
					new Status(stat_sev, PLUGINNAME, message, exception));
			}
		};
	}

}
