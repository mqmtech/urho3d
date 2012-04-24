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

#pragma once

#include "Model.h"
#include "Skeleton.h"
#include "StaticModel.h"

class Animation;
class AnimationState;
class DebugRenderer;

/// Animated model component.
class AnimatedModel : public StaticModel
{
    OBJECT(AnimatedModel);
    
    friend class AnimationState;
    
public:
    /// Construct.
    AnimatedModel(Context* context);
    /// Destruct.
    virtual ~AnimatedModel();
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);
    
    /// Load from binary data. Return true if successful.
    virtual bool Load(Deserializer& source);
    /// Load from XML data. Return true if successful.
    virtual bool LoadXML(const XMLElement& source);
    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    virtual void ApplyAttributes();
    /// Process octree raycast. May be called from a worker thread.
    virtual void ProcessRayQuery(const RayOctreeQuery& query, PODVector<RayQueryResult>& results);
    /// Update before octree reinsertion. Animation is updated here.
    virtual void Update(const FrameInfo& frame);
    /// Calculate distance and LOD level for rendering. May be called from worker thread(s), possibly re-entrantly.
    virtual void UpdateDistance(const FrameInfo& frame);
    /// Prepare geometry for rendering. Called from a worker thread if possible (no GPU update.)
    virtual void UpdateGeometry(const FrameInfo& frame);
    /// Return whether a geometry update is necessary, and if it should happen in a worker thread.
    virtual UpdateGeometryType GetUpdateGeometryType();
    /// Fill rendering batch with distance, geometry, material and world transform.
    virtual void GetBatch(Batch& batch, const FrameInfo& frame, unsigned batchIndex);
    /// Add debug geometry to the debug renderer.
    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);
    
    /// %Set model.
    void SetModel(Model* model, bool createBones = true);
    /// Add an animation.
    AnimationState* AddAnimationState(Animation* animation);
    /// Remove an animation by animation pointer.
    void RemoveAnimationState(Animation* animation);
    /// Remove an animation by animation name.
    void RemoveAnimationState(const String& animationName);
    /// Remove an animation by animation name hash.
    void RemoveAnimationState(StringHash animationNameHash);
    /// Remove an animation by AnimationState pointer.
    void RemoveAnimationState(AnimationState* state);
    /// Remove all animations.
    void RemoveAllAnimationStates();
    /// %Set animation LOD bias.
    void SetAnimationLodBias(float bias);
    /// %Set animation LOD distance factor when not visible (default 0 = do not update at all when invisible.)
    void SetInvisibleLodFactor(float factor);
    /// %Set vertex morph weight by index.
    void SetMorphWeight(unsigned index, float weight);
    /// %Set vertex morph weight by name.
    void SetMorphWeight(const String& name, float weight);
    /// %Set vertex morph weight by name hash.
    void SetMorphWeight(StringHash nameHash, float weight);
    /// Reset all vertex morphs to zero.
    void ResetMorphWeights();
    
    /// Return skeleton.
    Skeleton& GetSkeleton() { return skeleton_; }
    /// Return all animation states.
    const Vector<SharedPtr<AnimationState> >& GetAnimationStates() const { return animationStates_; }
    /// Return number of animation states.
    unsigned GetNumAnimationStates() const { return animationStates_.Size(); }
    /// Return animation state by animation pointer.
    AnimationState* GetAnimationState(Animation* animation) const;
    /// Return animation state by animation name.
    AnimationState* GetAnimationState(const String& animationName) const;
    /// Return animation state by animation name hash.
    AnimationState* GetAnimationState(const StringHash animationNameHash) const;
    /// Return animation state by index.
    AnimationState* GetAnimationState(unsigned index) const;
    /// Return animation LOD bias.
    float GetAnimationLodBias() const { return animationLodBias_; }
    /// Return animation LOD distance factor when not visible.
    float GetInvisibleLodFactor() const { return invisibleLodFactor_; }
    /// Return all vertex morphs.
    const Vector<ModelMorph>& GetMorphs() const { return morphs_; }
    /// Return all morph vertex buffers.
    const Vector<SharedPtr<VertexBuffer> >& GetMorphVertexBuffers() const { return morphVertexBuffers_; }
    /// Return number of vertex morphs.
    unsigned GetNumMorphs() const { return morphs_.Size(); }
    /// Return vertex morph weight by index.
    float GetMorphWeight(unsigned index) const;
    /// Return vertex morph weight by name.
    float GetMorphWeight(const String& name) const;
    /// Return vertex morph weight by name hash.
    float GetMorphWeight(StringHash nameHash) const;
    /// Return whether is the master (first) animated model.
    bool IsMaster() const { return isMaster_; }
    
    /// %Set model attribute.
    void SetModelAttr(ResourceRef value);
    /// %Set bones' animation enabled attribute.
    void SetBonesEnabledAttr(VariantVector value);
    /// %Set animation states attribute.
    void SetAnimationStatesAttr(VariantVector value);
    /// %Set morphs attribute.
    void SetMorphsAttr(const PODVector<unsigned char>& value);
    /// Return model attribute.
    ResourceRef GetModelAttr() const;
    /// Return bones' animation enabled attribute.
    VariantVector GetBonesEnabledAttr() const;
    /// Return animation states attribute.
    VariantVector GetAnimationStatesAttr() const;
    /// Return morphs attribute.
    const PODVector<unsigned char>& GetMorphsAttr() const;
    
protected:
    /// Handle node being assigned.
    virtual void OnNodeSet(Node* node);
    /// Handle node transform being dirtied.
    virtual void OnMarkedDirty(Node* node);
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();
    
private:
    /// Assign skeleton and animation bone node references as a postprocess. Called by ApplyAttributes.
    void AssignBoneNodes();
    /// Mark animation and skinning to require an update.
    void MarkAnimationDirty();
    /// Mark animation and skinning to require a forced update (blending order changed.)
    void MarkAnimationOrderDirty();
    /// Mark morphs to require an update.
    void MarkMorphsDirty();
    /// %Set skeleton.
    void SetSkeleton(const Skeleton& skeleton, bool createBones);
    /// Refresh mapping of subgeometry bone indices.
    void RefreshGeometryBoneMappings();
    /// Clone geometries as required.
    void CloneGeometries();
    /// Recalculate animations. Called from UpdateNode().
    void UpdateAnimation(const FrameInfo& frame);
    /// Recalculate skinning.
    void UpdateSkinning();
    /// Reapply all vertex morphs.
    void UpdateMorphs();
    /// Apply a vertex morph.
    void ApplyMorph(VertexBuffer* buffer, void* lockedMorphRange, const VertexBufferMorph& morph, float weight);
    /// Handle model reload finished.
    void HandleModelReloadFinished(StringHash eventType, VariantMap& eventData);
    
    /// Skeleton.
    Skeleton skeleton_;
    /// Morph vertex buffers.
    Vector<SharedPtr<VertexBuffer> > morphVertexBuffers_;
    /// Vertex morphs.
    Vector<ModelMorph> morphs_;
    /// Animation states.
    Vector<SharedPtr<AnimationState> > animationStates_;
    /// Skinning matrices.
    PODVector<Matrix3x4> skinMatrices_;
    /// Mapping of subgeometry bone indices, used if more bones than skinning shader can manage.
    Vector<PODVector<unsigned> > geometryBoneMappings_;
    /// Subgeometry skinning matrices, used if more bones than skinning shader can manage.
    Vector<PODVector<Matrix3x4> > geometrySkinMatrices_;
    /// Subgeometry skinning matrix pointers, if more bones than skinning shader can manage.
    Vector<PODVector<Matrix3x4*> > geometrySkinMatrixPtrs_;
   /// Attribute buffer.
    mutable VectorBuffer attrBuffer_;
    /// The frame number animation LOD distance was last calculated on.
    unsigned animationLodFrameNumber_;
    /// Animation LOD bias.
    float animationLodBias_;
    /// Animation LOD timer.
    float animationLodTimer_;
    /// Animation LOD distance, the minimum of all LOD view distances last frame.
    float animationLodDistance_;
    /// Animation LOD distance factor when not visible.
    float invisibleLodFactor_;
    /// Animation dirty flag.
    bool animationDirty_;
    /// Animation order dirty flag.
    bool animationOrderDirty_;
    /// Vertex morphs dirty flag.
    bool morphsDirty_;
    /// Skinning dirty flag.
    bool skinningDirty_;
    /// Master model flag.
    bool isMaster_;
    /// Loading flag. During loading bone nodes are not created, as they will be serialized as child nodes.
    bool loading_;
    /// Bone nodes assignment pending flag.
    bool assignBonesPending_;
};
