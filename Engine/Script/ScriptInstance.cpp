//
// Urho3D Engine
// Copyright (c) 2008-2012 Lasse ��rni
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
#include "Context.h"
#include "Log.h"
#include "MemoryBuffer.h"
#include "PhysicsEvents.h"
#include "PhysicsWorld.h"
#include "ResourceCache.h"
#include "ResourceEvents.h"
#include "Scene.h"
#include "SceneEvents.h"
#include "Script.h"
#include "ScriptFile.h"
#include "ScriptInstance.h"

#include <angelscript.h>

#include "DebugNew.h"

static const String methodDeclarations[] = {
    "void Start()",
    "void Stop()",
    "void Update(float)",
    "void PostUpdate(float)",
    "void FixedUpdate(float)",
    "void FixedPostUpdate(float)",
    "void Load(Deserializer&)",
    "void Save(Serializer&)",
    "void ApplyAttributes()"
};

OBJECTTYPESTATIC(ScriptInstance);

ScriptInstance::ScriptInstance(Context* context) :
    Component(context),
    script_(GetSubsystem<Script>()),
    scriptObject_(0),
    active_(true),
    fixedUpdateFps_(0),
    fixedUpdateInterval_(0.0f),
    fixedUpdateAcc_(0.0f),
    fixedPostUpdateAcc_(0.0f)
{
    ClearMethods();
}

ScriptInstance::~ScriptInstance()
{
    ReleaseObject();
}

void ScriptInstance::RegisterObject(Context* context)
{
    context->RegisterFactory<ScriptInstance>();
    
    ACCESSOR_ATTRIBUTE(ScriptInstance, VAR_RESOURCEREF, "Script File", GetScriptFileAttr, SetScriptFileAttr, ResourceRef, ResourceRef(ScriptFile::GetTypeStatic()), AM_DEFAULT);
    REF_ACCESSOR_ATTRIBUTE(ScriptInstance, VAR_STRING, "Class Name", GetClassName, SetClassName, String, String(), AM_DEFAULT);
    ATTRIBUTE(ScriptInstance, VAR_BOOL, "Is Active", active_, true, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(ScriptInstance, VAR_INT, "Fixed Update FPS", GetFixedUpdateFps, SetFixedUpdateFps, int, 0, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(ScriptInstance, VAR_FLOAT, "Time Accumulator", GetFixedUpdateAccAttr, SetFixedUpdateAccAttr, float, 0.0f, AM_FILE);
    ACCESSOR_ATTRIBUTE(ScriptInstance, VAR_BUFFER, "Delayed Method Calls", GetDelayedMethodCallsAttr, SetDelayedMethodCallsAttr, PODVector<unsigned char>, PODVector<unsigned char>(), AM_FILE | AM_NOEDIT);
    ACCESSOR_ATTRIBUTE(ScriptInstance, VAR_BUFFER, "Script Data", GetScriptDataAttr, SetScriptDataAttr, PODVector<unsigned char>, PODVector<unsigned char>(), AM_DEFAULT | AM_NOEDIT);
}

void ScriptInstance::ApplyAttributes()
{
    if (scriptObject_ && methods_[METHOD_FINISHUPDATE])
        scriptFile_->Execute(scriptObject_, methods_[METHOD_FINISHUPDATE]);
}

bool ScriptInstance::CreateObject(ScriptFile* scriptFile, const String& className)
{
    className_ = String(); // Do not create object during SetScriptFile()
    SetScriptFile(scriptFile);
    SetClassName(className);
    return scriptObject_ != 0;
}

void ScriptInstance::SetScriptFile(ScriptFile* scriptFile)
{
    if (scriptFile == scriptFile_ && scriptObject_)
        return;
    
    ReleaseObject();
    
    // Unsubscribe from the reload event of previous script file (if any), then subscribe to the new
    if (scriptFile_)
    {
        UnsubscribeFromEvent(scriptFile_, E_RELOADSTARTED);
        UnsubscribeFromEvent(scriptFile_, E_RELOADFINISHED);
    }
    if (scriptFile)
    {
        SubscribeToEvent(scriptFile, E_RELOADSTARTED, HANDLER(ScriptInstance, HandleScriptFileReload));
        SubscribeToEvent(scriptFile, E_RELOADFINISHED, HANDLER(ScriptInstance, HandleScriptFileReloadFinished));
    }
    
    scriptFile_ = scriptFile;
    
    CreateObject();
}

void ScriptInstance::SetClassName(const String& className)
{
    if (className == className_ && scriptObject_)
        return;
    
    ReleaseObject();
    className_ = className;
    CreateObject();
}

void ScriptInstance::SetActive(bool active)
{
    active_ = active;
}

void ScriptInstance::SetFixedUpdateFps(int fps)
{
    fixedUpdateFps_ = Max(fps, 0);
    fixedUpdateInterval_ = fixedUpdateFps_ ? (1.0f / fixedUpdateFps_) : 0.0f;
    fixedUpdateAcc_ = 0.0f;
    fixedPostUpdateAcc_ = 0.0f;
}

bool ScriptInstance::Execute(const String& declaration, const VariantVector& parameters)
{
    if (!scriptObject_)
        return false;
    
    asIScriptFunction* method = scriptFile_->GetMethod(scriptObject_, declaration);
    return scriptFile_->Execute(scriptObject_, method, parameters);
}

bool ScriptInstance::Execute(asIScriptFunction* method, const VariantVector& parameters)
{
    if (!method || !scriptObject_)
        return false;
    
    return scriptFile_->Execute(scriptObject_, method, parameters);
}

void ScriptInstance::DelayedExecute(float delay, const String& declaration, const VariantVector& parameters)
{
    if (!scriptObject_)
        return;
    
    DelayedMethodCall call;
    call.delay_ = Max(delay, 0.0f);
    call.declaration_ = declaration;
    call.parameters_ = parameters;
    delayedMethodCalls_.Push(call);
    
    // Make sure we are registered to the scene update event, because delayed calls are executed there
    if (!methods_[METHOD_UPDATE] && !HasSubscribedToEvent(E_SCENEUPDATE))
    {
        Node* node = GetNode();
        if (node)
            SubscribeToEvent(node->GetScene(), E_SCENEUPDATE, HANDLER(ScriptInstance, HandleSceneUpdate));
    }
}

void ScriptInstance::ClearDelayedExecute()
{
    delayedMethodCalls_.Clear();
}

void ScriptInstance::AddEventHandler(StringHash eventType, const String& handlerName)
{
    if (!scriptObject_)
        return;
    
    String declaration = "void " + handlerName + "(StringHash, VariantMap&)";
    asIScriptFunction* method = scriptFile_->GetMethod(scriptObject_, declaration);
    if (!method)
    {
        declaration = "void " + handlerName + "()";
        method = scriptFile_->GetMethod(scriptObject_, declaration);
        if (!method)
        {
            LOGERROR("Event handler method " + handlerName + " not found in " + scriptFile_->GetName());
            return;
        }
    }
    
    SubscribeToEvent(eventType, HANDLER_USERDATA(ScriptInstance, HandleScriptEvent, (void*)method));
}

void ScriptInstance::AddEventHandler(Object* sender, StringHash eventType, const String& handlerName)
{
    if (!scriptObject_)
        return;

    if (!sender)
    {
        LOGERROR("Null event sender for event " + String(eventType) + ", handler " + handlerName);
        return;
    }
    
    String declaration = "void " + handlerName + "(StringHash, VariantMap&)";
    asIScriptFunction* method = scriptFile_->GetMethod(scriptObject_, declaration);
    if (!method)
    {
        declaration = "void " + handlerName + "()";
        method = scriptFile_->GetMethod(scriptObject_, declaration);
        if (!method)
        {
            LOGERROR("Event handler method " + handlerName + " not found in " + scriptFile_->GetName());
            return;
        }
    }
    
    SubscribeToEvent(sender, eventType, HANDLER_USERDATA(ScriptInstance, HandleScriptEvent, (void*)method));
}

void ScriptInstance::SetScriptFileAttr(ResourceRef value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    SetScriptFile(cache->GetResource<ScriptFile>(value.id_));
}

void ScriptInstance::SetDelayedMethodCallsAttr(PODVector<unsigned char> value)
{
    MemoryBuffer buf(value);
    delayedMethodCalls_.Resize(buf.ReadVLE());
    for (Vector<DelayedMethodCall>::Iterator i = delayedMethodCalls_.Begin(); i != delayedMethodCalls_.End(); ++i)
    {
        i->delay_ = buf.ReadFloat();
        i->declaration_ = buf.ReadString();
        i->parameters_ = buf.ReadVariantVector();
    }
}

void ScriptInstance::SetFixedUpdateAccAttr(float value)
{
    fixedUpdateAcc_ = value;
    fixedPostUpdateAcc_ = value;
}

void ScriptInstance::SetScriptDataAttr(PODVector<unsigned char> data)
{
    if (scriptObject_ && methods_[METHOD_LOAD])
    {
        MemoryBuffer buf(data);
        VariantVector parameters;
        parameters.Push(Variant((void*)static_cast<Deserializer*>(&buf)));
        scriptFile_->Execute(scriptObject_, methods_[METHOD_LOAD], parameters);
    }
}

ResourceRef ScriptInstance::GetScriptFileAttr() const
{
    return GetResourceRef(scriptFile_, ScriptFile::GetTypeStatic());
}

PODVector<unsigned char> ScriptInstance::GetDelayedMethodCallsAttr() const
{
    VectorBuffer buf;
    buf.WriteVLE(delayedMethodCalls_.Size());
    for (Vector<DelayedMethodCall>::ConstIterator i = delayedMethodCalls_.Begin(); i != delayedMethodCalls_.End(); ++i)
    {
        buf.WriteFloat(i->delay_);
        buf.WriteString(i->declaration_);
        buf.WriteVariantVector(i->parameters_);
    }
    return buf.GetBuffer();
}

float ScriptInstance::GetFixedUpdateAccAttr() const
{
    return fixedUpdateAcc_;
}

PODVector<unsigned char> ScriptInstance::GetScriptDataAttr() const
{
    if (!scriptObject_ || !methods_[METHOD_SAVE])
        return PODVector<unsigned char>();
    else
    {
        VectorBuffer buf;
        VariantVector parameters;
        parameters.Push(Variant((void*)static_cast<Serializer*>(&buf)));
        scriptFile_->Execute(scriptObject_, methods_[METHOD_SAVE], parameters);
        return buf.GetBuffer();
    }
}

void ScriptInstance::CreateObject()
{
    if (!scriptFile_ || className_.Empty())
        return;
    
    scriptObject_ = scriptFile_->CreateObject(className_);
    if (scriptObject_)
    {
        script_->GetObjectMap()[(void*)scriptObject_] = this;
        ClearDelayedExecute();
        GetSupportedMethods();
        if (methods_[METHOD_START])
            scriptFile_->Execute(scriptObject_, methods_[METHOD_START]);
    }
    else
        LOGERROR("Failed to create object of class " + className_ + " from " + scriptFile_->GetName());
}

void ScriptInstance::ReleaseObject()
{
    if (scriptObject_)
    {
        if (methods_[METHOD_STOP])
            scriptFile_->Execute(scriptObject_, methods_[METHOD_STOP]);
        
        UnsubscribeFromAllEvents();
        ClearMethods();
        ClearDelayedExecute();
        
        scriptObject_->Release();
        scriptObject_ = 0;
        
        script_->GetObjectMap().Erase((void*)scriptObject_);
    }
}

void ScriptInstance::ClearMethods()
{
    for (unsigned i = 0; i < MAX_SCRIPT_METHODS; ++i)
        methods_[i] = 0;
    
    delayedMethodCalls_.Clear();
}

void ScriptInstance::GetSupportedMethods()
{
    for (unsigned i = 0; i < MAX_SCRIPT_METHODS; ++i)
        methods_[i] = scriptFile_->GetMethod(scriptObject_, methodDeclarations[i]);
    
    // Subscribe to the update events as supported
    Node* node = GetNode();
    if (node)
    {
        Scene* scene = node->GetScene();
        if (scene)
        {
            if (methods_[METHOD_UPDATE])
                SubscribeToEvent(scene, E_SCENEUPDATE, HANDLER(ScriptInstance, HandleSceneUpdate));
            if (methods_[METHOD_POSTUPDATE])
                SubscribeToEvent(scene, E_SCENEPOSTUPDATE, HANDLER(ScriptInstance, HandleScenePostUpdate));
            
            PhysicsWorld* world = scene->GetComponent<PhysicsWorld>();
            if (world)
            {
                if (methods_[METHOD_FIXEDUPDATE])
                    SubscribeToEvent(world, E_PHYSICSPRESTEP, HANDLER(ScriptInstance, HandlePhysicsPreStep));
                if (methods_[METHOD_FIXEDPOSTUPDATE])
                    SubscribeToEvent(world, E_PHYSICSPOSTSTEP, HANDLER(ScriptInstance, HandlePhysicsPostStep));
            }
        }
    }
}

void ScriptInstance::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!active_ || !scriptObject_)
        return;
    
    using namespace SceneUpdate;
    
    float timeStep = eventData[P_TIMESTEP].GetFloat();
    
    // Execute delayed method calls
    for (Vector<DelayedMethodCall>::Iterator i = delayedMethodCalls_.Begin(); i != delayedMethodCalls_.End();)
    {
        i->delay_ -= timeStep;
        if (i->delay_ <= 0.0f)
        {
            Execute(i->declaration_, i->parameters_);
            i = delayedMethodCalls_.Erase(i);
        }
        else
            ++i;
    }
    
    if (methods_[METHOD_UPDATE])
    {
        VariantVector parameters;
        parameters.Push(timeStep);
        scriptFile_->Execute(scriptObject_, methods_[METHOD_UPDATE], parameters);
    }
}

void ScriptInstance::HandleScenePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!active_ || !scriptObject_)
        return;
    
    using namespace ScenePostUpdate;
    
    VariantVector parameters;
    parameters.Push(eventData[P_TIMESTEP]);
    scriptFile_->Execute(scriptObject_, methods_[METHOD_POSTUPDATE], parameters);
}

void ScriptInstance::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
    if (!active_ || !scriptObject_)
        return;
    
    using namespace PhysicsPreStep;
    
    if (!fixedUpdateFps_)
    {
        VariantVector parameters;
        parameters.Push(eventData[P_TIMESTEP]);
        scriptFile_->Execute(scriptObject_, methods_[METHOD_FIXEDUPDATE], parameters);
    }
    else
    {
        float timeStep = eventData[P_TIMESTEP].GetFloat();
        fixedUpdateAcc_ += timeStep;
        if (fixedUpdateAcc_ >= fixedUpdateInterval_)
        {
            fixedUpdateAcc_ = fmodf(fixedUpdateAcc_, fixedUpdateInterval_);
            VariantVector parameters;
            parameters.Push(fixedUpdateInterval_);
            scriptFile_->Execute(scriptObject_, methods_[METHOD_FIXEDUPDATE], parameters);
        }
    }
}

void ScriptInstance::HandlePhysicsPostStep(StringHash eventType, VariantMap& eventData)
{
    if (!active_ || !scriptObject_)
        return;
    
    using namespace PhysicsPostStep;
    
    if (!fixedUpdateFps_)
    {
        VariantVector parameters;
        parameters.Push(eventData[P_TIMESTEP]);
        scriptFile_->Execute(scriptObject_, methods_[METHOD_FIXEDPOSTUPDATE], parameters);
    }
    else
    {
        float timeStep = eventData[P_TIMESTEP].GetFloat();
        fixedPostUpdateAcc_ += timeStep;
        if (fixedPostUpdateAcc_ >= fixedUpdateInterval_)
        {
            fixedPostUpdateAcc_ = fmodf(fixedPostUpdateAcc_, fixedUpdateInterval_);
            VariantVector parameters;
            parameters.Push(fixedUpdateInterval_);
            scriptFile_->Execute(scriptObject_, methods_[METHOD_FIXEDPOSTUPDATE], parameters);
        }
    }
}

void ScriptInstance::HandleScriptEvent(StringHash eventType, VariantMap& eventData)
{
    if (!active_ || !scriptFile_ || !scriptObject_)
        return;
    
    asIScriptFunction* method = static_cast<asIScriptFunction*>(context_->GetEventHandler()->GetUserData());
    
    VariantVector parameters;
    if (method->GetParamCount() > 0)
    {
        parameters.Push(Variant((void*)&eventType));
        parameters.Push(Variant((void*)&eventData));
    }
    
    scriptFile_->Execute(scriptObject_, method, parameters);
}

Context* GetScriptContext()
{
    return static_cast<Script*>(asGetActiveContext()->GetEngine()->GetUserData())->GetContext();
}

ScriptInstance* GetScriptContextInstance()
{
    asIScriptContext* context = asGetActiveContext();
    void* object = context->GetThisPointer();
    Map<void*, ScriptInstance*>& objectMap = static_cast<Script*>(context->GetEngine()->GetUserData())->GetObjectMap();
    Map<void*, ScriptInstance*>::ConstIterator i = objectMap.Find(object);
    if (i != objectMap.End())
        return i->second_;
    else
        return 0;
}

Node* GetScriptContextNode()
{
    ScriptInstance* instance = GetScriptContextInstance();
    return instance ? instance->GetNode() : 0;
}

Scene* GetScriptContextScene()
{
    Scene* scene = 0;
    Node* node = GetScriptContextNode();
    if (node)
        scene = node->GetScene();
    // If null, try to get the default scene
    if (!scene)
        scene = GetScriptContext()->GetSubsystem<Script>()->GetDefaultScene();
    
    return scene;
}

ScriptEventListener* GetScriptContextEventListener()
{
    // First try to get the script instance. If not found, get the script file for procedural event handling
    ScriptInstance* instance = GetScriptContextInstance();
    if (instance)
        return instance;
    ScriptFile* file = GetScriptContextFile();
    return file;
}

Object* GetScriptContextEventListenerObject()
{
    ScriptInstance* instance = GetScriptContextInstance();
    if (instance)
        return instance;
    ScriptFile* file = GetScriptContextFile();
    return file;
}

void ScriptInstance::HandleScriptFileReload(StringHash eventType, VariantMap& eventData)
{
    ReleaseObject();
}

void ScriptInstance::HandleScriptFileReloadFinished(StringHash eventType, VariantMap& eventData)
{
    if (!className_.Empty())
        CreateObject();
}
