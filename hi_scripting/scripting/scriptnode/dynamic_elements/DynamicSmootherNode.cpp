/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace control
{

	void pma_editor::resized()
	{
		setRepaintsOnMouseActivity(true);

		dragPath.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));

		auto b = getLocalBounds().toFloat();
		b = b.withSizeKeepingCentre(28.0f, 28.0f);

		b = b.translated(0.0f, JUCE_LIVE_CONSTANT_OFF(5.0f));
		
		getProperties().set("circleOffsetY", -0.5f * (float)getHeight() +2.0f);

		PathFactory::scalePath(dragPath, b);
	}

void pma_editor::paint(Graphics& g)
{

	
	

	g.setFont(GLOBAL_BOLD_FONT());

	auto r = obj->currentRange;

	String start, mid, end;

	int numDigits = jmax<int>(1, -1 * roundToInt(log10(r.interval)));

	start = String(r.start, numDigits);

	mid = String(r.convertFrom0to1(0.5), numDigits);

	end = String(r.end, numDigits);

	auto b = getLocalBounds().toFloat();

	float w = JUCE_LIVE_CONSTANT_OFF(85.0f);

	auto midCircle = b.withSizeKeepingCentre(w, w).translated(0.0f, 5.0f);

	float r1 = JUCE_LIVE_CONSTANT_OFF(3.0f);
	float r2 = JUCE_LIVE_CONSTANT_OFF(5.0f);

	float startArc = JUCE_LIVE_CONSTANT_OFF(-2.5f);
	float endArc = JUCE_LIVE_CONSTANT_OFF(2.5f);

	Colour trackColour = JUCE_LIVE_CONSTANT_OFF(Colour(0xff4f4f4f));

	auto createArc = [startArc, endArc](Rectangle<float> b, float startNormalised, float endNormalised)
	{
		Path p;

		auto s = startArc + jmin(startNormalised, endNormalised) * (endArc - startArc);
		auto e = startArc + jmax(startNormalised, endNormalised) * (endArc - startArc);

		s = jlimit(startArc, endArc, s);
		e = jlimit(startArc, endArc, e);

		p.addArc(b.getX(), b.getY(), b.getWidth(), b.getHeight(), s, e, true);

		return p;
	};

	auto oc = midCircle;
	auto mc = midCircle.reduced(5.0f);
	auto ic = midCircle.reduced(10.0f);

	auto outerTrack = createArc(oc, 0.0f, 1.0f);
	auto midTrack = createArc(mc, 0.0f, 1.0f);
	auto innerTrack = createArc(ic, 0.0f, 1.0f);

	if (isMouseOver())
		trackColour = trackColour.withMultipliedBrightness(1.1f);

	if (isMouseButtonDown())
		trackColour = trackColour.withMultipliedBrightness(1.1f);

	g.setColour(trackColour);

	g.strokePath(outerTrack, PathStrokeType(r1));
	g.strokePath(midTrack, PathStrokeType(r2));
	g.strokePath(innerTrack, PathStrokeType(r1));

	g.fillPath(dragPath);

	auto data = obj->getUIData();

	auto nrm = [r](double v)
	{
		return r.convertTo0to1(v);
	};

	auto mulValue = nrm(data.value * data.mulValue);
	auto totalValue = nrm(data.getValue());

	auto outerRing = createArc(oc, mulValue, totalValue);
	auto midRing = createArc(mc, 0.0f, totalValue);
	auto innerRing = createArc(ic, 0.0f, mulValue);
	auto valueRing = createArc(ic, 0.0f, nrm(data.value));

	auto c1 = MultiOutputDragSource::getFadeColour(0, 2).withAlpha(0.8f);
	auto c2 = MultiOutputDragSource::getFadeColour(1, 2).withAlpha(0.8f);

	auto ab = getLocalBounds().removeFromBottom(5).toFloat();
	ab.removeFromLeft(ab.getWidth() / 3.0f);
	auto ar2 = ab.removeFromLeft(ab.getWidth() / 2.0f).withSizeKeepingCentre(5.0f, 5.0f);
	auto ar1 = ab.withSizeKeepingCentre(5.0f, 5.0f);

	g.setColour(Colour(c1));
	g.strokePath(outerRing, PathStrokeType(r1-1.0f));
	g.setColour(c1.withMultipliedAlpha(data.addValue == 0.0 ? 0.2f : 1.0f));
	g.fillEllipse(ar1);
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xffd7d7d7)));
	g.strokePath(midRing, PathStrokeType(r2 - 1.0f));

	g.setColour(c2.withMultipliedAlpha(JUCE_LIVE_CONSTANT_OFF(0.4f)));
	g.strokePath(valueRing, PathStrokeType(r1-1.0f));
	g.setColour(c2.withMultipliedAlpha(data.mulValue == 1.0 ? 0.2f : 1.0f));
	g.fillEllipse(ar2);
	g.setColour(c2);
	g.strokePath(innerRing, PathStrokeType(r1));

	b.removeFromTop(18.0f);

	g.setColour(Colours::white.withAlpha(0.3f));

	Rectangle<float> t((float)getWidth() / 2.0f - 35.0f, 0.0f, 70.0f, 15.0f);

	g.drawText(start, t.translated(-70.0f, 80.0f), Justification::centred);
	g.drawText(mid, t, Justification::centred);
	g.drawText(end, t.translated(70.0f, 80.0f), Justification::centred);
}

minmax_editor::minmax_editor(MinMaxBase* b, PooledUIUpdater* u) :
	ScriptnodeExtraComponent<MinMaxBase>(b, u),
	dragger(u)
{
	addAndMakeVisible(rangePresets);
	addAndMakeVisible(dragger);
	rangePresets.setLookAndFeel(&slaf);
	rangePresets.setColour(ComboBox::ColourIds::textColourId, Colours::white.withAlpha(0.8f));

	for (const auto& p : presets.presets)
		rangePresets.addItem(p.id, p.index + 1);

	rangePresets.onChange = [this]()
	{
		for (const auto& p : presets.presets)
		{
			if (p.id == rangePresets.getText())
				setRange(p.nr);
		}
	};

	setSize(256, 128);
	start();
}

void minmax_editor::paint(Graphics& g)
{
	ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, pathArea, false);
	g.setFont(GLOBAL_BOLD_FONT());

	auto r = lastData.range.rng;

	String s;
	s << "[" << r.start;
	s << " - " << r.end << "]";

	g.setColour(Colours::white);
	g.drawText(s, pathArea.reduced(UIValues::NodeMargin), lastData.range.rng.skew < 1.0 ? Justification::centredTop : Justification::centredBottom);

	auto c = Colours::white.withAlpha(0.8f);

	if (auto nc = findParentComponentOfClass<NodeComponent>())
	{
		auto c2 = nc->header.colour;
		if (!c2.isTransparent())
			c = c2;
	}

	g.setColour(c);

	Path dst;

	UnblurryGraphics ug(g, *this, true);
	auto ps = ug.getPixelSize();
	float l[2] = { 4.0f * ps, 4.0f * ps };

	PathStrokeType(2.0f * ps).createDashedStroke(dst, fullPath, l, 2);

	g.fillPath(dst);

	g.strokePath(valuePath, PathStrokeType(4.0f * ug.getPixelSize()));

}

void minmax_editor::timerCallback()
{
	auto thisData = getObject()->getUIData();

	if (!(thisData == lastData))
	{
		lastData = thisData;
		rebuildPaths();
	}
}

void minmax_editor::setRange(InvertableParameterRange newRange)
{
	if (auto nc = findParentComponentOfClass<NodeComponent>())
	{
		auto n = nc->node;

		RangeHelpers::storeDoubleRange(n->getParameter(1)->data, newRange, n->getUndoManager());
		RangeHelpers::storeDoubleRange(n->getParameter(2)->data, newRange, n->getUndoManager());

		n->getParameter(1)->setValueFromUI(newRange.inv ? newRange.rng.end : newRange.rng.start);
		n->getParameter(2)->setValueFromUI(newRange.inv ? newRange.rng.start : newRange.rng.end);
		n->getParameter(3)->setValueFromUI(newRange.rng.skew);
		n->getParameter(4)->setValueFromUI(newRange.rng.interval);
		rebuildPaths();
	}
}

void minmax_editor::rebuildPaths()
{
	fullPath.clear();
	valuePath.clear();

	if (getWidth() == 0)
		return;

	auto maxValue = (float)lastData.range.convertFrom0to1(1.0);
	auto minValue = (float)lastData.range.convertFrom0to1(0.0);

	auto vToY = [&](float v)
	{
		return lastData.range.inv ? v : -v;
	};

	fullPath.startNewSubPath(1.0f, vToY(maxValue));
	fullPath.startNewSubPath(1.0f, vToY(minValue));
	fullPath.startNewSubPath(0.0f, vToY(maxValue));
	fullPath.startNewSubPath(0.0f, vToY(minValue));
	
	valuePath.startNewSubPath(1.0f, vToY(maxValue));
	valuePath.startNewSubPath(1.0f, vToY(minValue));
	valuePath.startNewSubPath(0.0f, vToY(maxValue));
	valuePath.startNewSubPath(0.0f, vToY(minValue));
	
	for (int i = 0; i < getWidth(); i += 3)
	{
		float normX = (float)i / (float)getWidth();

		auto v = lastData.range.convertFrom0to1(normX);
		v = lastData.range.snapToLegalValue(v);

		auto y = vToY((float)v);

		fullPath.lineTo(normX, y);

		if (lastData.value > normX)
			valuePath.lineTo(normX, y);
	}

	fullPath.lineTo(1.0, vToY(maxValue));

	if (lastData.value == 1.0)
		valuePath.lineTo(1.0, vToY(maxValue));

	auto pb = pathArea.reduced((float)UIValues::NodeMargin);

	fullPath.scaleToFit(pb.getX(), pb.getY(), pb.getWidth(), pb.getHeight(), false);
	valuePath.scaleToFit(pb.getX(), pb.getY(), pb.getWidth(), pb.getHeight(), false);

	repaint();
}

void minmax_editor::resized()
{
	auto b = getLocalBounds();
	auto bottom = b.removeFromBottom(28);
	b.removeFromBottom(UIValues::NodeMargin);
	dragger.setBounds(bottom.removeFromRight(256));
	bottom.removeFromRight(UIValues::NodeMargin);
	rangePresets.setBounds(bottom);

	b.reduced(UIValues::NodeWidth, 0);

	pathArea = b.toFloat().reduced(128/2, 0);

	rebuildPaths();
}

void bipolar_editor::paint(Graphics& g)
{
	ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, pathArea, false);

	UnblurryGraphics ug(g, *this, true);

	g.setColour(Colours::white.withAlpha(0.1f));

	auto pb = pathArea.reduced(UIValues::NodeMargin / 2);

	ug.draw1PxHorizontalLine(pathArea.getCentreY(), pb.getX(), pb.getRight());
	ug.draw1PxVerticalLine(pathArea.getCentreX(), pb.getY(), pb.getBottom());
	ug.draw1PxRect(pb);

	auto c = Colours::white.withAlpha(0.8f);

	if (auto nc = findParentComponentOfClass<NodeComponent>())
	{
		auto c2 = nc->header.colour;
		if (!c2.isTransparent())
			c = c2;
	}

	g.setColour(c);

	Path dst;

	auto ps = ug.getPixelSize();
	float l[2] = { 4.0f * ps, 4.0f * ps };

	PathStrokeType(2.0f * ps).createDashedStroke(dst, outlinePath, l, 2);

	g.fillPath(dst);

	g.strokePath(valuePath, PathStrokeType(4.0f * ug.getPixelSize()));
}

}

namespace smoothers
{
	dynamic::editor::editor(dynamic* p, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<dynamic>(p, updater),
		plotter(updater),
		modeSelector("Linear Ramp")
	{
		addAndMakeVisible(modeSelector);
		addAndMakeVisible(plotter);
		setSize(200, 58);
	}

	void dynamic::editor::paint(Graphics& g)
	{
		float a = JUCE_LIVE_CONSTANT_OFF(0.4f);

		auto b = getLocalBounds();
		b.removeFromTop(modeSelector.getHeight());
		b.removeFromTop(UIValues::NodeMargin);
		g.setColour(currentColour.withAlpha(a));

		b = b.removeFromRight(b.getHeight()).reduced(5);

		g.fillEllipse(b.toFloat());
	}

	void dynamic::editor::timerCallback()
	{
		double v = 0.0;
		currentColour = getObject()->lastValue.getChangedValue(v) ? Colour(SIGNAL_COLOUR) : Colours::grey;
		repaint();

		modeSelector.initModes(smoothers::dynamic::getSmoothNames(), plotter.getSourceNodeFromParent());
	}

}

}