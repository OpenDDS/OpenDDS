package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.notify.Adapter;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.InternalEObject;
import org.eclipse.emf.ecore.impl.ENotificationImpl;
import org.eclipse.jface.viewers.ContentViewer;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.FocusAdapter;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.KeyAdapter;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.part.EditorPart;
import org.opendds.modeling.sdk.model.GeneratorSpecification.GeneratorPackage;
import org.opendds.modeling.sdk.model.GeneratorSpecification.ModelFile;
import org.opendds.modeling.sdk.model.GeneratorSpecification.TargetDir;

public class GeneratorTab extends ContentViewer {
	protected Composite control;
	
	protected Label idlLabel;
	protected Label cppLabel;
	protected Label hLabel;
	protected Label mpcLabel;
	
	protected Text sourceText;
	protected Text targetDir;

	protected ModelFile source;
	protected TargetDir target;

	protected GeneratorEditor changeListener;

	public GeneratorTab( final Composite parent) {
		control = new Composite( parent, 0);
		control.setLayout( new GridLayout( 2, false));

		// Left Panel
		Composite panel = new Composite(control, 0);
		GridData gridData = new GridData(SWT.FILL, SWT.FILL, true, true);
		panel.setLayoutData(gridData);
		
		panel.setLayout( new GridLayout( 3, false));

		Label label = new Label(panel, SWT.LEFT);
		label.setText("Model File: ");
		gridData = new GridData(SWT.RIGHT, SWT.CENTER, false, false);
		label.setLayoutData(gridData);

		sourceText = new Text(panel, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		gridData = new GridData(SWT.FILL, SWT.CENTER, true, false);
		sourceText.setLayoutData(gridData);
		// Maybe have the modify listener update the Text field and the
		// focus and key listeners update the ParsedModelFile
//		sourceText.addModifyListener( new ModifyListener() {
//			@Override
//			public void modifyText(ModifyEvent e) {
//				source.setName( sourceText.getText());
//			}
//		});
        sourceText.addFocusListener( new FocusAdapter() {
            @Override
            public void focusLost(FocusEvent event) {
              source.setName( sourceText.getText());
              changeListener.firePropertyChange(EditorPart.PROP_DIRTY);
            }
        });
        sourceText.addKeyListener( new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent event) {
              if (event.character == '\r' || event.character == '\n') {
                source.setName( sourceText.getText());
                changeListener.firePropertyChange(EditorPart.PROP_DIRTY);
//            	String oldValue = source.getName();
//                source.eNotify(
//                		new ENotificationImpl(
//                				(InternalEObject) getInput(),
//                				Notification.SET,
//                				GeneratorPackage.GENSPEC__SOURCE,
//                				oldValue,
//                				source.getName()
//                		)
//                );
              }
            }
        });
		
		Button button = new Button(panel, SWT.PUSH | SWT.LEFT);
		button.setText("Browse...");
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected( SelectionEvent e) {
				IPath modelPath = Utils.browseForModelFile( parent, new Path(sourceText.getText()));
				if( modelPath != null) {
					sourceText.setText( modelPath.toString());
	                source.setName( sourceText.getText());
				}
			}
		});
		
		label = new Label(panel, SWT.LEFT);
		label.setText("Target Folder: ");
		gridData = new GridData(SWT.RIGHT, SWT.CENTER, false, false);
		label.setLayoutData(gridData);

		targetDir = new Text(panel, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		gridData = new GridData(SWT.FILL, SWT.CENTER, true, false);
		targetDir.setLayoutData(gridData);
//		targetDir.addModifyListener( new ModifyListener() {
//			@Override
//			public void modifyText(ModifyEvent e) {
//				target.setName( targetDir.getText());
//			}
//		});
        targetDir.addFocusListener( new FocusAdapter() {
            @Override
            public void focusLost(FocusEvent event) {
            	target.setName( targetDir.getText());
            }
        });
        targetDir.addKeyListener( new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent event) {
              if (event.character == '\r' || event.character == '\n') {
                target.setName( targetDir.getText());
              }
            }
        });
		
		button = new Button(panel, SWT.PUSH | SWT.LEFT);
		button.setText("Browse...");
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected( SelectionEvent e) {
				Object[] current = new Object[] {targetDir.getText()};
				IPath targetPath = Utils.browseForTargetDir( parent, current);
				if( targetPath != null) {
					targetDir.setText(targetPath.toString());
	            	target.setName( targetDir.getText());
				}
			}
		});
		
		// Right Panel
		panel = new Composite(control, 0);
		gridData = new GridData(SWT.FILL, SWT.FILL, true, true);
		panel.setLayoutData(gridData);
		panel.setLayout( new GridLayout( 2, false));

		button = new Button(panel, SWT.PUSH);
		button.setText("Generate IDL");
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
//				generator.generate(CodeGenerator.TransformType.IDL, getSourceName(), getTargetName());
			}
		});
		idlLabel =  new Label(panel, SWT.LEFT);
		idlLabel.setText("<model>.idl");
		gridData = new GridData(SWT.CENTER, SWT.TOP, false, false);
		idlLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText("Generate C++ Header");
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
//				generator.generate(CodeGenerator.TransformType.H, getSourceName(), getTargetName());
			}
		});
		hLabel =  new Label(panel, SWT.LEFT);
		hLabel.setText("<model>.h");
		gridData = new GridData(SWT.CENTER, SWT.TOP, false, false);
		hLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText("Generate C++ Body");
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
//				generator.generate(CodeGenerator.TransformType.CPP, getSourceName(), getTargetName());
			}
		});
		cppLabel =  new Label(panel, SWT.LEFT);
		cppLabel.setText("<model>.cpp");
		gridData = new GridData(SWT.CENTER, SWT.TOP, false, false);
		cppLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText("Generate MPC File");
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
//				generator.generate(CodeGenerator.TransformType.MPC, getSourceName(), getTargetName());
			}
		});
		mpcLabel =  new Label(panel, SWT.LEFT);
		mpcLabel.setText("<model>.mpc");
		gridData = new GridData(SWT.CENTER, SWT.TOP, false, false);
		mpcLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText("Generate All");
		gridData = new GridData(SWT.LEFT, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
//				generator.generate(CodeGenerator.TransformType.IDL, getSourceName(), getTargetName());
//				generator.generate(CodeGenerator.TransformType.H,   getSourceName(), getTargetName());
//				generator.generate(CodeGenerator.TransformType.CPP, getSourceName(), getTargetName());
//				generator.generate(CodeGenerator.TransformType.MPC, getSourceName(), getTargetName());
			}
		});
	}

	public void setSource(ModelFile source) {
		this.source = source;
		if( sourceText != null && source != null) {
			sourceText.setText(source.getName());
		}
	}

	public void setTarget(TargetDir target) {
		this.target = target;
		if( targetDir != null && target != null) {
			targetDir.setText(target.getName());
		}
	}

	@Override
	public Control getControl() {
		return control;
	}

	@Override
	public void refresh() {
		if( sourceText != null && source != null) {
			sourceText.setText(source.getName());
		}
		if( targetDir != null && target != null) {
			targetDir.setText(target.getName());
		}
	}

	@Override
	public ISelection getSelection() {
		return StructuredSelection.EMPTY;
	}

	@Override
	public void setSelection(ISelection selection, boolean reveal) {
		// TODO Auto-generated method stub

	}

	public void addChangeListener(GeneratorEditor generatorEditor) {
		changeListener = generatorEditor;
	}

}
