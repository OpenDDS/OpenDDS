
package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

public interface IErrorHandler {
	enum Severity {ERROR, WARNING, INFO}

	public abstract void error(Severity sev, String title, String message,
			Throwable exception);
}