package org.opendds.modeling.uml2tools.mirrored;

import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Shape;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.PointList;
import org.eclipse.draw2d.geometry.Rectangle;

/**
 * @generated
 */
public class AssociationClassRhomb extends Shape {

	/**
	 * @generated
	 */
	public AssociationClassRhomb() {
		this.addPoint(new Point(20, 0));
		this.addPoint(new Point(40, 20));
		this.addPoint(new Point(20, 40));
		this.addPoint(new Point(0, 20));
		this.setFill(true);
		this.setLineWidth(1);
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

}
