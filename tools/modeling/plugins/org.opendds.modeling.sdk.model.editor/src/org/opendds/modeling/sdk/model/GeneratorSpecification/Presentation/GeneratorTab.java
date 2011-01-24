package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import java.util.List;

import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.edit.command.SetCommand;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.FocusAdapter;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.KeyAdapter;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.Widget;
import org.opendds.modeling.sdk.model.GeneratorSpecification.GeneratorPackage;
import org.opendds.modeling.sdk.model.GeneratorSpecification.ModelFile;
import org.opendds.modeling.sdk.model.GeneratorSpecification.TargetDir;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGeneratorFactory;

public class GeneratorTab extends StructuredViewer {
	protected Composite control;
	
	protected Label idlLabel;
	protected Label hLabel;
	protected Label cppLabel;
	protected Label trhLabel;
	protected Label trcLabel;
	protected Label mpcLabel;
	protected Label mpbLabel;
	
	protected Text sourceText;
	protected Text targetDir;

	protected ModelFile source;
	protected TargetDir target;

	protected SdkGenerator generator;

	protected GeneratorEditor editor;
	
	protected ISelection selection = StructuredSelection.EMPTY;

	public GeneratorTab( final Composite parent) {
		generator = SdkGeneratorFactory.createSdkGenerator(parent.getShell());
		
		control = new Composite( parent, 0);
		control.setLayout( new GridLayout( 2, false));

		// Left Panel
		Composite panel = new Composite(control, 0);
		GridData gridData = new GridData(SWT.FILL, SWT.FILL, true, true);
		panel.setLayoutData(gridData);
		
		panel.setLayout( new GridLayout( 3, false));

		// Model file selection
		Label label = new Label(panel, SWT.LEFT);
		label.setText("Model File: ");
		gridData = new GridData(SWT.RIGHT, SWT.CENTER, false, false);
		label.setLayoutData(gridData);

		sourceText = new Text(panel, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		gridData = new GridData(SWT.FILL, SWT.CENTER, true, false);
		sourceText.setLayoutData(gridData);
        sourceText.addFocusListener( new FocusAdapter() {
            @Override
            public void focusLost(FocusEvent event) {
    			updateSource();
            }
        });
        sourceText.addKeyListener( new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent event) {
              if (event.character == '\r' || event.character == '\n') {
      			updateSource();
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
        			updateSource();
        		}
        	}
        });

        // Target folder selection.
		label = new Label(panel, SWT.LEFT);
		label.setText("Target Folder: ");
		gridData = new GridData(SWT.RIGHT, SWT.CENTER, false, false);
		label.setLayoutData(gridData);

		targetDir = new Text(panel, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		gridData = new GridData(SWT.FILL, SWT.CENTER, true, false);
		targetDir.setLayoutData(gridData);
        targetDir.addFocusListener( new FocusAdapter() {
            @Override
            public void focusLost(FocusEvent event) {
          	  updateTarget();
            }
        });
        targetDir.addKeyListener( new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent event) {
              if (event.character == '\r' || event.character == '\n') {
            	  updateTarget();
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
	            	  updateTarget();
				}
			}
		});
		
		// Right Panel
		panel = new Composite(control, 0);
		gridData = new GridData(SWT.FILL, SWT.FILL, false, true);
		panel.setLayoutData(gridData);
		panel.setLayout( new GridLayout( 2, false));
		
		// Subsequent resizing can become incorrect if there is no initial text size.
		final String uninitializedLabel = new String("unitialized");

		// Code generation pushbuttons
		button = new Button(panel, SWT.PUSH);
		button.setText(SdkGenerator.TransformType.IDL.dialogTitle());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(SdkGenerator.TransformType.IDL, source.getName());
			}
		});
		idlLabel =  new Label(panel, SWT.LEFT);
		idlLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		idlLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkGenerator.TransformType.H.dialogTitle());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(SdkGenerator.TransformType.H, source.getName());
			}
		});
		hLabel =  new Label(panel, SWT.LEFT);
		hLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		hLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkGenerator.TransformType.CPP.dialogTitle());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(SdkGenerator.TransformType.CPP, source.getName());
			}
		});
		cppLabel =  new Label(panel, SWT.LEFT);
		cppLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		cppLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkGenerator.TransformType.TRH.dialogTitle());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(SdkGenerator.TransformType.TRH, source.getName());
			}
		});
		trhLabel =  new Label(panel, SWT.LEFT);
		trhLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		trhLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkGenerator.TransformType.TRC.dialogTitle());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(SdkGenerator.TransformType.TRC, source.getName());
			}
		});
		trcLabel =  new Label(panel, SWT.LEFT);
		trcLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		trcLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkGenerator.TransformType.MPC.dialogTitle());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(SdkGenerator.TransformType.MPC, source.getName());
			}
		});
		mpcLabel =  new Label(panel, SWT.LEFT);
		mpcLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		mpcLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkGenerator.TransformType.MPB.dialogTitle());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(SdkGenerator.TransformType.MPB, source.getName());
			}
		});
		mpbLabel =  new Label(panel, SWT.LEFT);
		mpbLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		mpbLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText("Generate All");
		gridData = new GridData(SWT.LEFT, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(SdkGenerator.TransformType.IDL, source.getName());
				generator.generate(SdkGenerator.TransformType.H,   source.getName());
				generator.generate(SdkGenerator.TransformType.CPP, source.getName());
				generator.generate(SdkGenerator.TransformType.TRH, source.getName());
				generator.generate(SdkGenerator.TransformType.TRC, source.getName());
				generator.generate(SdkGenerator.TransformType.MPC, source.getName());
				generator.generate(SdkGenerator.TransformType.MPB, source.getName());
			}
		});
	}

	protected void updateSource() {
		if( !sourceText.getText().equals(source.getName())) {
			editor.getEditingDomain().getCommandStack().execute(
					SetCommand.create(
							editor.getEditingDomain(),
							source,
							GeneratorPackage.eINSTANCE.getModelFile_Name(),
							sourceText.getText()
					));
			updateModelnameLabels();
		}
	}
	
	protected void updateTarget() {
		if( !targetDir.getText().equals(target.getName())) {
			editor.getEditingDomain().getCommandStack().execute(
					SetCommand.create(
							editor.getEditingDomain(),
							target,
							GeneratorPackage.eINSTANCE.getTargetDir_Name(),
							targetDir.getText()
					));
		}
	}
	
	protected void updateModelnameLabels() {
		String basename = null;// getModelName();
		if( basename == null) {
			final String invalidLabel = new String("Invalid model file");

			idlLabel.setText(invalidLabel);

			// The size should be the same for all labels with the same message.
			Point invalidSize = idlLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true);
			idlLabel.setSize(invalidSize);
			
			hLabel.setText(invalidLabel);
			hLabel.setSize(invalidSize);
			
			cppLabel.setText(invalidLabel);
			cppLabel.setSize(invalidSize);
			
			trhLabel.setText(invalidLabel);
			trhLabel.setSize(invalidSize);
			
			trcLabel.setText(invalidLabel);
			trcLabel.setSize(invalidSize);
			
			mpcLabel.setText(invalidLabel);
			mpcLabel.setSize(invalidSize);
			
			mpbLabel.setText(invalidLabel);
			mpbLabel.setSize(invalidSize);

		} else {
			idlLabel.setText(basename + SdkGenerator.TransformType.IDL.suffix());
			idlLabel.setSize(idlLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			hLabel.setText(basename + SdkGenerator.TransformType.H.suffix());
			hLabel.setSize(hLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			cppLabel.setText(basename + SdkGenerator.TransformType.CPP.suffix());
			cppLabel.setSize(cppLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			trhLabel.setText(basename + SdkGenerator.TransformType.TRH.suffix());
			trhLabel.setSize(trhLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			trcLabel.setText(basename + SdkGenerator.TransformType.TRC.suffix());
			trcLabel.setSize(trcLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			mpcLabel.setText(basename + SdkGenerator.TransformType.MPC.suffix());
			mpcLabel.setSize(mpcLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			mpbLabel.setText(basename + SdkGenerator.TransformType.MPB.suffix());
			mpbLabel.setSize(mpbLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
		}
		
		control.layout(true);
	}

	public void setSource(ModelFile source) {
		this.source = source;
		if( sourceText != null && source != null) {
  			sourceText.setText(source.getName());
			updateModelnameLabels();
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
		super.refresh();
	}

	@Override
	public ISelection getSelection() {
		return selection;
	}

	@Override
	public void setSelection(ISelection selection, boolean reveal) {
		this.selection = selection;
	}

	public void setGeneratorEditor(GeneratorEditor generatorEditor) {
		editor = generatorEditor;
	}

	@Override
	protected Widget doFindInputItem(Object element) {
		if( element == source) {
			return sourceText;
		} else if( element == target) {
			return targetDir;
		}
		return null;
	}

	@Override
	protected Widget doFindItem(Object element) {
		return doFindInputItem( element);
	}

	@Override
	protected void doUpdateItem(Widget item, Object element, boolean fullMap) {
		if( !fullMap) {
			return;
			
		} else if( element == source) {
			if( item == sourceText) {
				sourceText.setText( source.getName());
			}
		} else if( element == target) {
			if( item == targetDir) {
				targetDir.setText( target.getName());
			}
		}
	}

	@SuppressWarnings("unchecked")
	@Override
	protected List getSelectionFromWidget() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	protected void internalRefresh(Object element) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void reveal(Object element) {
		// TODO Auto-generated method stub
		
	}

	@SuppressWarnings("unchecked")
	@Override
	protected void setSelectionToWidget(List l, boolean reveal) {
		// TODO Auto-generated method stub
		
	}

}
