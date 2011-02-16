package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import java.util.List;

import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.edit.domain.EditingDomain;
import org.eclipse.jface.action.IStatusLineManager;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
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
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.actions.WorkspaceModifyOperation;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGeneratorFactory;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkTransformer;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkTransformer.TransformType;

public class GeneratorTab extends StructuredViewer {
	protected Composite control;
	
	protected Label idlLabel;
	protected Label hLabel;
	protected Label cppLabel;
	protected Label trhLabel;
	protected Label trcLabel;
	protected Label mpcLabel;
	protected Label mpbLabel;
	protected Label pathMpbLabel;
	
	protected Text sourceText;
	protected Text targetDir;

	protected SdkGenerator generator;

	protected GeneratorEditor editor;
	
	protected ISelection selection = StructuredSelection.EMPTY;

	public GeneratorTab(final Composite parent, GeneratorEditor generatorEditor) {
		generator = SdkGeneratorFactory.createSdkGenerator(parent.getShell(), generatorEditor);
		editor = generatorEditor;

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
		final String uninitializedLabel = new String("uninitialized");

		// Code generation pushbuttons
		button = new Button(panel, SWT.PUSH);
		button.setText(SdkTransformer.TransformType.IDL.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.IDL));
		idlLabel =  new Label(panel, SWT.LEFT);
		idlLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		idlLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkTransformer.TransformType.H.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.H));
		hLabel =  new Label(panel, SWT.LEFT);
		hLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		hLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkTransformer.TransformType.CPP.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.CPP));
		cppLabel =  new Label(panel, SWT.LEFT);
		cppLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		cppLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkTransformer.TransformType.TRH.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.TRH));
		trhLabel =  new Label(panel, SWT.LEFT);
		trhLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		trhLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkTransformer.TransformType.TRC.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.TRC));
		trcLabel =  new Label(panel, SWT.LEFT);
		trcLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		trcLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkTransformer.TransformType.MPC.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.MPC));
		mpcLabel =  new Label(panel, SWT.LEFT);
		mpcLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		mpcLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkTransformer.TransformType.MPB.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.MPB));
		mpbLabel =  new Label(panel, SWT.LEFT);
		mpbLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		mpbLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText(SdkTransformer.TransformType.PATH_MPB.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.PATH_MPB));
		pathMpbLabel =  new Label(panel, SWT.LEFT);
		pathMpbLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		pathMpbLabel.setLayoutData(gridData);

		button = new Button(panel, SWT.PUSH);
		button.setText("Generate All");
		gridData = new GridData(SWT.LEFT, SWT.TOP, false, false);
		button.setLayoutData(gridData);
		button.addSelectionListener(new GenerateButtonListener(null));
	}

	private final class GenerateButtonListener extends SelectionAdapter {
		private TransformType whichTransform;

		/// @param which transform output, or null to generate all
		public GenerateButtonListener(TransformType which) {
			whichTransform = which;
		}

		public void widgetSelected(SelectionEvent e) {
			if (!editor.getSite().getPage().saveEditor(editor, true /*confirm*/)) {
				// user canceled
				return;
			}
			
			IStatusLineManager statusLineManager = editor.getActionBars().getStatusLineManager();

			// This is the progress monitor on the status line.
//			IProgressMonitor progressMonitor
//			  = statusLineManager != null ?
//					  statusLineManager.getProgressMonitor(): new NullProgressMonitor();

			// Do the work within an operation because this is a long running activity that modifies the workbench.
			//
			WorkspaceModifyOperation operation = new WorkspaceModifyOperation() {
				// This is the method that gets invoked when the operation runs.
				//
				@Override
				public void execute(IProgressMonitor monitor) {
					if (whichTransform == null) {
						generator.generateAll();
					} else {
						generator.generate(whichTransform);
					}
				}
			};

			try {
				// This runs the options, and shows progress.
				//
				new ProgressMonitorDialog(control.getShell()).run(true, false,
						operation);
				editor.firePropertyChange(IEditorPart.PROP_DIRTY); // Maybe this will cause a refresh?
				if( statusLineManager != null) {
					statusLineManager.setMessage("Code Generation Complete");
				}

			} catch (Exception exception) {
				// Something went wrong that shouldn't.
				//
				GeneratorEditorPlugin.INSTANCE.log(exception);
				if( statusLineManager != null) {
					statusLineManager.setErrorMessage("Code Generation: " + exception.getMessage());
				}
			}
		}
	}

	protected void updateSource() {
		generator.setModelFileName( sourceText.getText());
		updateModelnameLabels();
	}
	
	protected void updateTarget() {
		generator.setTargetDirName( targetDir.getText());
	}
	
	protected void updateModelnameLabels() {
		String basename = generator.getModelName();
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
			
			pathMpbLabel.setText(invalidLabel);
			pathMpbLabel.setSize(invalidSize);

		} else {
			idlLabel.setText(basename + SdkTransformer.TransformType.IDL.getSuffix());
			idlLabel.setSize(idlLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			hLabel.setText(basename + SdkTransformer.TransformType.H.getSuffix());
			hLabel.setSize(hLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			cppLabel.setText(basename + SdkTransformer.TransformType.CPP.getSuffix());
			cppLabel.setSize(cppLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			trhLabel.setText(basename + SdkTransformer.TransformType.TRH.getSuffix());
			trhLabel.setSize(trhLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			trcLabel.setText(basename + SdkTransformer.TransformType.TRC.getSuffix());
			trcLabel.setSize(trcLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			mpcLabel.setText(basename + SdkTransformer.TransformType.MPC.getSuffix());
			mpcLabel.setSize(mpcLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			mpbLabel.setText(basename + SdkTransformer.TransformType.MPB.getSuffix());
			mpbLabel.setSize(mpbLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
			
			pathMpbLabel.setText(basename + SdkTransformer.TransformType.PATH_MPB.getSuffix());
			pathMpbLabel.setSize(pathMpbLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
		}
		
		control.layout(true);
	}

	@Override
	public Control getControl() {
		return control;
	}

	@Override
	public void refresh() {
		if( sourceText != null && generator != null) {
			String name = generator.getModelFileName();
			if( name != null) {
				sourceText.setText( name);
			}
		}
		if( targetDir != null && generator != null) {
			String name = generator.getTargetDirName();
			if( name != null) {
				targetDir.setText( name);
			}
		}
		updateModelnameLabels();
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

	public void setEditingDomain(EditingDomain editingDomain) {
		generator.setEditingDomain( editingDomain);
		refresh();
	}

	@Override
	protected Widget doFindInputItem(Object element) {
		if( generator.isModelSource(element)) {
			return sourceText;
		} else if( generator.isModelTarget(element)) {
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
			
		} else if( generator.isModelSource(element)) {
			if( item == sourceText) {
				sourceText.setText( generator.getModelFileName());
			}
			
		} else if( generator.isModelTarget(element)) {
			if( item == targetDir) {
				targetDir.setText( generator.getTargetDirName());
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
