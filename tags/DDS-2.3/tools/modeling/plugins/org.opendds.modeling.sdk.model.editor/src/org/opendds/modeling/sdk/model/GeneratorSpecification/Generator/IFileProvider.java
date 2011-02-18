
package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.net.MalformedURLException;
import java.net.URL;

public interface IFileProvider {

	public abstract URL fromWorkspace(String fileName)
	throws MalformedURLException;

	public abstract URL fromWorkspace(String fileName, boolean directory)
			throws MalformedURLException;

	public abstract URL fromBundle(String fileName)
			throws MalformedURLException;

	public abstract void refresh(String targetFolder);

}