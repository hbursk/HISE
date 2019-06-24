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


struct DspNetwork::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(DspNetwork, processBlock);
	API_VOID_METHOD_WRAPPER_2(DspNetwork, prepareToPlay);
	API_METHOD_WRAPPER_3(DspNetwork, create);
	API_METHOD_WRAPPER_1(DspNetwork, get);
	//API_VOID_METHOD_WRAPPER_0(DspNetwork, disconnectAll);
	//API_VOID_METHOD_WRAPPER_3(DspNetwork, injectAfter);
};

DspNetwork::DspNetwork(hise::ProcessorWithScriptingContent* p, ValueTree data_) :
	ConstScriptingObject(p, 2),
	data(data_)
{
	ownedFactories.add(new NodeContainerFactory(this));
	ownedFactories.add(new core::Factory(this));
	ownedFactories.add(new math::Factory(this));
	ownedFactories.add(new routing::Factory(this));
	ownedFactories.add(new filters::Factory(this));
	ownedFactories.add(new dynamics::Factory(this));
	ownedFactories.add(new stk::StkFactory(this));

	for (auto nf : ownedFactories)
		nodeFactories.add(nf);

	nodeFactories.add(meta::Factory::getInstance(this));

#if HI_ENABLE_CUSTOM_NODE_LOCATION
	nodeFactories.add(custom::Factory::getInstance(this));
#endif

	loader = new DspFactory::LibraryLoader(dynamic_cast<Processor*>(p));

	signalPath = createFromValueTree(true, data.getChild(0), true);

	networkParameterHandler.root = signalPath;

	ADD_API_METHOD_1(processBlock);
	ADD_API_METHOD_2(prepareToPlay);
	ADD_API_METHOD_3(create);
	ADD_API_METHOD_1(get);
	//ADD_API_METHOD_0(disconnectAll);
	//ADD_API_METHOD_3(injectAfter);

	selectionUpdater = new SelectionUpdater(*this);

	setEnableUndoManager(enableUndo);
}

DspNetwork::~DspNetwork()
{
	selectionUpdater = nullptr;
	nodes.clear();
	
}


void DspNetwork::setNumChannels(int newNumChannels)
{
	signalPath->getValueTree().setProperty(PropertyIds::NumChannels, newNumChannels, nullptr);
}

void DspNetwork::rightClickCallback(const MouseEvent& e, Component* c)
{

#if USE_BACKEND

	auto* d = new DspNetworkGraph(this);

	d->setSize(600, 600);

	auto editor = GET_BACKEND_ROOT_WINDOW(c);

	MouseEvent ee = e.getEventRelativeTo(editor);

	editor->getRootFloatingTile()->showComponentInRootPopup(d, editor, ee.getMouseDownPosition());
#else
	ignoreUnused(e, c);
#endif
}

NodeBase* DspNetwork::getNodeForValueTree(const ValueTree& v)
{
	if (!v.isValid())
		return {};

	for (auto n : nodes)
	{
		if (n->getValueTree() == v)
			return n;
	}

	return nullptr;
}

NodeBase::List DspNetwork::getListOfUnconnectedNodes() const
{
	NodeBase::List unconnectedNodes;
	unconnectedNodes.ensureStorageAllocated(nodes.size());

	for (auto n : nodes)
	{
		if (!n->isConnected())
			unconnectedNodes.add(n);
	}

	return unconnectedNodes;
}


juce::ValueTree DspNetwork::getListOfAvailableModulesAsTree() const
{
	ValueTree v(PropertyIds::Nodes);

	for (auto nf : nodeFactories)
	{
		auto list = nf->getModuleList();

		ValueTree f("Factory");
		f.setProperty(PropertyIds::ID, nf->getId().toString(), nullptr);

		for (auto l : list)
		{
			ValueTree n(PropertyIds::Node);
			n.setProperty(PropertyIds::FactoryPath, f[PropertyIds::ID].toString(), nullptr);
			n.setProperty(PropertyIds::ID, l, nullptr);
			f.addChild(n, -1, nullptr);
		}

		v.addChild(f, -1, nullptr);
	}

	return v;
}

juce::StringArray DspNetwork::getListOfAllAvailableModuleIds() const
{
	auto l = dynamic_cast<DspFactory::LibraryLoader*>(loader.get());

	StringArray sa;

	for (auto nf : nodeFactories)
	{
		nf->setNetworkToUse(const_cast<DspNetwork*>(this));
		sa.addArray(nf->getModuleList());
	}

	return sa;
}


juce::StringArray DspNetwork::getListOfUsedNodeIds() const
{
	StringArray sa;

	for (auto n : nodes)
	{
		if (isInSignalPath(n))
			sa.add(n->getId());
	}

	return sa;
}


juce::StringArray DspNetwork::getListOfUnusedNodeIds() const
{
	auto list = getListOfUnconnectedNodes();

	StringArray sa;

	for (auto l : list)
		sa.add(l->getId());

	return sa;
}

void DspNetwork::process(AudioSampleBuffer& b, HiseEventBuffer* e)
{
	ScopedLock sl(getConnectionLock());

	ProcessData d(b.getArrayOfWritePointers(), b.getNumChannels(), b.getNumSamples());
	d.eventBuffer = e;

	signalPath->process(d);
}

juce::Identifier DspNetwork::getParameterIdentifier(int parameterIndex)
{
	return signalPath->getParameter(parameterIndex)->getId();
}

void DspNetwork::prepareToPlay(double sampleRate, double blockSize)
{
	ScopedLock sl(getConnectionLock());

	PrepareSpecs ps;
	ps.sampleRate = sampleRate;
	ps.blockSize = (int)blockSize;
	ps.numChannels = signalPath->getNumChannelsToProcess();
	ps.voiceIndex = &voiceIndex;

	signalPath->prepare(ps);
}

void DspNetwork::processBlock(var pData)
{
	ScopedLock sl(getConnectionLock());

	if (auto ar = pData.getArray())
	{
		ProcessData d;
		d.numChannels = ar->size();

		int index = 0;

		for (const auto& v : *ar)
		{
			if (auto bf = v.getBuffer())
			{
				int thisSamples = bf->buffer.getNumSamples();

				if (d.size == 0)
					d.size = thisSamples;
				else if (d.size != thisSamples)
					reportScriptError("Buffer mismatch");

				currentData[index++] = bf->buffer.getWritePointer(0);
			}
		}

		d.data = currentData;

		signalPath->process(d);
	}
}

var DspNetwork::create(String path, String id, bool createPolyNode)
{
	var existing = get(id);

	if (existing.isObject())
		return existing;
	
	ValueTree newNodeData(PropertyIds::Node);

	if (id.isEmpty())
	{
		String nameToUse = path.contains(".") ? path.fromFirstOccurrenceOf(".", false, false) : path;
		StringArray unused;
		id = getNonExistentId(nameToUse, unused);
	}

	newNodeData.setProperty(PropertyIds::ID, id, nullptr);
	newNodeData.setProperty(PropertyIds::FactoryPath, path, nullptr);
	
	return createFromValueTree(createPolyNode, newNodeData);
}



var DspNetwork::get(String id) const
{
	if (id.isEmpty())
		return {};

	if (id == getId())
		return var(signalPath);

	for (auto n : nodes)
	{
		if (n->getId() == id)
			return var(n);
	}
	
	return {};
}

NodeBase* DspNetwork::createFromValueTree(bool createPolyIfAvailable, ValueTree d, bool forceCreate)
{
	auto id = d[PropertyIds::ID].toString();

	auto existing = forceCreate ? var() : get(id);

	if (existing.isObject())
		return dynamic_cast<NodeBase*>(existing.getObject());

	auto childNodes = d.getChildWithName(PropertyIds::Nodes);

	for (auto c : childNodes)
	{
		createFromValueTree(createPolyIfAvailable, c, forceCreate);
	}

	NodeBase::Ptr newNode;

	for (auto nf : nodeFactories)
	{
		nf->setNetworkToUse(this);
		newNode = nf->createNode(d, createPolyIfAvailable);

		if (newNode != nullptr)
		{
			for (auto n : nodes)
			{
				if (newNode->getId() == n->getId())
				{
					StringArray sa;

					auto oldId = newNode->getId();
					auto newId = getNonExistentId(id, sa);

					updateId(newNode, newId);
				}
			}
			//StringArray usedIds;
			//updateIdsInValueTree(d, usedIds);

			nodes.add(newNode);
			return newNode.get();
		}
	}

	return nullptr;
}



bool DspNetwork::isInSignalPath(NodeBase* b) const
{
	if (signalPath == nullptr)
		return false;

	if (b == nullptr)
		return false;

	if (b == signalPath.get())
		return true;

	return b->getValueTree().isAChildOf(signalPath->getValueTree());
}


scriptnode::NodeBase* DspNetwork::getNodeWithId(const String& id) const
{
	return dynamic_cast<NodeBase*>(get(id).getObject());
}

void DspNetwork::addToSelection(NodeBase* node, ModifierKeys mods)
{
	auto pNode = node->getParentNode();

	while (mods.isAnyModifierKeyDown() && pNode != nullptr)
	{
		if (isSelected(pNode))
			return;

		pNode = pNode->getParentNode();
	}

	selection.addToSelectionBasedOnModifiers(node, mods);
}


bool DspNetwork::updateIdsInValueTree(ValueTree& v, StringArray& usedIds)
{
	auto oldId = v[PropertyIds::ID].toString();
	auto newId = getNonExistentId(oldId, usedIds);

	if (oldId != newId)
	{
		v.setProperty(PropertyIds::ID, newId, signalPath->getUndoManager());

#if 0
		auto um = signalPath->getUndoManager();

		DBG("_______________________________________________________");
		DBG("New: " + newId + ", old: " + oldId);
		DBG("Update ID in " + v.createXml()->createDocument(""));

		auto updateIds = [newId, oldId, um](ValueTree& v)
		{
			auto oldConnection = v[PropertyIds::Connection].toString();

			if (oldConnection.contains(oldId))
			{
				auto newConnection = oldConnection.replace(oldId, newId);
				DBG("Update " + oldConnection + " to " + newConnection);
				v.setProperty(PropertyIds::Connection, newConnection, um);
			};

			return false;
		};

		valuetree::Helpers::foreach(v, updateIds);
#endif
	}

	auto nodeTree = v.getChildWithName(PropertyIds::Nodes);

	for (auto n : nodeTree)
	{
		updateIdsInValueTree(n, usedIds);
	}

	return true;
}


juce::String DspNetwork::getNonExistentId(String id, StringArray& usedIds) const
{
	if (!get(id).isObject())
		return id;

	int trailingIndex = id.getTrailingIntValue();

	auto idWithoutNumber = trailingIndex == 0 ? id : id.upToLastOccurrenceOf(String(trailingIndex), false, false);

	trailingIndex++;

	id = idWithoutNumber + String(trailingIndex);

	var existingNode = get(id);

	while (existingNode.isObject() || usedIds.contains(id))
	{
		trailingIndex++;
		id = idWithoutNumber + String(trailingIndex);
		existingNode = get(id);
	}

	usedIds.add(id);

	return id;
}


void DspNetwork::updateId(NodeBase* n, const String& newId)
{
	auto oldId = n->getId();
	n->setValueTreeProperty(PropertyIds::ID, newId);

	auto um = signalPath->getUndoManager();

	DBG("_______________________________________________________");
	DBG("New: " + newId + ", old: " + oldId);
	
	auto updateIds = [newId, oldId, um](ValueTree& v)
	{
		auto oldConnection = v[PropertyIds::Connection].toString();

		if (oldConnection.contains(oldId))
		{
			auto newConnection = oldConnection.replace(oldId, newId);
			DBG("Update " + oldConnection + " to " + newConnection);
			v.setProperty(PropertyIds::Connection, newConnection, um);
		};

		return false;
	};

	valuetree::Helpers::foreach(n->getValueTree().getChildWithName(PropertyIds::Parameters), updateIds);
}

juce::ValueTree DspNetwork::cloneValueTreeWithNewIds(const ValueTree& treeToClone)
{
	struct IdChange
	{
		String oldId;
		String newId;
	};

	auto c = treeToClone.createCopy();

	StringArray sa;
	for (auto n : nodes)
		sa.add(n->getId());

	auto saRef = &sa;

	Array<IdChange> changes;

	auto changeRef = &changes;

	auto setNewId = [changeRef, saRef, this](ValueTree& v)
	{
		if (v.hasType(PropertyIds::Node))
		{
			auto thisId = v[PropertyIds::ID].toString();

			if (this->get(thisId))
			{
				auto newId = this->getNonExistentId(thisId, *saRef);
				changeRef->add({ thisId, newId });
				v.setProperty(PropertyIds::ID, newId, nullptr);
			}
		}

		return false;
	};

	valuetree::Helpers::foreach(c, setNewId);

	DBG("Changed IDS:");

	

	for (auto& ch : changes)
	{
		DBG(ch.oldId + " -> " + ch.newId);

		auto updateParameter = [ch](ValueTree& v)
		{
			if (v.hasType(PropertyIds::Parameter))
			{
				auto conId = v[PropertyIds::Connection].toString();

				if (conId.contains(ch.oldId))
				{
					auto newConId = conId.replace(ch.oldId, ch.newId);
					DBG("Update " + conId + " - >" + newConId);
					v.setProperty(PropertyIds::Connection, newConId, nullptr);
				}
			}

			return false;
		};

		valuetree::Helpers::foreach(c, updateParameter);

		auto updateConnection = [ch](ValueTree& v)
		{
			if (v.hasType(PropertyIds::Connection))
			{
				auto nId = v[PropertyIds::NodeId].toString();

				if (nId == ch.oldId)
				{
					DBG("Update connection ");
					v.setProperty(PropertyIds::NodeId, ch.newId, nullptr);
				}
			}

			return false;
		};

		valuetree::Helpers::foreach(c, updateConnection);
		
		auto updateSendConnection = [ch](ValueTree& v)
		{
			if (v.hasType(PropertyIds::Property) && 
				v[PropertyIds::ID].toString() == PropertyIds::Connection.toString())
			{
				auto oldValue = v[PropertyIds::Value].toString();

				if (oldValue == ch.oldId)
				{
					DBG("Update feedback connection");
					v.setProperty(PropertyIds::Value, ch.newId, nullptr);
				}
			}

			return false;
		};

		valuetree::Helpers::foreach(c, updateSendConnection);
	}

	return c;
}

DspNetwork* DspNetwork::Holder::getOrCreate(const String& id)
{
	auto asScriptProcessor = dynamic_cast<ProcessorWithScriptingContent*>(this);

	for (auto n : networks)
	{
		if (n->getId() == id)
			return n;
	}

	ValueTree v(scriptnode::PropertyIds::Network);
	v.setProperty(scriptnode::PropertyIds::ID, id, nullptr);

	ValueTree s(scriptnode::PropertyIds::Node);
	s.setProperty(scriptnode::PropertyIds::FactoryPath, "container.chain", nullptr);
	s.setProperty(scriptnode::PropertyIds::ID, id, nullptr);

	v.addChild(s, -1, nullptr);

	auto newNetwork = new DspNetwork(asScriptProcessor, v);

	networks.add(newNetwork);

	setActiveNetwork(newNetwork);

	return newNetwork;
}


juce::StringArray DspNetwork::Holder::getIdList()
{
	StringArray sa;

	for (auto n : networks)
		sa.add(n->getId());

	return sa;
}

void DspNetwork::Holder::saveNetworks(ValueTree& d) const
{
	if (networks.size() > 0)
	{
		ValueTree v("Networks");

		for (auto n : networks)
		{
			v.addChild(n->getValueTree().createCopy(), -1, nullptr);
		}

		d.addChild(v, -1, nullptr);
	}
}

void DspNetwork::Holder::restoreNetworks(const ValueTree& d)
{
	auto v = d.getChildWithName("Networks");

	if (v.isValid())
	{
		networks.clear();

		for (auto c : v)
		{
			auto newNetwork = new DspNetwork(dynamic_cast<ProcessorWithScriptingContent*>(this),
				c.createCopy());

			networks.add(newNetwork);
			setActiveNetwork(newNetwork);
		}
	}
}

scriptnode::NodeBase* NodeFactory::createNode(ValueTree data, bool createPolyIfAvailable) const
{
	auto path = data[PropertyIds::FactoryPath].toString();
	auto className = Identifier(path.upToFirstOccurrenceOf(".", false, false));

	if (className != getId())
		return nullptr;

	auto id = Identifier(path.fromFirstOccurrenceOf(".", false, false));

	if (createPolyIfAvailable)
	{
		for (const auto& item : polyNodes)
		{
			if (item.id() == id)
			{
				auto newNode = item.cb(network.get(), data);

				if (item.pb)
					item.pb(newNode);

				return newNode;
			}
		}
	}

	for (const auto& item : monoNodes)
	{
		if (item.id() == id)
		{
			auto newNode = item.cb(network.get(), data);

			if (item.pb)
				item.pb(newNode);

			return newNode;
		}
	}

	return nullptr;
}

DspNetwork::SelectionUpdater::SelectionUpdater(DspNetwork& parent_) :
	parent(parent_)
{
	WeakReference<DspNetwork> weakParent = &parent;

	auto f = [weakParent, this]()
	{
		if (weakParent != nullptr)
			weakParent.get()->selection.addChangeListener(this);
	};

	MessageManager::callAsync(f);
}

DspNetwork::SelectionUpdater::~SelectionUpdater()
{
	parent.selection.removeChangeListener(this);
}

void DspNetwork::SelectionUpdater::changeListenerCallback(ChangeBroadcaster*)
{
	auto& list = parent.selection.getItemArray();

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->selectionChanged(list);
	}
}

}

