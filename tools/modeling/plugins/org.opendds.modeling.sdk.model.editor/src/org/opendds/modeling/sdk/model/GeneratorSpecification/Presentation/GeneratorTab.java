package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import java.io.File;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.List;

import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.edit.domain.EditingDomain;
import org.eclipse.emf.edit.ui.provider.ExtendedImageRegistry;
import org.eclipse.jface.action.IStatusLineManager;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.FocusAdapter;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.KeyAdapter;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
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
import org.opendds.modeling.common.Plugin;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.IFileProvider;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGeneratorFactory;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkTransformer;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkTransformer.TransformType;

public class GeneratorTab extends StructuredViewer {

	protected final Composite control;

	protected final Label idlLabel;
	protected final Label hLabel;
	protected final Label cppLabel;
	protected final Label trhLabel;
	protected final Label trcLabel;
	protected final Label mpcLabel;
	protected final Label mpbLabel;
	protected final Label pathMpbLabel;

	protected final Button idlBtn;
	protected final Button hBtn;
	protected final Button cppBtn;
	protected final Button trhBtn;
	protected final Button trcBtn;
	protected final Button mpcBtn;
	protected final Button mpbBtn;
	protected final Button pathMpbBtn;
	protected final Button genAllBtn;

	protected final CLabel sourceLabel;
	protected final CLabel targetLabel;

	protected final Text sourceText;
	protected final Text targetDir;

	protected final IFileProvider fileProvider;
	protected final SdkGenerator generator;

	protected final GeneratorEditor editor;

	protected final Object errorimage = Plugin.INSTANCE.getImage("full/obj16/error");
	protected final Object warningimage = Plugin.INSTANCE.getImage("full/obj16/warning");

	protected ISelection selection = StructuredSelection.EMPTY;

	public GeneratorTab(final Composite parent, GeneratorEditor generatorEditor) {

		editor = generatorEditor;
		fileProvider = SdkGeneratorFactory.createFileProvider(generatorEditor);
		generator = SdkGeneratorFactory.createSdkGenerator(fileProvider, SdkGeneratorFactory.createErrorHandler(parent.getShell()));

		control = new Composite(parent, 0);
		control.setLayout(new GridLayout(2, false));

		// Left Panel
		Composite panel = new Composite(control, 0);
		panel.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		panel.setLayout(new GridLayout(3, false));

		// Model file selection
		sourceLabel = new CLabel(panel, SWT.RIGHT);
		sourceLabel.setText("Model File: ");
		GridData gridData = new GridData(SWT.RIGHT, SWT.CENTER, false, false);
		sourceLabel.setLayoutData(gridData);

		sourceText = new Text(panel, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		gridData = new GridData(SWT.FILL, SWT.CENTER, true, false);
		sourceText.setText(getFirstNotNullTrimmed(generator.getModelFileName(), ""));
		sourceText.setLayoutData(gridData);
		sourceText.addFocusListener(new FocusAdapter() {
			@Override
			public void focusLost(FocusEvent event) {
				generator.setModelFileName(getFirstNotNullTrimmed(sourceText.getText(), ""));
				updateSource();
				updateLabels();
				control.layout();
			}
		});

		sourceText.addKeyListener(new KeyAdapter() {

			@Override
			public void keyReleased(KeyEvent e) {
				generator.setModelFileName(getFirstNotNullTrimmed(sourceText.getText(), ""));
				updateSource();
				updateLabels();
				control.layout();
			}
		});

		Button button = new Button(panel, SWT.PUSH | SWT.LEFT);
		button.setText("Browse...");
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {

				IPath modelPath = Utils.browseForModelFile(parent, new Path(sourceText.getText()));
				if (modelPath == null)
					return;

				sourceText.setText(modelPath.toString());
				generator.setModelFileName(modelPath.toString());
				updateSource();
				updateLabels();
				control.layout();
			}
		});

		// Target folder selection.
		targetLabel = new CLabel(panel, SWT.RIGHT);
		targetLabel.setText("Target Folder: ");
		gridData = new GridData(SWT.RIGHT, SWT.CENTER, false, false);
		targetLabel.setLayoutData(gridData);

		targetDir = new Text(panel, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		gridData = new GridData(SWT.FILL, SWT.CENTER, true, false);
		targetDir.setText(getFirstNotNullTrimmed(generator.getTargetDirName(), ""));
		targetDir.setLayoutData(gridData);
		targetDir.addFocusListener(new FocusAdapter() {
			@Override
			public void focusLost(FocusEvent event) {
				generator.setTargetDirName(getFirstNotNullTrimmed(targetDir.getText(), ""));
				updateTarget();
				updateLabels();
				control.layout();
			}
		});

		targetDir.addKeyListener(new KeyAdapter() {

			@Override
			public void keyReleased(KeyEvent e) {
				generator.setTargetDirName(getFirstNotNullTrimmed(targetDir.getText(), ""));
				updateTarget();
				updateLabels();
				control.layout();
			}
		});

		button = new Button(panel, SWT.PUSH | SWT.LEFT);
		button.setText("Browse...");
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {

				Object[] current = new Object[] { targetDir.getText() };
				IPath targetPath = Utils.browseForTargetDir(parent, current);
				if (targetPath == null)
					return;

				targetDir.setText(targetPath.toString());
				generator.setTargetDirName(targetPath.toString());

				updateTarget();
				updateLabels();
				control.layout();
			}
		});

		// Right Panel
		panel = new Composite(control, 0);
		gridData = new GridData(SWT.FILL, SWT.FILL, false, true);
		panel.setLayoutData(gridData);
		panel.setLayout(new GridLayout(2, false));

		// Subsequent resizing can become incorrect if there is no initial text
		// size.
		final String uninitializedLabel = "uninitialized";

		// Code generation pushbuttons
		idlBtn = new Button(panel, SWT.PUSH);
		idlBtn.setText(SdkTransformer.TransformType.IDL.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		idlBtn.setLayoutData(gridData);
		idlBtn.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.IDL));
		idlLabel = new Label(panel, SWT.LEFT);
		idlLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		idlLabel.setLayoutData(gridData);

		hBtn = new Button(panel, SWT.PUSH);
		hBtn.setText(SdkTransformer.TransformType.H.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		hBtn.setLayoutData(gridData);
		hBtn.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.H));
		hLabel = new Label(panel, SWT.LEFT);
		hLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		hLabel.setLayoutData(gridData);

		cppBtn = new Button(panel, SWT.PUSH);
		cppBtn.setText(SdkTransformer.TransformType.CPP.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		cppBtn.setLayoutData(gridData);
		cppBtn.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.CPP));
		cppLabel = new Label(panel, SWT.LEFT);
		cppLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		cppLabel.setLayoutData(gridData);

		trhBtn = new Button(panel, SWT.PUSH);
		trhBtn.setText(SdkTransformer.TransformType.TRH.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		trhBtn.setLayoutData(gridData);
		trhBtn.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.TRH));
		trhLabel = new Label(panel, SWT.LEFT);
		trhLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		trhLabel.setLayoutData(gridData);

		trcBtn = new Button(panel, SWT.PUSH);
		trcBtn.setText(SdkTransformer.TransformType.TRC.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		trcBtn.setLayoutData(gridData);
		trcBtn.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.TRC));
		trcLabel = new Label(panel, SWT.LEFT);
		trcLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		trcLabel.setLayoutData(gridData);

		mpcBtn = new Button(panel, SWT.PUSH);
		mpcBtn.setText(SdkTransformer.TransformType.MPC.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		mpcBtn.setLayoutData(gridData);
		mpcBtn.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.MPC));
		mpcLabel = new Label(panel, SWT.LEFT);
		mpcLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		mpcLabel.setLayoutData(gridData);

		mpbBtn = new Button(panel, SWT.PUSH);
		mpbBtn.setText(SdkTransformer.TransformType.MPB.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		mpbBtn.setLayoutData(gridData);
		mpbBtn.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.MPB));
		mpbLabel = new Label(panel, SWT.LEFT);
		mpbLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		mpbLabel.setLayoutData(gridData);

		pathMpbBtn = new Button(panel, SWT.PUSH);
		pathMpbBtn.setText(SdkTransformer.TransformType.PATH_MPB.getText());
		gridData = new GridData(SWT.FILL, SWT.TOP, false, false);
		pathMpbBtn.setLayoutData(gridData);
		pathMpbBtn.addSelectionListener(new GenerateButtonListener(SdkTransformer.TransformType.PATH_MPB));
		pathMpbLabel = new Label(panel, SWT.LEFT);
		pathMpbLabel.setText(uninitializedLabel);
		gridData = new GridData(SWT.LEFT, SWT.CENTER, false, false);
		pathMpbLabel.setLayoutData(gridData);

		genAllBtn = new Button(panel, SWT.PUSH);
		genAllBtn.setText("Generate All");
		gridData = new GridData(SWT.LEFT, SWT.TOP, false, false);
		genAllBtn.setLayoutData(gridData);
		genAllBtn.addSelectionListener(new GenerateButtonListener(null));
	}

	private final class GenerateButtonListener extends SelectionAdapter {
		private TransformType whichTransform;

		// / @param which transform output, or null to generate all
		public GenerateButtonListener(TransformType which) {
			whichTransform = which;
		}

		public void widgetSelected(SelectionEvent e) {
			if (!editor.getSite().getPage().saveEditor(editor, true /* confirm */)) {
				// user canceled
				return;
			}
			IStatusLineManager statusLineManager = editor.getActionBars().getStatusLineManager();

                        generator.setSourceName(editor.getPartName());
			String msg = generator.validate();
			if (msg != null) {
				statusLineManager.setErrorMessage("Code Generation: " + msg);			   
				return;
			}

			// This is the progress monitor on the status line.
			// IProgressMonitor progressMonitor
			// = statusLineManager != null ?
			// statusLineManager.getProgressMonitor(): new
			// NullProgressMonitor();

			// Do the work within an operation because this is a long running
			// activity that modifies the workbench.
			WorkspaceModifyOperation operation = new WorkspaceModifyOperation() {
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
				new ProgressMonitorDialog(control.getShell()).run(true, false, operation);
				editor.firePropertyChange(IEditorPart.PROP_DIRTY); // Maybe this
				// will
				// cause a
				// refresh?
				if (statusLineManager != null) {
					statusLineManager.setMessage("Code Generation Complete");
				}

			} catch (Exception exception) {
				// Something went wrong that shouldn't.
				GeneratorEditorPlugin.INSTANCE.log(exception);
				if (statusLineManager != null) {
					statusLineManager.setErrorMessage("Code Generation: " + exception.getMessage());
				}
			}
		}
	}

	private boolean isFileAccesible(String path) {
		if (path == null || path.isEmpty())
			return false;

		try {
			URL url = fileProvider.fromWorkspace(path, false);
			if (url == null)
				return false;

			File ff = new File(url.toURI());
			return ff.canRead();

		} catch (MalformedURLException e) {
			return false;
		} catch (URISyntaxException e) {
			return false;
		}
	}

	private boolean isFolderUsable(String path) {
		if (path == null || path.isEmpty())
			return false;

		// HACK: IFileProvider#fromWorkspace() requires that if the path is
		// absolute then it must have the form /project/resource...
		IPath p = Path.fromOSString(path);
		if (p == null || (p.isAbsolute() && p.segmentCount() < 2))
			return false;

		try {
			URL url = fileProvider.fromWorkspace(path, true);
			if (url == null)
				return true;

			File ff = new File(url.toURI());
			return !ff.exists() || ff.canWrite();

		} catch (MalformedURLException e) {
			return false;
		} catch (URISyntaxException e) {
			return false;
		}
	}

	protected void updateSource() {
		String text = getFirstNotNullTrimmed(sourceText.getText(), "");
		if (isFileAccesible(text)) {
			sourceText.setForeground(control.getDisplay().getSystemColor(SWT.COLOR_BLACK));
			sourceText.setToolTipText(null);
			sourceLabel.setImage(null);
		} else {
			sourceText.setForeground(sourceText.getDisplay().getSystemColor(SWT.COLOR_RED));
			sourceText.setToolTipText("Cannot access " + text + " for reading");
			sourceLabel.setImage(ExtendedImageRegistry.INSTANCE.getImage(errorimage));
		}
		sourceLabel.setSize(sourceLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, false));
	}

	protected static String getFirstNotNullTrimmed(String... args) {
		for (String t : args) {
			if (t != null)
				return t.trim();
		}
		throw new NullPointerException("None was non-null: " + args);
	}

	protected void updateTarget() {
		String text = getFirstNotNullTrimmed(targetDir.getText(), "");
		if (isFolderUsable(text)) {
			targetDir.setForeground(control.getDisplay().getSystemColor(SWT.COLOR_BLACK));
			if (isFileAccesible(text)) {
				targetDir.setToolTipText(null);
				targetLabel.setImage(null);
			} else {
				targetDir.setToolTipText(text + " does not yet exist");
				targetLabel.setImage(ExtendedImageRegistry.INSTANCE.getImage(warningimage));
			}
		} else {
			targetDir.setForeground(targetDir.getDisplay().getSystemColor(SWT.COLOR_RED));
			targetDir.setToolTipText("Cannot use " + text + " for writing");
			targetLabel.setImage(ExtendedImageRegistry.INSTANCE.getImage(errorimage));
		}

		targetLabel.setSize(targetLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, false));
	}

	private void updateLabels() {
		String src = getFirstNotNullTrimmed(sourceText.getText(), "");
		boolean validSource = isFileAccesible(src);

		IPath path = Path.fromOSString(src);
		String filename = (src.isEmpty() || path.isEmpty()) ? "Invalid" : path.segment(path.segmentCount() - 1);
		String extension = (src.isEmpty() || path.isEmpty()) ? null : path.getFileExtension();
		String noext = (extension == null) ? filename : filename.substring(0, filename.indexOf(extension) - 1);
		String basename = (validSource) ? generator.getModelName() : noext;

		idlLabel.setEnabled(validSource);
		hLabel.setEnabled(validSource);
		cppLabel.setEnabled(validSource);
		trhLabel.setEnabled(validSource);
		trcLabel.setEnabled(validSource);
		mpcLabel.setEnabled(validSource);
		mpbLabel.setEnabled(validSource);
		pathMpbLabel.setEnabled(validSource);

		idlBtn.setEnabled(validSource);
		hBtn.setEnabled(validSource);
		cppBtn.setEnabled(validSource);
		trhBtn.setEnabled(validSource);
		trcBtn.setEnabled(validSource);
		mpcBtn.setEnabled(validSource);
		mpbBtn.setEnabled(validSource);
		pathMpbBtn.setEnabled(validSource);

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

		String tgt = getFirstNotNullTrimmed(targetDir.getText(), "");
		boolean validTarget = isFolderUsable(tgt);

		genAllBtn.setEnabled(validSource && validTarget);

		IStatusLineManager statusLineManager = editor.getActionBars().getStatusLineManager();
		statusLineManager.setErrorMessage(validSource & validTarget ? null : "Unable to generate into " + targetDir.getText());
	}

	@Override
	public Control getControl() {
		return control;
	}

	@Override
	public void refresh() {
		sourceText.setText(getFirstNotNullTrimmed(generator.getModelFileName(), ""));
		targetDir.setText(getFirstNotNullTrimmed(generator.getTargetDirName(), ""));
		updateSource();
		updateTarget();
		updateLabels();
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
		generator.setEditingDomain(editingDomain);
		refresh();
	}

	@Override
	protected Widget doFindInputItem(Object element) {
		if (generator.isModelSource(element)) {
			return sourceText;
		} else if (generator.isModelTarget(element)) {
			return targetDir;
		}
		return null;
	}

	@Override
	protected Widget doFindItem(Object element) {
		return doFindInputItem(element);
	}

	@Override
	protected void doUpdateItem(Widget item, Object element, boolean fullMap) {
		if (!fullMap) {
			return;

		} else if (generator.isModelSource(element)) {
			if (item == sourceText) {
				sourceText.setText(generator.getModelFileName());
			}

		} else if (generator.isModelTarget(element)) {
			if (item == targetDir) {
				targetDir.setText(generator.getTargetDirName());
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
