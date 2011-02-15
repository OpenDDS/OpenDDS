package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import org.eclipse.jface.viewers.ICellModifier;
import org.opendds.modeling.sdk.model.GeneratorSpecification.LocationPath;
import org.opendds.modeling.sdk.model.GeneratorSpecification.LocationVariable;

public class DeploymentCellModifier implements ICellModifier {
	
//	private GeneratorEditor editor;

	public DeploymentCellModifier( GeneratorEditor editor, DeploymentTab viewer) {
//		this.editor = editor;
	}

	@Override
	public boolean canModify(Object element, String property) {
		if( property == GeneratorEditor.VARIABLE_COLUMN_ID) {
			if( element instanceof LocationVariable) {
				return true;
			}
		}
		if( property == GeneratorEditor.PATH_COLUMN_ID) {
			if( element instanceof LocationPath) {
				return true;
			}
		}
		return false;
	}

	@Override
	public Object getValue(Object element, String property) {
		if( property == GeneratorEditor.VARIABLE_COLUMN_ID) {
			if( element instanceof LocationVariable) {
				return ((LocationVariable)element).getValue();
			}
		}
		if( property == GeneratorEditor.PATH_COLUMN_ID) {
			if( element instanceof LocationPath) {
				return ((LocationPath)element).getValue();
			}
		}
		return null;
	}

	@Override
	public void modify(Object element, String property, Object value) {
		if( value == null) {
			return;
		}
		
		String text = ((String)value).trim();
		if( property == GeneratorEditor.VARIABLE_COLUMN_ID) {
			if( element instanceof LocationVariable) {
				((LocationVariable)element).setValue(text);
			}
		}
		if( property == GeneratorEditor.PATH_COLUMN_ID) {
			if( element instanceof LocationPath) {
				((LocationPath)element).setValue(text);
			}
		}
	}

}
