//
// Urho3D Engine
// Copyright (c) 2008-2011 Lasse ��rni
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
#include "APITemplates.h"
#include "Scene.h"

static void RegisterSerializable(asIScriptEngine* engine)
{
    engine->RegisterGlobalProperty("const uint AM_SERIALIZATION", (void*)AM_SERIALIZATION);
    engine->RegisterGlobalProperty("const uint AM_NETWORK", (void*)AM_NETWORK);
    engine->RegisterGlobalProperty("const uint AM_BOTH", (void*)AM_BOTH);
    
    RegisterSerializable<Serializable>(engine, "Serializable");
}

static void RegisterNode(asIScriptEngine* engine)
{
    // Register Component first. At this point Node is not yet registered, so can not register GetNode for Component
    RegisterComponent<Component>(engine, "Component", false);
    RegisterNode<Node>(engine, "Node");
    RegisterObjectConstructor<Node>(engine, "Node");
    RegisterNamedObjectConstructor<Node>(engine, "Node");
    engine->RegisterGlobalFunction("Node@+ get_node()", asFUNCTION(GetScriptContextNode), asCALL_CDECL);
    
    // Now GetNode can be registered
    engine->RegisterObjectMethod("Component", "Node@+ get_node() const", asMETHOD(Component, GetNode), asCALL_THISCALL);
    
    // Register Variant GetPtr() for Node & Component
    engine->RegisterObjectMethod("Variant", "Node@+ GetNode() const", asFUNCTION(GetVariantPtr<Node>), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Variant", "Component@+ GetComponent() const", asFUNCTION(GetVariantPtr<Component>), asCALL_CDECL_OBJLAST);
}

static bool SceneLoadXML(File* file, Scene* ptr)
{
    if (file)
        return ptr->LoadXML(*file);
    else
        return false;
}

static bool SceneSaveXML(File* file, Scene* ptr)
{
    if (file)
        return ptr->SaveXML(*file);
    else
        return false;
}

static void RegisterScene(asIScriptEngine* engine)
{
    RegisterNode<Scene>(engine, "Scene");
    RegisterObjectConstructor<Scene>(engine, "Scene");
    RegisterNamedObjectConstructor<Scene>(engine, "Scene");
    engine->RegisterObjectMethod("Scene", "bool LoadXML(File@+)", asFUNCTION(SceneLoadXML), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Scene", "bool SaveXML(File@+)", asFUNCTION(SceneSaveXML), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("Scene", "Component@+ GetComponentByID(uint)", asMETHOD(Scene, GetComponentByID), asCALL_THISCALL);
    engine->RegisterObjectMethod("Scene", "Node@+ GetNodeByID(uint)", asMETHOD(Scene, GetNodeByID), asCALL_THISCALL);
    engine->RegisterObjectMethod("Scene", "void Update(float)", asMETHOD(Scene, Update), asCALL_THISCALL);
    engine->RegisterObjectMethod("Scene", "void set_active(bool)", asMETHOD(Scene, SetActive), asCALL_THISCALL);
    engine->RegisterObjectMethod("Scene", "bool get_active() const", asMETHOD(Scene, IsActive), asCALL_THISCALL);
    engine->RegisterObjectMethod("Node", "Scene@+ get_scene() const", asMETHOD(Node, GetScene), asCALL_THISCALL);
    engine->RegisterGlobalFunction("Scene@+ get_scene()", asFUNCTION(GetScriptContextScene), asCALL_CDECL);
    
    // Register Variant GetPtr() for Scene
    engine->RegisterObjectMethod("Variant", "Scene@+ GetScene() const", asFUNCTION(GetVariantPtr<Scene>), asCALL_CDECL_OBJLAST);
}

void RegisterSceneAPI(asIScriptEngine* engine)
{
    RegisterSerializable(engine);
    RegisterNode(engine);
    RegisterScene(engine);
}