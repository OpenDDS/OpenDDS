package org.opendds.modeling.uml2tools.mirrored;

import org.eclipse.draw2d.MarginBorder;
import org.eclipse.draw2d.RectangleFigure;
import org.eclipse.draw2d.StackLayout;
import org.eclipse.draw2d.ToolbarLayout;
import org.eclipse.gmf.runtime.draw2d.ui.figures.WrappingLabel;
import org.eclipse.uml2.diagram.common.draw2d.NameAndStereotypeBlock;
import org.eclipse.uml2.diagram.common.draw2d.StereotypeLabel2;

/**
 * @generated
 */
public class ClassFigure extends RectangleFigure {

	/**
	 * @generated
	 */
	private RectangleFigure fFigureClassFigure_PropertiesCompartment;
	/**
	 * @generated
	 */
	private RectangleFigure fFigureClassFigure_OperationsCompartment;
	/**
	 * @generated
	 */
	private RectangleFigure fFigureClassFigure_ClassesCompartment;
	/**
	 * @generated
	 */
	private RectangleFigure fFigureClassFigure_LiteralsCompartment;
	/**
	 * @generated
	 */
	private RectangleFigure fFigureClassFigure_OthersCompartment;
	/**
	 * @generated
	 */
	private NameAndStereotypeBlock fNameAndStereotypeBlock;

	/**
	 * @generated
	 */
	public ClassFigure() {

		ToolbarLayout layoutThis = new ToolbarLayout();
		layoutThis.setStretchMinorAxis(true);
		layoutThis.setMinorAlignment(ToolbarLayout.ALIGN_CENTER);

		layoutThis.setSpacing(0);
		layoutThis.setVertical(true);

		this.setLayoutManager(layoutThis);

		this.setLineWidth(1);

		this.setBorder(new MarginBorder(1, 1, 10, 1));
		createContents();
	}

	/**
	 * @generated
	 */
	private void createContents() {

		fNameAndStereotypeBlock = new NameAndStereotypeBlock();

		fNameAndStereotypeBlock.setBorder(new MarginBorder(8, 5, 6, 5));

		this.add(fNameAndStereotypeBlock);

		fFigureClassFigure_PropertiesCompartment = new RectangleFigure();
		fFigureClassFigure_PropertiesCompartment.setOutline(false);
		fFigureClassFigure_PropertiesCompartment.setLineWidth(1);

		this.add(fFigureClassFigure_PropertiesCompartment);

		StackLayout layoutFFigureClassFigure_PropertiesCompartment = new StackLayout();

		layoutFFigureClassFigure_PropertiesCompartment
				.setObserveVisibility(true);

		fFigureClassFigure_PropertiesCompartment
				.setLayoutManager(layoutFFigureClassFigure_PropertiesCompartment);

		fFigureClassFigure_OperationsCompartment = new RectangleFigure();
		fFigureClassFigure_OperationsCompartment.setOutline(false);
		fFigureClassFigure_OperationsCompartment.setLineWidth(1);

		this.add(fFigureClassFigure_OperationsCompartment);

		StackLayout layoutFFigureClassFigure_OperationsCompartment = new StackLayout();

		layoutFFigureClassFigure_OperationsCompartment
				.setObserveVisibility(true);

		fFigureClassFigure_OperationsCompartment
				.setLayoutManager(layoutFFigureClassFigure_OperationsCompartment);

		fFigureClassFigure_ClassesCompartment = new RectangleFigure();
		fFigureClassFigure_ClassesCompartment.setOutline(false);
		fFigureClassFigure_ClassesCompartment.setLineWidth(1);

		this.add(fFigureClassFigure_ClassesCompartment);

		StackLayout layoutFFigureClassFigure_ClassesCompartment = new StackLayout();

		layoutFFigureClassFigure_ClassesCompartment.setObserveVisibility(true);

		fFigureClassFigure_ClassesCompartment
				.setLayoutManager(layoutFFigureClassFigure_ClassesCompartment);

		fFigureClassFigure_LiteralsCompartment = new RectangleFigure();
		fFigureClassFigure_LiteralsCompartment.setOutline(false);
		fFigureClassFigure_LiteralsCompartment.setLineWidth(1);

		this.add(fFigureClassFigure_LiteralsCompartment);

		StackLayout layoutFFigureClassFigure_LiteralsCompartment = new StackLayout();

		layoutFFigureClassFigure_LiteralsCompartment.setObserveVisibility(true);

		fFigureClassFigure_LiteralsCompartment
				.setLayoutManager(layoutFFigureClassFigure_LiteralsCompartment);

		fFigureClassFigure_OthersCompartment = new RectangleFigure();
		fFigureClassFigure_OthersCompartment.setOutline(false);
		fFigureClassFigure_OthersCompartment.setLineWidth(1);

		this.add(fFigureClassFigure_OthersCompartment);

		StackLayout layoutFFigureClassFigure_OthersCompartment = new StackLayout();

		layoutFFigureClassFigure_OthersCompartment.setObserveVisibility(true);

		fFigureClassFigure_OthersCompartment
				.setLayoutManager(layoutFFigureClassFigure_OthersCompartment);

	}

	/**
	 * @generated
	 */
	private boolean myUseLocalCoordinates = false;

	/**
	 * @generated
	 */
	protected boolean useLocalCoordinates() {
		return myUseLocalCoordinates;
	}

	/**
	 * @generated
	 */
	protected void setUseLocalCoordinates(boolean useLocalCoordinates) {
		myUseLocalCoordinates = useLocalCoordinates;
	}

	/**
	 * @generated
	 */
	public RectangleFigure getFigureClassFigure_PropertiesCompartment() {
		return fFigureClassFigure_PropertiesCompartment;
	}

	/**
	 * @generated
	 */
	public RectangleFigure getFigureClassFigure_OperationsCompartment() {
		return fFigureClassFigure_OperationsCompartment;
	}

	/**
	 * @generated
	 */
	public RectangleFigure getFigureClassFigure_ClassesCompartment() {
		return fFigureClassFigure_ClassesCompartment;
	}

	/**
	 * @generated
	 */
	public RectangleFigure getFigureClassFigure_LiteralsCompartment() {
		return fFigureClassFigure_LiteralsCompartment;
	}

	/**
	 * @generated
	 */
	public RectangleFigure getFigureClassFigure_OthersCompartment() {
		return fFigureClassFigure_OthersCompartment;
	}

	/**
	 * @generated
	 */
	public NameAndStereotypeBlock getNameAndStereotypeBlock() {
		return fNameAndStereotypeBlock;
	}

	/**
	 * @generated
	 */
	public StereotypeLabel2 getFigureClassFigure_StereoLabel() {
		return getNameAndStereotypeBlock().getStereotypeLabel();
	}

	/**
	 * @generated
	 */
	public WrappingLabel getFigureClassFigure_NameLabel() {
		return getNameAndStereotypeBlock().getNameLabel();
	}

}
