/**
 * GeneratorManager handles moving the code generation
 * specification data from the XML file in which it resides
 * and its in-memory representation.
 */
package org.opendds.modeling.sdk.codegen;

import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import javax.xml.bind.*;

import org.eclipse.core.commands.operations.OperationStatus;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.jface.viewers.IStructuredContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;

/**
 * @author martinezm
 *
 */
public class GeneratorManager  {
	
	private final String packageName = "org.opendds.modeling.sdk.codegen";
	
	private ObjectFactory of;
	private Genspec spec;
	private String modelName;

	private List<IManagerListener> managerListeners;

	public GeneratorManager() {
		of = new ObjectFactory();
		spec = of.createGenspec();
		managerListeners = new ArrayList<IManagerListener>();
	}
	
	public void addManagerListener(IManagerListener listener) {
		if(!managerListeners.contains(listener)) {			
			this.managerListeners.add(listener);
		}
	}
	
	public void removeManagerListener(IManagerListener listener) {
		managerListeners.remove(listener);
	}

	public void fireModelChanged() {
		for( IManagerListener listener: managerListeners) {
			listener.modelChanged(this);
		}		
	}
	
	public void addInstanceSpec( String key, String value) {
		// ... STUB
		fireModelChanged();
	}
	public List<Instancespec> getInstancespec() {
		// ... STUB
		return null;
	}
	
	public void addModelFile( String name, String contents) {
		Modelfile model = of.createModelfile();
		model.setName(name);
		model.setContents(contents);
		spec.getSourceOrTargetsOrSpecs().add(model);
		fireModelChanged();
	}
	public Modelfile getModelFile(String which) {
		for(Object current: spec.getSourceOrTargetsOrSpecs()) {
			if( current instanceof Modelfile) {
				Modelfile modelfile = (Modelfile)current;
				if(modelfile.getName().equalsIgnoreCase(which)) {
					return modelfile;
				}
			}
		}
		return null;			
	}
	public List<Modelfile> getModelfiles() {
		List<Modelfile> list = new ArrayList<Modelfile>();
		for(Object current: spec.getSourceOrTargetsOrSpecs()) {
			if( current instanceof Modelfile) {
				list.add((Modelfile)current);
			}
		}
		return list;
	}
	
	public void addTargetDir( String name) {
		Targetdir target = of.createTargetdir();
		target.setName(name);
		spec.getSourceOrTargetsOrSpecs().add(target);
		fireModelChanged();
	}
	public List<Targetdir> getTargetdirs() {
		List<Targetdir> list = new ArrayList<Targetdir>();
		for(Object current: spec.getSourceOrTargetsOrSpecs()) {
			if( current instanceof Targetdir) {
				list.add((Targetdir)current);
			}
		}
		return list;
	}
	
	public void addControl( boolean h, boolean cpp, boolean idl, boolean mpc) {
		Gencontrol control = of.createGencontrol();
		control.setCreateHfile(h);
		control.setCreateCPPfile(cpp);
		control.setCreateIDLfile(idl);
		control.setCreateMPCfile(mpc);
		spec.getSourceOrTargetsOrSpecs().add(control);
		fireModelChanged();
	}
	public List<Gencontrol> getGencontrols() {
		List<Gencontrol> list = new ArrayList<Gencontrol>();
		for(Object current: spec.getSourceOrTargetsOrSpecs()) {
			if( current instanceof Gencontrol) {
				list.add((Gencontrol)current);
			}
		}
		return list;
	}
	
	public void unmarshal( InputStream inputStream )
		throws JAXBException {
			JAXBContext context = JAXBContext.newInstance( packageName );
			Unmarshaller unmarshaller = context.createUnmarshaller();
			@SuppressWarnings( "unchecked" )
			JAXBElement<Genspec> doc = (JAXBElement<Genspec>)unmarshaller.unmarshal( inputStream );
			spec = doc.getValue();
			fireModelChanged();
	}

	public void marshal( OutputStream outputStream) {
		try {
			JAXBElement<Genspec> element =
				of.createGeneratorspecification( spec);
			JAXBContext context = JAXBContext.newInstance( packageName);
			
			Marshaller marshaller = context.createMarshaller();
			marshaller.setProperty( Marshaller.JAXB_FORMATTED_OUTPUT, Boolean.TRUE );
			marshaller.marshal(element, outputStream);
		} catch( JAXBException jbe) {
			String msg = jbe.toString();
			final IStatus status = new OperationStatus(IStatus.ERROR, "org.opendds.modeling.sdk", 77, msg, jbe.getCause());
            Display.getDefault().asyncExec(new Runnable() {
                public void run() {
        			ErrorDialog.openError(
        					null,
        					"Error writing XML file",
        					null,
        					status);
                }
            });
		}
	}
	
	private class ModelfileProvider implements IStructuredContentProvider {
		private GeneratorManager manager;
		
		ModelfileProvider( GeneratorManager manager) {
			this.manager = manager;
		}

		@Override
		public Object[] getElements(Object inputElement) {
			return (Object[])manager.getModelfiles().toArray();
		}

		@Override
		public void dispose() {
		}

		@Override
		public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
		}
		
	}
	
	IStructuredContentProvider getModelfileProvider() {
		return new ModelfileProvider( this);
	}
	
	private class TargetdirProvider implements IStructuredContentProvider {
		private GeneratorManager manager;
		
		public TargetdirProvider( GeneratorManager manager) {
			this.manager = manager;
		}

		@Override
		public Object[] getElements(Object inputElement) {
			return (Object[])manager.getTargetdirs().toArray();
		}

		@Override
		public void dispose() {
		}

		@Override
		public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
		}
	}
	
	IStructuredContentProvider getTargetdirProvider() {
		return new TargetdirProvider(this);
	}
	
	private class InstanceProvider implements IStructuredContentProvider {
		private GeneratorManager manager;
		
		InstanceProvider( GeneratorManager manager) {
			this.manager = manager;
		}

		@Override
		public Object[] getElements(Object inputElement) {
			return (Object[])manager.getInstancespec().toArray();
		}

		@Override
		public void dispose() {
		}

		@Override
		public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
		}
	}
	
	IStructuredContentProvider getInstanceProvider() {
		return new InstanceProvider(this);
	}
	
	private class ControlProvider implements IStructuredContentProvider {
		private GeneratorManager manager;
		
		ControlProvider( GeneratorManager manager) {
			this.manager = manager;
		}

		@Override
		public Object[] getElements(Object inputElement) {
			return (Object[])manager.getGencontrols().toArray();
		}

		@Override
		public void dispose() {
		}

		@Override
		public void inputChanged(Viewer viewer, Object oldInput, Object newInput) {
		}
	}
	
	IStructuredContentProvider getControlProvider() {
		return new ControlProvider(this);
	}
	
	class CodegenLabelProvider extends LabelProvider implements ITableLabelProvider {

		@Override
		public Image getColumnImage(Object element, int columnIndex) {
			return null;
		}

		@Override
		public String getColumnText(Object element, int columnIndex) {
			final String unsupported = "<UNSUPPORTED>";
			
			if( element instanceof Modelfile) {
				Modelfile file = (Modelfile)element;
				switch(columnIndex) {
				case 0:  return file.getName();
				case 1:  return file.getContents();
				default: return unsupported;
				}
			}

			if( element instanceof Targetdir) {
				Targetdir target = (Targetdir)element;
				switch(columnIndex) {
				case 0:  return target.getName();
				case 1:  return unsupported;
				default: return unsupported;
				}
			}

			if( element instanceof Instancespec) {
				@SuppressWarnings("unused")
				Instancespec instance = (Instancespec)element;
				switch(columnIndex) {
				case 0:
				case 1:
				default: return unsupported;
				}
			}

			if( element instanceof Gencontrol) {
				return unsupported;
			}

			if( element == null){
				return "<null>";
			}
			return element.toString();
		}
		
	}
	
	ITableLabelProvider getLabelProvider() {
		return new CodegenLabelProvider();
	}

	public void setModelName(String modelName) {
		this.modelName = modelName;
	}

	public String getModelName() {
		return modelName;
	}

}
