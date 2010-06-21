package org.opendds.modeling.uml2tools.mirrored;

import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.MarginBorder;
import org.eclipse.draw2d.RectangleFigure;
import org.eclipse.draw2d.Shape;
import org.eclipse.draw2d.ToolbarLayout;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.PointList;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.gmf.runtime.draw2d.ui.figures.WrappingLabel;
import org.opendds.modeling.uml2tools.mirrored.activator.PluginActivator;

/**
 * @generated
 */
public class SecondaryPackageFigure extends Shape {

	/**
	 * @generated
	 */
	private WrappingLabel fFigureSecondaryPackage_PackageLabel;
	/**
	 * @generated
	 */
	private WrappingLabel fFigureSecondaryPackage_NameLabel;
	/**
	 * @generated
	 */
	private RectangleFigure fFigureSecondaryPackage_Imports;

	/**
	 * @generated
	 */
	public SecondaryPackageFigure() {

		ToolbarLayout layoutThis = new ToolbarLayout();
		layoutThis.setStretchMinorAxis(true);
		layoutThis.setMinorAlignment(ToolbarLayout.ALIGN_CENTER);

		layoutThis.setSpacing(0);
		layoutThis.setVertical(true);

		this.setLayoutManager(layoutThis);

		this
				.addPoint(new Point(PluginActivator.getDefault().getMapMode()
						.DPtoLP(0), PluginActivator.getDefault().getMapMode()
						.DPtoLP(0)));
		this.addPoint(new Point(PluginActivator.getDefault().getMapMode()
				.DPtoLP(0), PluginActivator.getDefault().getMapMode()
				.DPtoLP(40)));
		this.addPoint(new Point(PluginActivator.getDefault().getMapMode()
				.DPtoLP(35), PluginActivator.getDefault().getMapMode().DPtoLP(
				40)));
		this.addPoint(new Point(PluginActivator.getDefault().getMapMode()
				.DPtoLP(40), PluginActivator.getDefault().getMapMode().DPtoLP(
				35)));
		this.addPoint(new Point(PluginActivator.getDefault().getMapMode()
				.DPtoLP(40), PluginActivator.getDefault().getMapMode()
				.DPtoLP(0)));
		this.setFill(true);
		this.setLineWidth(1);

		this.setBorder(new MarginBorder(PluginActivator.getDefault()
				.getMapMode().DPtoLP(5), PluginActivator.getDefault()
				.getMapMode().DPtoLP(5), PluginActivator.getDefault()
				.getMapMode().DPtoLP(13), PluginActivator.getDefault()
				.getMapMode().DPtoLP(10)));
		createContents();
	}

	/**
	 * @generated
	 */
	private void createContents() {

		fFigureSecondaryPackage_PackageLabel = new WrappingLabel();
		fFigureSecondaryPackage_PackageLabel.setText("");

		this.add(fFigureSecondaryPackage_PackageLabel);

		fFigureSecondaryPackage_NameLabel = new WrappingLabel();
		fFigureSecondaryPackage_NameLabel.setText("");

		this.add(fFigureSecondaryPackage_NameLabel);

		fFigureSecondaryPackage_Imports = new RectangleFigure();
		fFigureSecondaryPackage_Imports.setOutline(false);
		fFigureSecondaryPackage_Imports.setLineWidth(1);

		this.add(fFigureSecondaryPackage_Imports);

	}

	/**
	 * @generated
	 */
	private final PointList myTemplate = new PointList();
	/**
	 * @generated
	 */
	private Rectangle myTemplateBounds;

	/**
	 * @generated
	 */
	public void addPoint(Point point) {
		myTemplate.addPoint(point);
		myTemplateBounds = null;
	}

	/**
	 * @generated
	 */
	protected void fillShape(Graphics graphics) {
		Rectangle bounds = getBounds();
		graphics.pushState();
		graphics.translate(bounds.x, bounds.y);
		graphics.fillPolygon(scalePointList());
		graphics.popState();
	}

	/**
	 * @generated
	 */
	protected void outlineShape(Graphics graphics) {
		Rectangle bounds = getBounds();
		graphics.pushState();
		graphics.translate(bounds.x, bounds.y);
		graphics.drawPolygon(scalePointList());
		graphics.popState();
	}

	/**
	 * @generated
	 */
	private Rectangle getTemplateBounds() {
		if (myTemplateBounds == null) {
			myTemplateBounds = myTemplate.getBounds().getCopy().union(0, 0);
			//just safety -- we are going to use this as divider 
			if (myTemplateBounds.width < 1) {
				myTemplateBounds.width = 1;
			}
			if (myTemplateBounds.height < 1) {
				myTemplateBounds.height = 1;
			}
		}
		return myTemplateBounds;
	}

	/**
	 * @generated
	 */
	private int[] scalePointList() {
		Rectangle pointsBounds = getTemplateBounds();
		Rectangle actualBounds = getBounds();

		float xScale = ((float) actualBounds.width) / pointsBounds.width;
		float yScale = ((float) actualBounds.height) / pointsBounds.height;

		if (xScale == 1 && yScale == 1) {
			return myTemplate.toIntArray();
		}
		int[] scaled = (int[]) myTemplate.toIntArray().clone();
		for (int i = 0; i < scaled.length; i += 2) {
			scaled[i] = (int) Math.floor(scaled[i] * xScale);
			scaled[i + 1] = (int) Math.floor(scaled[i + 1] * yScale);
		}
		return scaled;
	}

	/**
	 * @generated
	 */
	public WrappingLabel getFigureSecondaryPackage_PackageLabel() {
		return fFigureSecondaryPackage_PackageLabel;
	}

	/**
	 * @generated
	 */
	public WrappingLabel getFigureSecondaryPackage_NameLabel() {
		return fFigureSecondaryPackage_NameLabel;
	}

	/**
	 * @generated
	 */
	public RectangleFigure getFigureSecondaryPackage_Imports() {
		return fFigureSecondaryPackage_Imports;
	}

}
