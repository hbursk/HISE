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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;




ParameterSlider::ParameterSlider(NodeBase* node_, int index) :
	SimpleTimer(node_->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
	parameterToControl(node_->getParameter(index)),
	node(node_),
	pTree(node_->getParameter(index)->getTreeWithValue())
{
	setName(pTree[PropertyIds::ID].toString());

	connectionListener.setTypesToWatch({ PropertyIds::Connections, PropertyIds::ModulationTargets });
	connectionListener.setCallback(pTree.getRoot(), valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(ParameterSlider::updateOnConnectionChange));

	rangeListener.setCallback(pTree, RangeHelpers::getRangeIds(),
		valuetree::AsyncMode::Coallescated,
		BIND_MEMBER_FUNCTION_2(ParameterSlider::updateRange));

	valueListener.setCallback(pTree, { PropertyIds::Value },
		valuetree::AsyncMode::Asynchronously,
		[this](Identifier, var newValue)
	{
		setValue(newValue, dontSendNotification);
		repaint();
	});

	addListener(this);
	setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	setTextBoxStyle(Slider::TextBoxBelow, false, 100, 18);
	setLookAndFeel(&laf);

	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->updateText();
	}

	checkEnabledState();

	setColour(Slider::ColourIds::textBoxTextColourId, Colours::white);

	setScrollWheelEnabled(false);
}
    
ParameterSlider::~ParameterSlider()
{
    removeListener(this);
}

void ParameterSlider::updateOnConnectionChange(ValueTree p, bool wasAdded)
{
	if (!matchesConnection(p))
		return;

	checkEnabledState();
}

void ParameterSlider::checkEnabledState()
{
	modulationActive = getConnectionSourceTree().isValid();

	setEnabled(!modulationActive);

	if (modulationActive)
		start();
	else
		stop();

	if (auto g = findParentComponentOfClass<DspNetworkGraph>())
		g->repaint();
}

void ParameterSlider::updateRange(Identifier, var)
{
	auto range = RangeHelpers::getDoubleRange(pTree);

	setRange(range.getRange(), range.interval);
	setSkewFactor(range.skew);

	repaint();
}

bool ParameterSlider::isInterestedInDragSource(const SourceDetails& details)
{
	if (details.sourceComponent == this)
		return false;

	if(dynamic_cast<cable::dynamic::editor*>(details.sourceComponent.get()) != nullptr)
		return false;

	return !isReadOnlyModulated;
}

void ParameterSlider::paint(Graphics& g)
{
	Slider::paint(g);

	if (macroHoverIndex != -1)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.drawRect(getLocalBounds(), 1);
	}
}


void ParameterSlider::itemDragEnter(const SourceDetails& )
{
	macroHoverIndex = 1;
	repaint();
}


void ParameterSlider::itemDragExit(const SourceDetails& )
{
	macroHoverIndex = -1;
	repaint();
}



void ParameterSlider::itemDropped(const SourceDetails& dragSourceDetails)
{
	macroHoverIndex = -1;

	if (auto wc = findParentComponentOfClass<WrapperSlot>())
	{
		auto copy = SourceDetails(dragSourceDetails);
		copy.description.getDynamicObject()->setProperty("NumVoices", 8);
		
		parameterToControl->addConnectionFrom(copy.description);
	}

	parameterToControl->addConnectionFrom(dragSourceDetails.description);
	repaint();	
}

Array<NodeContainer::MacroParameter*> ParameterSlider::getConnectedMacroParameters()
{
	Array<NodeContainer::MacroParameter*> list;

	if (parameterToControl == nullptr)
		return list;

	auto unTypedList = parameterToControl->getConnectedMacroParameters();

	for (auto t : unTypedList)
		list.add(dynamic_cast<NodeContainer::MacroParameter*>(t));

	return list;
}

juce::ValueTree ParameterSlider::getConnectionSourceTree()
{
	auto pId = parameterToControl->getId();
	auto nId = parameterToControl->parent->getId();
	auto n = parameterToControl->parent->getRootNetwork();

	{
		auto containers = n->getListOfNodesWithType<NodeContainer>(true);

		for (auto c : containers)
		{
			for (auto p : c->getParameterTree())
			{
				auto cTree = p.getChildWithName(PropertyIds::Connections);

				for (auto c : cTree)
				{
					if (c[PropertyIds::NodeId].toString() == nId &&
						c[PropertyIds::ParameterId].toString() == pId)
					{
						return c;
					}
				}
			}
		}
	}

	{
		auto modNodes = n->getListOfNodesWithType<ModulationSourceNode>(true);

		for (auto mn : modNodes)
		{
			auto mTree = mn->getValueTree().getChildWithName(PropertyIds::ModulationTargets);

			for (auto mt : mTree)
			{
				if (mt[PropertyIds::NodeId].toString() == nId &&
					mt[PropertyIds::ParameterId].toString() == pId)
				{
					return mt;
				}
			}

			auto sTree = mn->getValueTree().getChildWithName(PropertyIds::SwitchTargets);

			for (auto sts : sTree)
			{
				for (auto st : sts.getChildWithName(PropertyIds::Connections))
				{
					if (st[PropertyIds::NodeId].toString() == nId &&
						st[PropertyIds::ParameterId].toString() == pId)
					{
						return st;
					}
				}
			}
		}
	}

	return {};
}

bool ParameterSlider::matchesConnection(ValueTree& c) const
{
	if (parameterToControl == nullptr)
		return false;

	return parameterToControl->matchesConnection(c);

}

void ParameterSlider::mouseDown(const MouseEvent& e)
{
	if (e.mods.isShiftDown())
	{
		Slider::showTextBox();
		return;
	}

	if (e.mods.isRightButtonDown())
	{
		auto pe = new MacroPropertyEditor(node, pTree);

		pe->setName("Edit Parameter");

		auto g = findParentComponentOfClass<DspNetworkGraph::ScrollableParent>();
		auto b = g->getLocalArea(this, getLocalBounds());

		g->setCurrentModalWindow(pe, b);
	}
	else
	{
		auto dp = findParentComponentOfClass<DspNetworkGraph>();

		if (dp->probeSelectionEnabled && isEnabled())
		{
			parameterToControl->isProbed = !parameterToControl->isProbed;
			dp->repaint();
			return;
		}

		Slider::mouseDown(e);
	}
}




void ParameterSlider::mouseEnter(const MouseEvent& e)
{
	if (!isEnabled())
	{
		findParentComponentOfClass<DspNetworkGraph>()->repaint();
	}

	Slider::mouseEnter(e);
}

void ParameterSlider::mouseExit(const MouseEvent& e)
{
	if (!isEnabled())
	{
		findParentComponentOfClass<DspNetworkGraph>()->repaint();
	}

	Slider::mouseExit(e);
}

void ParameterSlider::mouseDoubleClick(const MouseEvent&)
{
	if (!isEnabled())
	{
		auto c = getConnectionSourceTree();

		auto v = parameterToControl->getValue();

		if (c.isValid())
			c.getParent().removeChild(c, parameterToControl->parent->getUndoManager());
		
		
		setValue(v, dontSendNotification);
	}
}

void ParameterSlider::sliderDragStarted(Slider*)
{
	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->startDrag();
	}
}

void ParameterSlider::sliderDragEnded(Slider*)
{
	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->endDrag();
	}
}

void ParameterSlider::sliderValueChanged(Slider*)
{
	if (parameterToControl != nullptr)
	{
		parameterToControl->getTreeWithValue().setProperty(PropertyIds::Value, getValue(), parameterToControl->parent->getUndoManager());
	}

	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->updateText();
	}
}


juce::String ParameterSlider::getTextFromValue(double value)
{
	if (parameterToControl == nullptr)
		return "Empty";

	if (parameterToControl->valueNames.isEmpty())
		return Slider::getTextFromValue(value);

	int index = (int)value;
	return parameterToControl->valueNames[index];
}

double ParameterSlider::getValueFromText(const String& text)
{
	if (parameterToControl == nullptr)
		return 0.0;

	if (parameterToControl->valueNames.contains(text))
		return (double)parameterToControl->valueNames.indexOf(text);

	return Slider::getValueFromText(text);
}

ParameterKnobLookAndFeel::ParameterKnobLookAndFeel()
{
	cachedImage_smalliKnob_png = ImageProvider::getImage(ImageProvider::ImageType::KnobEmpty); // ImageCache::getFromMemory(BinaryData::knob_empty_png, BinaryData::knob_empty_pngSize);
	cachedImage_knobRing_png = ImageProvider::getImage(ImageProvider::ImageType::KnobUnmodulated); // ImageCache::getFromMemory(BinaryData::ring_unmodulated_png, BinaryData::ring_unmodulated_pngSize);

#if USE_BACKEND
	withoutArrow = ImageCache::getFromMemory(BinaryData::knob_without_arrow_png, BinaryData::knob_without_arrow_pngSize);
#endif
}


juce::Font ParameterKnobLookAndFeel::getLabelFont(Label&)
{
	return GLOBAL_BOLD_FONT();
}

    

juce::Label* ParameterKnobLookAndFeel::createSliderTextBox(Slider& slider)
{
	auto l = new SliderLabel(slider);
	l->refreshWithEachKey = false;

	l->setJustificationType(Justification::centred);
	l->setKeyboardType(TextInputTarget::decimalKeyboard);

	auto tf = slider.findColour(Slider::ColourIds::textBoxTextColourId);

	l->setColour(Label::textColourId, tf);
	l->setColour(Label::backgroundColourId, Colours::transparentBlack);
	l->setColour(Label::outlineColourId, Colours::transparentBlack);
	l->setColour(TextEditor::textColourId, tf);
	l->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
	l->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
	l->setColour(TextEditor::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.5f));
	l->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
	l->setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));

	return l;
}

void ParameterKnobLookAndFeel::drawRotarySlider(Graphics& g, int , int , int width, int height, float , float , float , Slider& s)
{
	

	const double value = s.getValue();
	const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
	const double proportion = pow(normalizedValue, s.getSkewFactor());

	auto ps = dynamic_cast<ParameterSlider*>(&s);
	auto modValue = ps->parameterToControl->getValue();
	const double normalisedModValue = (modValue - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
	float modProportion = jlimit<float>(0.0f, 1.0f, pow((float)normalisedModValue, (float)s.getSkewFactor()));

	modProportion = FloatSanitizers::sanitizeFloatNumber(modProportion);

	bool isBipolar = -1.0 * s.getMinimum() == s.getMaximum();

	auto b = s.getLocalBounds();
	b = b.removeFromTop(48);
	b = b.withSizeKeepingCentre(48, 48).translated(0.0f, 3.0f);

	drawVectorRotaryKnob(g, b.toFloat(), modProportion, isBipolar, s.isMouseOverOrDragging(true), s.isMouseButtonDown(), s.isEnabled(), modProportion);


#if 0
	height = s.getHeight();
	width = s.getWidth();

	int xOffset = (s.getWidth() - 48) / 2;

	const int filmstripHeight = cachedImage_smalliKnob_png.getHeight() / 128;

	
	if (!s.isEnabled())
	{
		g.drawImageWithin(withoutArrow, xOffset, 3, 48, 48, RectanglePlacement::fillDestination);
	}
	else
	{
		const double value = s.getValue();
		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = pow(normalizedValue, s.getSkewFactor());
		const int stripIndex = (int)(proportion * 127);


		const int offset = stripIndex * filmstripHeight;

		Image clip = cachedImage_smalliKnob_png.getClippedImage(Rectangle<int>(0, offset, filmstripHeight, filmstripHeight));

		g.setColour(Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
		g.drawImage(clip, xOffset, 3, 48, 48, 0, 0, filmstripHeight, filmstripHeight);
	}

	{
		auto ps = dynamic_cast<ParameterSlider*>(&s);
		auto value = ps->parameterToControl->getValue();
		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = jlimit(0.0, 1.0, pow(normalizedValue, s.getSkewFactor()));
		const int stripIndex = (int)(proportion * 127);

		Image *imageToUse = &cachedImage_knobRing_png;

		Image clipRing = imageToUse->getClippedImage(Rectangle<int>(0, (int)(stripIndex * filmstripHeight), filmstripHeight, filmstripHeight));

		//g.setColour(Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
		g.drawImage(clipRing, xOffset, 3, 48, 48, 0, 0, filmstripHeight, filmstripHeight);
	}
#endif
}

MacroParameterSlider::MacroParameterSlider(NodeBase* node, int index) :
	slider(node, index)
{
	addAndMakeVisible(slider);
	setWantsKeyboardFocus(true);
}

void MacroParameterSlider::resized()
{
	auto b = getLocalBounds();
	b.removeFromBottom(10);
	slider.setBounds(b);
}

void MacroParameterSlider::mouseDrag(const MouseEvent& )
{
	if (editEnabled)
	{
		if (auto container = DragAndDropContainer::findParentDragContainerFor(this))
		{
			auto details = DragHelpers::createDescription(slider.node->getId(), slider.parameterToControl->getId());

			container->startDragging(details, &slider, ModulationSourceBaseComponent::createDragImageStatic(false));

			findParentComponentOfClass<DspNetworkGraph>()->repaint();
		}
	}
}

void MacroParameterSlider::mouseUp(const MouseEvent& e)
{
	findParentComponentOfClass<DspNetworkGraph>()->repaint();
}

void MacroParameterSlider::paintOverChildren(Graphics& g)
{
	if (editEnabled)
	{
		Path p;
		p.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));

		auto pa = getLocalBounds().toFloat().withSizeKeepingCentre(20.0f, 20.0f).translated(0.0, -8);

		PathFactory::scalePath(p, pa);

		g.setColour(Colours::white.withAlpha(0.3f));
		g.fillPath(p);

		auto b = getLocalBounds().reduced(2).toFloat();
		b.removeFromBottom(8.0f);

		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.05f));
		g.fillRoundedRectangle(b, 3);

		if (hasKeyboardFocus(true))
		{
			g.setColour(Colour(SIGNAL_COLOUR));
			g.drawRoundedRectangle(b, 3, 1.0f);
		}
	}
}

void MacroParameterSlider::setEditEnabled(bool shouldBeEnabled)
{
	slider.setEnabled(!shouldBeEnabled);
	editEnabled = shouldBeEnabled;

	if (editEnabled)
		slider.addMouseListener(this, true);
	else
		slider.removeMouseListener(this);

	if (shouldBeEnabled)
	{
		slider.setMouseCursor(ModulationSourceBaseComponent::createMouseCursor());
	}
	else
		slider.setMouseCursor({});

	repaint();
}

bool MacroParameterSlider::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey)
	{
		auto treeToRemove = slider.parameterToControl->data;
		auto um = slider.node->getUndoManager();

		auto f = [treeToRemove, um]()
		{
			treeToRemove.getParent().removeChild(treeToRemove, um);
		};

		MessageManager::callAsync(f);

		return true;
	}

	return false;
}

}

