//
// Copyright (c) 2008-2013 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Precompiled.h"
#include "Component.h"
#include "Context.h"
#include "CoreEvents.h"
#include "File.h"
#include "Log.h"
#include "PackageFile.h"
#include "Profiler.h"
#include "ReplicationState.h"
#include "Scene.h"
#include "SceneEvents.h"
#include "SmoothedTransform.h"
#include "WorkQueue.h"
#include "XMLFile.h"

#include "DebugNew.h"

namespace Urho3D
{

static const int ASYNC_LOAD_MIN_FPS = 30;
static const int ASYNC_LOAD_MAX_MSEC = (int)(1000.0f / ASYNC_LOAD_MIN_FPS);
static const float DEFAULT_SMOOTHING_CONSTANT = 50.0f;
static const float DEFAULT_SNAP_THRESHOLD = 5.0f;

OBJECTTYPESTATIC(Scene);

Scene::Scene(Context* context) :
    Node(context),
    replicatedNodeID_(FIRST_REPLICATED_ID),
    replicatedComponentID_(FIRST_REPLICATED_ID),
    localNodeID_(FIRST_LOCAL_ID),
    localComponentID_(FIRST_LOCAL_ID),
    checksum_(0),
    timeScale_(1.0f),
    elapsedTime_(0),
    smoothingConstant_(DEFAULT_SMOOTHING_CONSTANT),
    snapThreshold_(DEFAULT_SNAP_THRESHOLD),
    active_(true),
    asyncLoading_(false),
    threadedUpdate_(false)
{
    // Assign an ID to self so that nodes can refer to this node as a parent
    SetID(GetFreeNodeID(REPLICATED));
    NodeAdded(this);
    
    SubscribeToEvent(E_UPDATE, HANDLER(Scene, HandleUpdate));
}

Scene::~Scene()
{
    RemoveAllChildren();
    RemoveAllComponents();
    
    // Remove scene reference and owner from all nodes that still exist
    for (HashMap<unsigned, Node*>::Iterator i = replicatedNodes_.Begin(); i != replicatedNodes_.End(); ++i)
        i->second_->ResetScene();
    for (HashMap<unsigned, Node*>::Iterator i = localNodes_.Begin(); i != localNodes_.End(); ++i)
        i->second_->ResetScene();
}

void Scene::RegisterObject(Context* context)
{
    context->RegisterFactory<Scene>();
    
    REF_ACCESSOR_ATTRIBUTE(Scene, VAR_STRING, "Name", GetName, SetName, String, String(), AM_DEFAULT);
    REF_ACCESSOR_ATTRIBUTE(Scene, VAR_VECTOR3, "Position", GetPosition, SetPosition, Vector3, Vector3::ZERO, AM_DEFAULT | AM_LATESTDATA);
    REF_ACCESSOR_ATTRIBUTE(Scene, VAR_QUATERNION, "Rotation", GetRotation, SetRotation, Quaternion, Quaternion::IDENTITY, AM_FILE);
    REF_ACCESSOR_ATTRIBUTE(Scene, VAR_VECTOR3, "Scale", GetScale, SetScale, Vector3, Vector3::ONE, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(Scene, VAR_FLOAT, "Time Scale", GetTimeScale, SetTimeScale, float, 1.0f, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(Scene, VAR_FLOAT, "Smoothing Constant", GetSmoothingConstant, SetSmoothingConstant, float, DEFAULT_SMOOTHING_CONSTANT, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(Scene, VAR_FLOAT, "Snap Threshold", GetSnapThreshold, SetSnapThreshold, float, DEFAULT_SNAP_THRESHOLD, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(Scene, VAR_FLOAT, "Elapsed Time", GetElapsedTime, SetElapsedTime, float, 0.0f, AM_FILE);
    ATTRIBUTE(Scene, VAR_INT, "Next Replicated Node ID", replicatedNodeID_, FIRST_REPLICATED_ID, AM_FILE | AM_NOEDIT);
    ATTRIBUTE(Scene, VAR_INT, "Next Replicated Component ID", replicatedComponentID_, FIRST_REPLICATED_ID, AM_FILE | AM_NOEDIT);
    ATTRIBUTE(Scene, VAR_INT, "Next Local Node ID", localNodeID_, FIRST_LOCAL_ID, AM_FILE | AM_NOEDIT);
    ATTRIBUTE(Scene, VAR_INT, "Next Local Component ID", localComponentID_, FIRST_LOCAL_ID, AM_FILE | AM_NOEDIT);
    ATTRIBUTE(Scene, VAR_VARIANTMAP, "Variables", vars_, VariantMap(), AM_FILE); // Network replication of vars uses custom data
    ACCESSOR_ATTRIBUTE(Scene, VAR_STRING, "Variable Names", GetVarNamesAttr, SetVarNamesAttr, String, String(), AM_FILE | AM_NOEDIT);
    REF_ACCESSOR_ATTRIBUTE(Scene, VAR_BUFFER, "Network Rotation", GetNetRotationAttr, SetNetRotationAttr, PODVector<unsigned char>, PODVector<unsigned char>(), AM_NET | AM_LATESTDATA | AM_NOEDIT);
}

bool Scene::Load(Deserializer& source)
{
    PROFILE(LoadScene);
    
    StopAsyncLoading();
    
    // Check ID
    if (source.ReadFileID() != "USCN")
    {
        LOGERROR(source.GetName() + " is not a valid scene file");
        return false;
    }
    
    LOGINFO("Loading scene from " + source.GetName());
    
    // Load the whole scene, then perform post-load if successfully loaded
    if (Node::Load(source))
    {
        FinishLoading(&source);
        return true;
    }
    else
        return false;
}

bool Scene::Save(Serializer& dest)
{
    PROFILE(SaveScene);
    
    // Write ID first
    if (!dest.WriteFileID("USCN"))
    {
        LOGERROR("Could not save scene, writing to stream failed");
        return false;
    }
    
    Deserializer* ptr = dynamic_cast<Deserializer*>(&dest);
    if (ptr)
        LOGINFO("Saving scene to " + ptr->GetName());
    
    return Node::Save(dest);
}

bool Scene::LoadXML(const XMLElement& source)
{
    PROFILE(LoadSceneXML);
    
    StopAsyncLoading();
    
    // Load the whole scene, then perform post-load if successfully loaded
    // Note: the scene filename and checksum can not be set, as we only used an XML element
    if (Node::LoadXML(source))
    {
        FinishLoading(0);
        return true;
    }
    else
        return false;
}

void Scene::AddReplicationState(NodeReplicationState* state)
{
    Node::AddReplicationState(state);
    
    // This is the first update for a new connection. Mark all replicated nodes dirty
    for (HashMap<unsigned, Node*>::ConstIterator i = replicatedNodes_.Begin(); i != replicatedNodes_.End(); ++i)
        state->sceneState_->dirtyNodes_.Insert(i->first_);
}

bool Scene::LoadXML(Deserializer& source)
{
    PROFILE(LoadSceneXML);
    
    StopAsyncLoading();
    
    SharedPtr<XMLFile> xml(new XMLFile(context_));
    if (!xml->Load(source))
        return false;
    
    LOGINFO("Loading scene from " + source.GetName());
    
    if (Node::LoadXML(xml->GetRoot()))
    {
        FinishLoading(&source);
        return true;
    }
    else
        return false;
}

bool Scene::SaveXML(Serializer& dest)
{
    PROFILE(SaveSceneXML);
    
    SharedPtr<XMLFile> xml(new XMLFile(context_));
    XMLElement rootElem = xml->CreateRoot("scene");
    if (!SaveXML(rootElem))
        return false;
    
    Deserializer* ptr = dynamic_cast<Deserializer*>(&dest);
    if (ptr)
        LOGINFO("Saving scene to " + ptr->GetName());
    
    return xml->Save(dest);
}

bool Scene::LoadAsync(File* file)
{
    if (!file)
    {
        LOGERROR("Null file for async loading");
        return false;
    }
    
    StopAsyncLoading();
    
    // Check ID
    if (file->ReadFileID() != "USCN")
    {
        LOGERROR(file->GetName() + " is not a valid scene file");
        return false;
    }
    
    LOGINFO("Loading scene from " + file->GetName());
    
    Clear();
    
    // Store own old ID for resolving possible root node references
    unsigned nodeID = file->ReadUInt();
    resolver_.AddNode(nodeID, this);
    
    // Load root level components first
    if (!Node::Load(*file, resolver_, false))
        return false;
    
    // Then prepare for loading all root level child nodes in the async update
    asyncLoading_ = true;
    asyncProgress_.file_ = file;
    asyncProgress_.loadedNodes_ = 0;
    asyncProgress_.totalNodes_ = file->ReadVLE();
    
    return true;
}

bool Scene::LoadAsyncXML(File* file)
{
    if (!file)
    {
        LOGERROR("Null file for async loading");
        return false;
    }
    
    StopAsyncLoading();
    
    SharedPtr<XMLFile> xml(new XMLFile(context_));
    if (!xml->Load(*file))
        return false;
    
    LOGINFO("Loading scene from " + file->GetName());
    
    Clear();
    XMLElement rootElement = xml->GetRoot();
    
    // Store own old ID for resolving possible root node references
    unsigned nodeID = rootElement.GetInt("id");
    resolver_.AddNode(nodeID, this);
    
    // Load the root level components first
    if (!Node::LoadXML(rootElement, resolver_, false))
        return false;
    
    // Then prepare for loading all root level child nodes in the async update
    XMLElement childNodeElement = rootElement.GetChild("node");
    asyncLoading_ = true;
    asyncProgress_.file_ = file;
    asyncProgress_.xmlFile_ = xml;
    asyncProgress_.xmlElement_ = childNodeElement;
    asyncProgress_.loadedNodes_ = 0;
    asyncProgress_.totalNodes_ = 0;
    
    // Count the amount of child nodes
    while (childNodeElement)
    {
        ++asyncProgress_.totalNodes_;
        childNodeElement = childNodeElement.GetNext("node");
    }
    
    return true;
}

void Scene::StopAsyncLoading()
{
    asyncLoading_ = false;
    asyncProgress_.file_.Reset();
    asyncProgress_.xmlFile_.Reset();
    asyncProgress_.xmlElement_ = XMLElement::EMPTY;
    resolver_.Reset();
}

Node* Scene::Instantiate(Deserializer& source, const Vector3& position, const Quaternion& rotation, CreateMode mode)
{
    PROFILE(Instantiate);
    
    SceneResolver resolver;
    unsigned nodeID = source.ReadInt();
    // Rewrite IDs when instantiating
    Node* node = CreateChild(0, mode);
    resolver.AddNode(nodeID, node);
    if (node->Load(source, resolver, true, true, mode))
    {
        resolver.Resolve();
        node->ApplyAttributes();
        node->SetTransform(position, rotation);
        return node;
    }
    else
    {
        node->Remove();
        return 0;
    }
}

Node* Scene::InstantiateXML(const XMLElement& source, const Vector3& position, const Quaternion& rotation, CreateMode mode)
{
    PROFILE(InstantiateXML);
    
    SceneResolver resolver;
    unsigned nodeID = source.GetInt("id");
    // Rewrite IDs when instantiating
    Node* node = CreateChild(0, mode);
    resolver.AddNode(nodeID, node);
    if (node->LoadXML(source, resolver, true, true, mode))
    {
        resolver.Resolve();
        node->ApplyAttributes();
        node->SetTransform(position, rotation);
        return node;
    }
    else
    {
        node->Remove();
        return 0;
    }
}

Node* Scene::InstantiateXML(Deserializer& source, const Vector3& position, const Quaternion& rotation, CreateMode mode)
{
    SharedPtr<XMLFile> xml(new XMLFile(context_));
    if (!xml->Load(source))
        return 0;
    
    return InstantiateXML(xml->GetRoot(), position, rotation, mode);
}

void Scene::Clear()
{
    StopAsyncLoading();
    RemoveAllChildren();
    RemoveAllComponents();
    fileName_.Clear();
    checksum_ = 0;
    replicatedNodeID_ = FIRST_REPLICATED_ID;
    replicatedComponentID_ = FIRST_REPLICATED_ID;
    localNodeID_ = FIRST_LOCAL_ID;
    localComponentID_ = FIRST_LOCAL_ID;
}

void Scene::SetActive(bool enable)
{
    active_ = enable;
}

void Scene::SetTimeScale(float scale)
{
    timeScale_ = Max(scale, M_EPSILON);
    Node::MarkNetworkUpdate();
}

void Scene::SetSmoothingConstant(float constant)
{
    smoothingConstant_ = Max(constant, M_EPSILON);
    Node::MarkNetworkUpdate();
}

void Scene::SetSnapThreshold(float threshold)
{
    snapThreshold_ = Max(threshold, 0.0f);
    Node::MarkNetworkUpdate();
}

void Scene::SetElapsedTime(float time)
{
    elapsedTime_ = time;
}

void Scene::AddRequiredPackageFile(PackageFile* package)
{
    // Do not add packages that failed to load
    if (!package || !package->GetNumFiles())
        return;
    
    requiredPackageFiles_.Push(SharedPtr<PackageFile>(package));
}

void Scene::ClearRequiredPackageFiles()
{
    requiredPackageFiles_.Clear();
}

void Scene::RegisterVar(const String& name)
{
    varNames_[ShortStringHash(name)] = name;
}

void Scene::UnregisterVar(const String& name)
{
    varNames_.Erase(ShortStringHash(name));
}

void Scene::UnregisterAllVars()
{
    varNames_.Clear();
}

Node* Scene::GetNode(unsigned id) const
{
    if (id < FIRST_LOCAL_ID)
    {
        HashMap<unsigned, Node*>::ConstIterator i = replicatedNodes_.Find(id);
        if (i != replicatedNodes_.End())
            return i->second_;
        else
            return 0;
    }
    else
    {
        HashMap<unsigned, Node*>::ConstIterator i = localNodes_.Find(id);
        if (i != localNodes_.End())
            return i->second_;
        else
            return 0;
    }
}

Component* Scene::GetComponent(unsigned id) const
{
    if (id < FIRST_LOCAL_ID)
    {
        HashMap<unsigned, Component*>::ConstIterator i = replicatedComponents_.Find(id);
        if (i != replicatedComponents_.End())
            return i->second_;
        else
            return 0;
    }
    else
    {
        HashMap<unsigned, Component*>::ConstIterator i = localComponents_.Find(id);
        if (i != localComponents_.End())
            return i->second_;
        else
            return 0;
    }
}

float Scene::GetAsyncProgress() const
{
    if (!asyncLoading_ || !asyncProgress_.totalNodes_)
        return 1.0f;
    else
        return (float)asyncProgress_.loadedNodes_ / (float)asyncProgress_.totalNodes_;
}

const String& Scene::GetVarName(ShortStringHash hash) const
{
    HashMap<ShortStringHash, String>::ConstIterator i = varNames_.Find(hash);
    return i != varNames_.End() ? i->second_ : String::EMPTY;
}

void Scene::Update(float timeStep)
{
    if (asyncLoading_)
    {
        UpdateAsyncLoading();
        return;
    }
    
    PROFILE(UpdateScene);
    
    timeStep *= timeScale_;
    
    using namespace SceneUpdate;
    
    VariantMap eventData;
    eventData[P_SCENE] = (void*)this;
    eventData[P_TIMESTEP] = timeStep;
    
    // Update variable timestep logic
    SendEvent(E_SCENEUPDATE, eventData);
    
    // Update scene subsystems. If a physics world is present, it will be updated, triggering fixed timestep logic updates
    SendEvent(E_SCENESUBSYSTEMUPDATE, eventData);
    
    // Update transform smoothing
    {
        PROFILE(UpdateSmoothing);
        
        float constant = 1.0f - Clamp(powf(2.0f, -timeStep * smoothingConstant_), 0.0f, 1.0f);
        float squaredSnapThreshold = snapThreshold_ * snapThreshold_;
        
        using namespace UpdateSmoothing;
        
        VariantMap eventData;
        eventData[P_CONSTANT] = constant;
        eventData[P_SQUAREDSNAPTHRESHOLD] = squaredSnapThreshold;
        SendEvent(E_UPDATESMOOTHING, eventData);
    }
    
    // Post-update variable timestep logic
    SendEvent(E_SCENEPOSTUPDATE, eventData);
    
    // Note: using a float for elapsed time accumulation is inherently inaccurate. The purpose of this value is
    // primarily to update material animation effects, as it is available to shaders. It can be reset by calling
    // SetElapsedTime()
    elapsedTime_ += timeStep;
}

void Scene::BeginThreadedUpdate()
{
    // Check the work queue subsystem whether it actually has created worker threads. If not, do not enter threaded mode.
    if (GetSubsystem<WorkQueue>()->GetNumThreads())
        threadedUpdate_ = true;
}

void Scene::EndThreadedUpdate()
{
    if (!threadedUpdate_)
        return;
    
    threadedUpdate_ = false;
    
    if (!delayedDirtyComponents_.Empty())
    {
        PROFILE(EndThreadedUpdate);
        
        for (PODVector<Component*>::ConstIterator i = delayedDirtyComponents_.Begin(); i != delayedDirtyComponents_.End(); ++i)
            (*i)->OnMarkedDirty((*i)->GetNode());
        delayedDirtyComponents_.Clear();
    }
}

void Scene::DelayedMarkedDirty(Component* component)
{
    MutexLock lock(sceneMutex_);
    delayedDirtyComponents_.Push(component);
}

unsigned Scene::GetFreeNodeID(CreateMode mode)
{
    if (mode == REPLICATED)
    {
        for (;;)
        {
            if (!replicatedNodes_.Contains(replicatedNodeID_))
                return replicatedNodeID_;
            
            if (replicatedNodeID_ != LAST_REPLICATED_ID)
                ++replicatedNodeID_;
            else
                replicatedNodeID_ = FIRST_REPLICATED_ID;
        }
    }
    else
    {
        for (;;)
        {
            if (!localNodes_.Contains(localNodeID_))
                return localNodeID_;
            
            if (localNodeID_ != LAST_LOCAL_ID)
                ++localNodeID_;
            else
                localNodeID_ = FIRST_LOCAL_ID;
        }
    }
}

unsigned Scene::GetFreeComponentID(CreateMode mode)
{
    if (mode == REPLICATED)
    {
        for (;;)
        {
            if (!replicatedComponents_.Contains(replicatedComponentID_))
                return replicatedComponentID_;
            
            if (replicatedComponentID_ != LAST_REPLICATED_ID)
                ++replicatedComponentID_;
            else
                replicatedComponentID_ = FIRST_REPLICATED_ID;
        }
    }
    else
    {
        for (;;)
        {
            if (!localComponents_.Contains(localComponentID_))
                return localComponentID_;
            
            if (localComponentID_ != LAST_LOCAL_ID)
                ++localComponentID_;
            else
                localComponentID_ = FIRST_LOCAL_ID;
        }
    }
}

void Scene::NodeAdded(Node* node)
{
    if (!node || node->GetScene())
        return;
    
    node->SetScene(this);
    
    // If we already have an existing node with the same ID, must remove the scene reference from it
    unsigned id = node->GetID();
    if (id < FIRST_LOCAL_ID)
    {
        HashMap<unsigned, Node*>::Iterator i = replicatedNodes_.Find(id);
        if (i != replicatedNodes_.End() && i->second_ != node)
        {
            LOGWARNING("Overwriting node with ID " + String(id));
            i->second_->ResetScene();
        }
        
        replicatedNodes_[id] = node;
        
        MarkNetworkUpdate(node);
        MarkReplicationDirty(node);
    }
    else
    {
        HashMap<unsigned, Node*>::Iterator i = localNodes_.Find(id);
        if (i != localNodes_.End() && i->second_ != node)
        {
            LOGWARNING("Overwriting node with ID " + String(id));
            i->second_->ResetScene();
        }
        
        localNodes_[id] = node;
    }
}

void Scene::NodeRemoved(Node* node)
{
    if (!node || node->GetScene() != this)
        return;
    
    unsigned id = node->GetID();
    if (id < FIRST_LOCAL_ID)
    {
        replicatedNodes_.Erase(id);
        MarkReplicationDirty(node);
    }
    else
        localNodes_.Erase(id);
    
    node->SetID(0);
    node->SetScene(0);
}

void Scene::ComponentAdded(Component* component)
{
    if (!component)
        return;
    
    unsigned id = component->GetID();
    if (id < FIRST_LOCAL_ID)
    {
        HashMap<unsigned, Component*>::Iterator i = replicatedComponents_.Find(id);
        if (i != replicatedComponents_.End() && i->second_ != component)
        {
            LOGWARNING("Overwriting component with ID " + String(id));
            i->second_->SetID(0);
        }
        
        replicatedComponents_[id] = component;
    }
    else
    {
        HashMap<unsigned, Component*>::Iterator i = localComponents_.Find(id);
        if (i != localComponents_.End() && i->second_ != component)
        {
            LOGWARNING("Overwriting component with ID " + String(id));
            i->second_->SetID(0);
        }
        
        localComponents_[id] = component;
    }
}

void Scene::ComponentRemoved(Component* component)
{
    if (!component)
        return;
    
    unsigned id = component->GetID();
    if (id < FIRST_LOCAL_ID)
        replicatedComponents_.Erase(id);
    else
        localComponents_.Erase(id);
    
    component->SetID(0);
}

void Scene::SetVarNamesAttr(String value)
{
    Vector<String> varNames = value.Split(';');
    
    varNames_.Clear();
    for (Vector<String>::ConstIterator i = varNames.Begin(); i != varNames.End(); ++i)
        varNames_[ShortStringHash(*i)] = *i;
}

String Scene::GetVarNamesAttr() const
{
    String ret;
    
    if (!varNames_.Empty())
    {
        for (HashMap<ShortStringHash, String>::ConstIterator i = varNames_.Begin(); i != varNames_.End(); ++i)
            ret += i->second_ + ';';
        
        ret.Resize(ret.Length() - 1);
    }
    
    return ret;
}

void Scene::PrepareNetworkUpdate()
{
    for (HashSet<unsigned>::Iterator i = networkUpdateNodes_.Begin(); i != networkUpdateNodes_.End(); ++i)
    {
        Node* node = GetNode(*i);
        if (node)
            node->PrepareNetworkUpdate();
    }
    
    for (HashSet<unsigned>::Iterator i = networkUpdateComponents_.Begin(); i != networkUpdateComponents_.End(); ++i)
    {
        Component* component = GetComponent(*i);
        if (component)
            component->PrepareNetworkUpdate();
    }
    
    networkUpdateNodes_.Clear();
    networkUpdateComponents_.Clear();
}

void Scene::CleanupConnection(Connection* connection)
{
    Node::CleanupConnection(connection);
    
    for (HashMap<unsigned, Node*>::Iterator i = replicatedNodes_.Begin(); i != replicatedNodes_.End(); ++i)
        i->second_->CleanupConnection(connection);
    
    for (HashMap<unsigned, Component*>::Iterator i = replicatedComponents_.Begin(); i != replicatedComponents_.End(); ++i)
        i->second_->CleanupConnection(connection);
}

void Scene::MarkNetworkUpdate(Node* node)
{
    if (node)
        networkUpdateNodes_.Insert(node->GetID());
}

void Scene::MarkNetworkUpdate(Component* component)
{
    if (component)
        networkUpdateComponents_.Insert(component->GetID());
}

void Scene::MarkReplicationDirty(Node* node)
{
    unsigned id = node->GetID();
    
    if (id < FIRST_LOCAL_ID && networkState_)
    {
        for (PODVector<ReplicationState*>::Iterator i = networkState_->replicationStates_.Begin(); i !=
            networkState_->replicationStates_.End(); ++i)
        {
            NodeReplicationState* nodeState = static_cast<NodeReplicationState*>(*i);
            nodeState->sceneState_->dirtyNodes_.Insert(id);
        }
    }
}

void Scene::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;
    
    if (active_)
        Update(eventData[P_TIMESTEP].GetFloat());
}

void Scene::UpdateAsyncLoading()
{
    PROFILE(UpdateAsyncLoading);
    
    Timer asyncLoadTimer;
    
    for (;;)
    {
        if (asyncProgress_.loadedNodes_ >= asyncProgress_.totalNodes_)
        {
            FinishAsyncLoading();
            return;
        }
        
        // Read one child node with its full sub-hierarchy either from binary or XML
        if (!asyncProgress_.xmlFile_)
        {
            unsigned nodeID = asyncProgress_.file_->ReadUInt();
            Node* newNode = CreateChild(nodeID, nodeID < FIRST_LOCAL_ID ? REPLICATED : LOCAL);
            resolver_.AddNode(nodeID, newNode);
            newNode->Load(*asyncProgress_.file_, resolver_);
        }
        else
        {
            unsigned nodeID = asyncProgress_.xmlElement_.GetInt("id");
            Node* newNode = CreateChild(nodeID, nodeID < FIRST_LOCAL_ID ? REPLICATED : LOCAL);
            resolver_.AddNode(nodeID, newNode);
            newNode->LoadXML(asyncProgress_.xmlElement_, resolver_);
            asyncProgress_.xmlElement_ = asyncProgress_.xmlElement_.GetNext("node");
        }
        
        ++asyncProgress_.loadedNodes_;
        
        // Break if time limit exceeded, so that we keep sufficient FPS
        if (asyncLoadTimer.GetMSec(false) >= ASYNC_LOAD_MAX_MSEC)
            break;
    }
    
    using namespace AsyncLoadProgress;
    
    VariantMap eventData;
    eventData[P_SCENE] = (void*)this;
    eventData[P_PROGRESS] = (float)asyncProgress_.loadedNodes_ / (float)asyncProgress_.totalNodes_;
    eventData[P_LOADEDNODES]  = asyncProgress_.loadedNodes_;
    eventData[P_TOTALNODES]  = asyncProgress_.totalNodes_;
    SendEvent(E_ASYNCLOADPROGRESS, eventData);
}

void Scene::FinishAsyncLoading()
{
    resolver_.Resolve();
    ApplyAttributes();
    FinishLoading(asyncProgress_.file_);
    StopAsyncLoading();
    
    using namespace AsyncLoadFinished;
    
    VariantMap eventData;
    eventData[P_SCENE] = (void*)this;
    SendEvent(E_ASYNCLOADFINISHED, eventData);
}

void Scene::FinishLoading(Deserializer* source)
{
    if (source)
    {
        fileName_ = source->GetName();
        checksum_ = source->GetChecksum();
    }
}

void RegisterSceneLibrary(Context* context)
{
    Node::RegisterObject(context);
    Scene::RegisterObject(context);
    SmoothedTransform::RegisterObject(context);
}

}
