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

import org.eclipse.jface.dialogs.ErrorDialog;

/**
 * @author martinezm
 *
 */
public class GeneratorManager {
	
	private final String packageName = "org.opendds.modeling.sdk.codegen";
	
	private ObjectFactory of;
	private Genspec spec;
	
	public GeneratorManager() {
		of = new ObjectFactory();
		spec = of.createGenspec();
	}
	
	public void addInstanceSpec( String key, String value) {
		// ... STUB
	}
	
	public void addModelFile( String name, String contents) {
		Modelfile model = of.createModelfile();
		model.setName(name);
		model.setContents(contents);
		spec.getSourceOrTargetsOrSpecs().add(model);
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
		try {
			JAXBContext context = JAXBContext.newInstance( packageName );
			Unmarshaller unmarshaller = context.createUnmarshaller();
			@SuppressWarnings( "unchecked" )
			JAXBElement<Genspec> doc = (JAXBElement<Genspec>)unmarshaller.unmarshal( inputStream );
			spec = doc.getValue();
		} catch( JAXBException jbe) {
			ErrorDialog.openError(
					null,
					"Error reading XML file",
					jbe.toString(),
					null);
		}
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
			ErrorDialog.openError(
					null,
					"Error writing XML file",
					jbe.toString(),
					null);
		}
	}

}
