package org.opendds.modeling.uml2tools.mirrored;

import org.eclipse.draw2d.BorderLayout;
import org.eclipse.draw2d.MarginBorder;
import org.eclipse.draw2d.RectangleFigure;
import org.eclipse.draw2d.StackLayout;
import org.eclipse.draw2d.ToolbarLayout;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.gmf.runtime.draw2d.ui.figures.ConstrainedToolbarLayout;
import org.eclipse.gmf.runtime.draw2d.ui.figures.WrappingLabel;
import org.eclipse.uml2.diagram.common.draw2d.CenterLayout;
import org.eclipse.uml2.diagram.common.draw2d.PartialRectangleFigure;
import org.opendds.modeling.uml2tools.mirrored.activator.PluginActivator;

/**
 * @generated
 */
public class PackageFigure extends RectangleFigure {

	/**
	 * @generated
	 */
	private WrappingLabel fFigurePackageFigure_name;
	/**
	 * @generated
	 */
	private RectangleFigure fFigurePackageFigure_PackagesCompartment;
	/**
	 * @generated
	 */
	private RectangleFigure fFigurePackageFigure_ClassesCompartment;
	/**
	 * @generated
	 */
	private RectangleFigure fFigurePackageFigure_OthersCompartment;

	/**
	 * @generated
	 */
	public PackageFigure() {

		BorderLayout layoutThis = new BorderLayout();
		this.setLayoutManager(layoutThis);

		this.setFill(false);
		this.setOutline(false);
		this.setLineWidth(1);
		createContents();
	}

	/**
	 * @generated
	 */
	private void createContents() {

		RectangleFigure packageFigure_AuxTop0 = new RectangleFigure();
		packageFigure_AuxTop0.setFill(false);
		packageFigure_AuxTop0.setOutline(false);
		packageFigure_AuxTop0.setLineWidth(1);

		this.add(packageFigure_AuxTop0, BorderLayout.TOP);

		ConstrainedToolbarLayout layoutPackageFigure_AuxTop0 = new ConstrainedToolbarLayout();

		layoutPackageFigure_AuxTop0.setStretchMajorAxis(true);

		layoutPackageFigure_AuxTop0.setVertical(false);

		packageFigure_AuxTop0.setLayoutManager(layoutPackageFigure_AuxTop0);

		PartialRectangleFigure packageFigure_AuxLeftTab1 = new PartialRectangleFigure();

		packageFigure_AuxLeftTab1.setBottomShown(false);

		packageFigure_AuxLeftTab1.setPreferredSize(new Dimension(
				PluginActivator.getDefault().getMapMode().DPtoLP(1),
				PluginActivator.getDefault().getMapMode().DPtoLP(30)));

		packageFigure_AuxTop0.add(packageFigure_AuxLeftTab1);

		RectangleFigure packageFigure_AuxRightPadding1 = new RectangleFigure();
		packageFigure_AuxRightPadding1.setFill(false);
		packageFigure_AuxRightPadding1.setOutline(false);
		packageFigure_AuxRightPadding1.setLineWidth(1);
		packageFigure_AuxRightPadding1.setPreferredSize(new Dimension(
				PluginActivator.getDefault().getMapMode().DPtoLP(1),
				PluginActivator.getDefault().getMapMode().DPtoLP(30)));

		packageFigure_AuxTop0.add(packageFigure_AuxRightPadding1);

		RectangleFigure packageFigure_AuxCenter0 = new RectangleFigure();
		packageFigure_AuxCenter0.setLineWidth(1);

		packageFigure_AuxCenter0.setBorder(new MarginBorder(PluginActivator
				.getDefault().getMapMode().DPtoLP(1), PluginActivator
				.getDefault().getMapMode().DPtoLP(1), PluginActivator
				.getDefault().getMapMode().DPtoLP(10), PluginActivator
				.getDefault().getMapMode().DPtoLP(1)));

		this.add(packageFigure_AuxCenter0, BorderLayout.CENTER);

		ToolbarLayout layoutPackageFigure_AuxCenter0 = new ToolbarLayout();
		layoutPackageFigure_AuxCenter0.setStretchMinorAxis(true);
		layoutPackageFigure_AuxCenter0
				.setMinorAlignment(ToolbarLayout.ALIGN_CENTER);

		layoutPackageFigure_AuxCenter0.setSpacing(0);
		layoutPackageFigure_AuxCenter0.setVertical(true);

		packageFigure_AuxCenter0
				.setLayoutManager(layoutPackageFigure_AuxCenter0);

		RectangleFigure packageFigure_NameContainer1 = new RectangleFigure();
		packageFigure_NameContainer1.setOutline(false);
		packageFigure_NameContainer1.setLineWidth(1);
		packageFigure_NameContainer1.setMinimumSize(new Dimension(
				PluginActivator.getDefault().getMapMode().DPtoLP(0),
				PluginActivator.getDefault().getMapMode().DPtoLP(20)));

		packageFigure_AuxCenter0.add(packageFigure_NameContainer1);

		CenterLayout layoutPackageFigure_NameContainer1 = new CenterLayout();

		packageFigure_NameContainer1
				.setLayoutManager(layoutPackageFigure_NameContainer1);

		fFigurePackageFigure_name = new WrappingLabel();
		fFigurePackageFigure_name.setText("");

		fFigurePackageFigure_name.setBorder(new MarginBorder(PluginActivator
				.getDefault().getMapMode().DPtoLP(5), PluginActivator
				.getDefault().getMapMode().DPtoLP(5), PluginActivator
				.getDefault().getMapMode().DPtoLP(5), PluginActivator
				.getDefault().getMapMode().DPtoLP(5)));

		packageFigure_NameContainer1.add(fFigurePackageFigure_name);

		fFigurePackageFigure_PackagesCompartment = new RectangleFigure();
		fFigurePackageFigure_PackagesCompartment.setOutline(false);
		fFigurePackageFigure_PackagesCompartment.setLineWidth(1);

		packageFigure_AuxCenter0.add(fFigurePackageFigure_PackagesCompartment);

		StackLayout layoutFFigurePackageFigure_PackagesCompartment = new StackLayout();

		layoutFFigurePackageFigure_PackagesCompartment
				.setObserveVisibility(true);

		fFigurePackageFigure_PackagesCompartment
				.setLayoutManager(layoutFFigurePackageFigure_PackagesCompartment);

		fFigurePackageFigure_ClassesCompartment = new RectangleFigure();
		fFigurePackageFigure_ClassesCompartment.setOutline(false);
		fFigurePackageFigure_ClassesCompartment.setLineWidth(1);

		packageFigure_AuxCenter0.add(fFigurePackageFigure_ClassesCompartment);

		StackLayout layoutFFigurePackageFigure_ClassesCompartment = new StackLayout();

		layoutFFigurePackageFigure_ClassesCompartment
				.setObserveVisibility(true);

		fFigurePackageFigure_ClassesCompartment
				.setLayoutManager(layoutFFigurePackageFigure_ClassesCompartment);

		fFigurePackageFigure_OthersCompartment = new RectangleFigure();
		fFigurePackageFigure_OthersCompartment.setOutline(false);
		fFigurePackageFigure_OthersCompartment.setLineWidth(1);

		packageFigure_AuxCenter0.add(fFigurePackageFigure_OthersCompartment);

		StackLayout layoutFFigurePackageFigure_OthersCompartment = new StackLayout();

		layoutFFigurePackageFigure_OthersCompartment.setObserveVisibility(true);

		fFigurePackageFigure_OthersCompartment
				.setLayoutManager(layoutFFigurePackageFigure_OthersCompartment);

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
	public WrappingLabel getFigurePackageFigure_name() {
		return fFigurePackageFigure_name;
	}

	/**
	 * @generated
	 */
	public RectangleFigure getFigurePackageFigure_PackagesCompartment() {
		return fFigurePackageFigure_PackagesCompartment;
	}

	/**
	 * @generated
	 */
	public RectangleFigure getFigurePackageFigure_ClassesCompartment() {
		return fFigurePackageFigure_ClassesCompartment;
	}

	/**
	 * @generated
	 */
	public RectangleFigure getFigurePackageFigure_OthersCompartment() {
		return fFigurePackageFigure_OthersCompartment;
	}

}
