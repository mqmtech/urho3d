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
#include "CollisionShape.h"
#include "Context.h"
#include "DebugRenderer.h"
#include "Geometry.h"
#include "Log.h"
#include "Model.h"
#include "PhysicsWorld.h"
#include "Profiler.h"
#include "ResourceCache.h"
#include "RigidBody.h"
#include "Scene.h"
#include "XMLFile.h"

#include <ode/ode.h>
#include <../ode/src/heightfield.h>
#include <../ode/src/collision_std.h>
#include <hull.h>

#include "DebugNew.h"

static const String typeNames[] = 
{
    "None",
    "Box",
    "Sphere",
    "Capsule",
    "Cylinder",
    "TriangleMesh",
    "Heightfield",
    "ConvexHull",
    ""
};

static const float DEFAULT_FRICTION = 0.5f;
static const float DEFAULT_BOUNCE = 0.0f;
static const Quaternion CYLINDER_ROTATION(90.0f, 0.0f, 0.0f);

void GetVertexAndIndexData(const Model* model, unsigned lodLevel, SharedArrayPtr<Vector3>& destVertexData, unsigned& destVertexCount,
    SharedArrayPtr<unsigned>& destIndexData, unsigned& destIndexCount, const Vector3& scale)
{
    const Vector<Vector<SharedPtr<Geometry> > >& geometries = model->GetGeometries();
    
    destVertexCount = 0;
    destIndexCount = 0;
    
    for (unsigned i = 0; i < geometries.Size(); ++i)
    {
        unsigned subGeometryLodLevel = lodLevel;
        if (subGeometryLodLevel >= geometries[i].Size())
            subGeometryLodLevel = geometries[i].Size() / 2;
        
        Geometry* geom = geometries[i][subGeometryLodLevel];
        if (!geom)
            continue;
        
        destVertexCount += geom->GetVertexCount();
        destIndexCount += geom->GetIndexCount();
    }
    
    if (!destVertexCount || !destIndexCount)
        return;
    
    destVertexData = new Vector3[destVertexCount];
    destIndexData = new unsigned[destIndexCount];
    
    unsigned firstVertex = 0;
    unsigned firstIndex = 0;
    
    for (unsigned i = 0; i < geometries.Size(); ++i)
    {
        unsigned subGeometryLodLevel = lodLevel;
        if (subGeometryLodLevel >= geometries[i].Size())
            subGeometryLodLevel = geometries[i].Size() / 2;
        
        Geometry* geom = geometries[i][subGeometryLodLevel];
        if (!geom)
            continue;
        
        const unsigned char* vertexData;
        const unsigned char* indexData;
        unsigned vertexSize;
        unsigned indexSize;
        
        geom->GetRawData(vertexData, vertexSize, indexData, indexSize);
        if (!vertexData || !indexData)
            continue;
        
        unsigned vertexStart = geom->GetVertexStart();
        unsigned vertexCount = geom->GetVertexCount();
        
        // Copy vertex data
        for (unsigned j = 0; j < vertexCount; ++j)
        {
            const Vector3& v = *((const Vector3*)(&vertexData[(vertexStart + j) * vertexSize]));
            destVertexData[firstVertex + j] = scale * v;
        }
        
        unsigned indexStart = geom->GetIndexStart();
        unsigned indexCount = geom->GetIndexCount();
        
        // 16-bit indices
        if (indexSize == sizeof(unsigned short))
        {
            const unsigned short* indices = (const unsigned short*)indexData;
            
            for (unsigned j = 0; j < indexCount; j += 3)
            {
                // Rebase the indices according to our vertex numbering
                destIndexData[firstIndex + j] = indices[indexStart + j] - vertexStart + firstVertex;
                destIndexData[firstIndex + j + 1] = indices[indexStart + j + 1] - vertexStart + firstVertex;
                destIndexData[firstIndex + j + 2] = indices[indexStart + j + 2] - vertexStart + firstVertex;
            }
        }
        // 32-bit indices
        else
        {
            const unsigned* indices = (const unsigned*)indexData;
            
            for (unsigned j = 0; j < indexCount; j += 3)
            {
                // Rebase the indices according to our vertex numbering
                destIndexData[firstIndex + j] = indices[indexStart + j] - vertexStart + firstVertex;
                destIndexData[firstIndex + j + 1] = indices[indexStart + j + 1] - vertexStart + firstVertex;
                destIndexData[firstIndex + j + 2] = indices[indexStart + j + 2] - vertexStart + firstVertex;
            }
        }
        
        firstVertex += vertexCount;
        firstIndex += indexCount;
    }
}

TriangleMeshData::TriangleMeshData(Model* model, bool makeConvexHull, float thickness, unsigned lodLevel, const Vector3& scale) :
    triMesh_(0),
    indexCount_(0)
{
    modelName_ = model->GetName();
    
    unsigned vertexCount;
    unsigned indexCount;
    
    if (!makeConvexHull)
        GetVertexAndIndexData(model, lodLevel, vertexData_, vertexCount, indexData_, indexCount, scale);
    else
    {
        SharedArrayPtr<Vector3> originalVertices;
        SharedArrayPtr<unsigned> originalIndices;
        unsigned originalVertexCount;
        unsigned originalIndexCount;
        
        GetVertexAndIndexData(model, lodLevel, originalVertices, originalVertexCount, originalIndices, originalIndexCount, scale);
        
        // Build the convex hull from the raw geometry
        StanHull::HullDesc desc;
        desc.SetHullFlag(StanHull::QF_TRIANGLES);
        desc.mVcount = originalVertexCount;
        desc.mVertices = (float*)originalVertices.Get();
        desc.mVertexStride = 3 * sizeof(float);
        desc.mSkinWidth = thickness;
        
        StanHull::HullLibrary lib;
        StanHull::HullResult result;
        lib.CreateConvexHull(desc, result);
    
        vertexCount = result.mNumOutputVertices;
        indexCount = result.mNumIndices;
        
        // Copy vertex data
        vertexData_ = new Vector3[vertexCount];
        memcpy(vertexData_.Get(), result.mOutputVertices, vertexCount * sizeof(Vector3));
        
        // Copy index data
        indexData_ = new unsigned[indexCount];
        memcpy(indexData_.Get(), result.mIndices, indexCount * sizeof(unsigned));
        
        lib.ReleaseResult(result);
    }
    
    triMesh_ = dGeomTriMeshDataCreate();
    
    dGeomTriMeshDataBuildSingle(triMesh_, vertexData_.Get(), sizeof(Vector3), vertexCount,
        indexData_.Get(), indexCount, 3 * sizeof(unsigned));
    
    indexCount_ = indexCount;
}

TriangleMeshData::~TriangleMeshData()
{
    if (triMesh_)
    {
        dGeomTriMeshDataDestroy(triMesh_);
        triMesh_ = 0;
    }
}

HeightfieldData::HeightfieldData(Model* model, IntVector2 numPoints, float thickness, unsigned lodLevel,
    const Vector3& scale) :
    heightfield_(0)
{
    modelName_ = model->GetName();
    
    const Vector<Vector<SharedPtr<Geometry> > >& geometries = model->GetGeometries();
    
    if (!geometries.Size())
        return;
    
    lodLevel = Clamp(lodLevel, 0, geometries[0].Size());
    
    Geometry* geom = geometries[0][lodLevel];
    if (!geom)
        return;
    
    const unsigned char* vertexData;
    const unsigned char* indexData;
    unsigned vertexSize;
    unsigned indexSize;
    
    geom->GetRawData(vertexData, vertexSize, indexData, indexSize);
    if (!vertexData || !indexData)
        return;
    
    unsigned indexStart = geom->GetIndexStart();
    unsigned indexCount = geom->GetIndexCount();
    
    // If X & Z size not specified, try to guess them
    if (numPoints == IntVector2::ZERO)
        numPoints.x_ = numPoints.y_ = (int)sqrtf((float)geom->GetVertexCount());
    unsigned dataSize = numPoints.x_ * numPoints.y_;
    
    // Then allocate the heightfield
    BoundingBox box = model->GetBoundingBox();
    heightData_ = new float[dataSize];
    
    // Calculate spacing from model's bounding box
    float xSpacing = (box.max_.x_ - box.min_.x_) / (numPoints.x_ - 1);
    float zSpacing = (box.max_.z_ - box.min_.z_) / (numPoints.y_ - 1);
    
    // Initialize the heightfield with minimum height
    for (unsigned i = 0; i < dataSize; ++i)
        heightData_[i] = box.min_.y_ * scale.y_;
    
    unsigned vertexStart = geom->GetVertexStart();
    unsigned vertexCount = geom->GetVertexCount();
    
    // Now go through vertex data and fit the vertices into the heightfield
    for (unsigned i = vertexStart; i < vertexStart + vertexCount; ++i)
    {
        const Vector3& vertex = *((const Vector3*)(&vertexData[i * vertexSize]));
        int x = (int)((vertex.x_ - box.min_.x_) / xSpacing + 0.25f);
        int z = (int)((vertex.z_ - box.min_.z_) / zSpacing + 0.25f);
        if (x >= numPoints.x_)
            x = numPoints.x_ - 1;
        if (z >= numPoints.y_)
            z = numPoints.y_ - 1;
        if (vertex.y_ > heightData_[z * numPoints.x_ + x])
            heightData_[z * numPoints.x_ + x] = vertex.y_ * scale.y_;
    }
    
    heightfield_ = dGeomHeightfieldDataCreate();
    
    dGeomHeightfieldDataBuildSingle(heightfield_, heightData_.Get(), 0, (box.max_.x_ - box.min_.x_) * scale.x_,
        (box.max_.z_ - box.min_.z_) * scale.z_, numPoints.x_, numPoints.y_, 1.0f, 0.0f, thickness, 0);
    dGeomHeightfieldDataSetBounds(heightfield_, box.min_.y_ * scale.y_, box.max_.y_ * scale.y_);
}

HeightfieldData::~HeightfieldData()
{
    if (heightfield_)
    {
        dGeomHeightfieldDataDestroy(heightfield_);
        heightfield_ = 0;
    }
}

OBJECTTYPESTATIC(CollisionShape);

CollisionShape::CollisionShape(Context* context) :
    Component(context),
    geometry_(0),
    shapeType_(SHAPE_NONE),
    size_(Vector3::UNITY),
    thickness_(0.0f),
    lodLevel_(0),
    position_(Vector3::ZERO),
    rotation_(Quaternion::IDENTITY),
    geometryScale_(Vector3::UNITY),
    collisionLayer_(M_MAX_UNSIGNED),
    collisionMask_(M_MAX_UNSIGNED),
    friction_(DEFAULT_FRICTION),
    bounce_(DEFAULT_BOUNCE),
    phantom_(false),
    recreateGeometry_(false)
{
}

CollisionShape::~CollisionShape()
{
    Clear();
}

void CollisionShape::RegisterObject(Context* context)
{
    context->RegisterFactory<CollisionShape>();
    
    ENUM_ATTRIBUTE(CollisionShape, "Shape Type", shapeType_, typeNames, SHAPE_NONE, AM_DEFAULT);
    ATTRIBUTE(CollisionShape, VAR_VECTOR3, "Size", size_, Vector3::UNITY, AM_DEFAULT);
    REF_ACCESSOR_ATTRIBUTE(CollisionShape, VAR_VECTOR3, "Offset Position", GetPosition, SetPosition, Vector3, Vector3::ZERO, AM_DEFAULT);
    REF_ACCESSOR_ATTRIBUTE(CollisionShape, VAR_QUATERNION, "Offset Rotation", GetRotation, SetRotation, Quaternion, Quaternion::IDENTITY, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(CollisionShape, VAR_RESOURCEREF, "Model", GetModelAttr, SetModelAttr, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
    ATTRIBUTE(CollisionShape, VAR_INT, "LOD Level", lodLevel_, 0, AM_DEFAULT);
    ATTRIBUTE(CollisionShape, VAR_FLOAT, "Hull Thickness", thickness_, 0.0f, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(CollisionShape, VAR_FLOAT, "Friction", GetFriction, SetFriction, float, DEFAULT_FRICTION, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(CollisionShape, VAR_FLOAT, "Bounce", GetBounce, SetBounce, float, DEFAULT_BOUNCE, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(CollisionShape, VAR_INT, "Collision Group", GetCollisionLayer, SetCollisionLayer, unsigned, M_MAX_UNSIGNED, AM_DEFAULT);
    ACCESSOR_ATTRIBUTE(CollisionShape, VAR_INT, "Collision Mask", GetCollisionMask, SetCollisionMask, unsigned, M_MAX_UNSIGNED, AM_DEFAULT);
    ATTRIBUTE(CollisionShape, VAR_BOOL, "Is Phantom", phantom_, false, AM_DEFAULT);
}

void CollisionShape::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
    Serializable::OnSetAttribute(attr, src);
    
    // Change of some attributes requires the geometry to be recreated
    switch (attr.offset_)
    {
    case offsetof(CollisionShape, size_):
        size_ = size_.Abs(); // Negative size is not allowed
        recreateGeometry_ = true;
        break;
        
    case offsetof(CollisionShape, shapeType_):
    case offsetof(CollisionShape, thickness_):
    case offsetof(CollisionShape, lodLevel_):
        recreateGeometry_ = true;
        break;
    }
}

void CollisionShape::ApplyAttributes()
{
    if (recreateGeometry_)
    {
        CreateGeometry();
        recreateGeometry_ = false;
    }
}

void CollisionShape::Clear()
{
    ReleaseGeometry();
    
    shapeType_ = SHAPE_NONE;
}

void CollisionShape::SetSphere(float diameter, const Vector3& position, const Quaternion& rotation)
{
    ReleaseGeometry();
    
    shapeType_ = SHAPE_SPHERE;
    size_ = Vector3(diameter, diameter, diameter);
    position_ = position;
    rotation_ = rotation;
    
    CreateGeometry();
}

void CollisionShape::SetBox(const Vector3& size, const Vector3& position, const Quaternion& rotation)
{
    ReleaseGeometry();
    
    shapeType_ = SHAPE_BOX;
    size_ = size;
    position_ = position;
    rotation_ = rotation;
    
    CreateGeometry();
}

void CollisionShape::SetCapsule(float diameter, float height, const Vector3& position, const Quaternion& rotation)
{
    ReleaseGeometry();
    
    shapeType_ = SHAPE_CAPSULE;
    size_ = Vector3(diameter, height, diameter);
    position_ = position;
    rotation_ = rotation;
    
    CreateGeometry();
}

void CollisionShape::SetCylinder(float diameter, float height, const Vector3& position, const Quaternion& rotation)
{
    ReleaseGeometry();
    
    shapeType_ = SHAPE_CYLINDER;
    size_ = Vector3(diameter, height, diameter);
    position_ = position;
    rotation_ = rotation;
    
    CreateGeometry();
}

void CollisionShape::SetTriangleMesh(Model* model, unsigned lodLevel, const Vector3& size, const Vector3& position, const Quaternion& rotation)
{
    PROFILE(SetTriangleMeshShape);
    
    if (!model)
    {
        LOGERROR("Null model, can not set triangle mesh");
        return;
    }
    
    ReleaseGeometry();
    
    model_ = model;
    shapeType_ = SHAPE_TRIANGLEMESH;
    lodLevel_ = lodLevel;
    size_ = size.Abs();
    position_ = position;
    rotation_ = rotation;
    
    CreateGeometry();
}

void CollisionShape::SetHeightfield(Model* model, unsigned xPoints, unsigned zPoints, float thickness, unsigned lodLevel, const Vector3& size, const Vector3& position, const Quaternion& rotation)
{
    PROFILE(SetHeightFieldShape);
    
    if (!model)
    {
        LOGERROR("Null model, can not set heightfield");
        return;
    }
    
    ReleaseGeometry();
    
    model_ = model;
    shapeType_ = SHAPE_HEIGHTFIELD;
    numPoints_ = IntVector2(xPoints, zPoints);
    thickness_ = thickness;
    lodLevel_ = lodLevel;
    size_ = size.Abs();
    position_ = position;
    rotation_ = rotation;
    
    CreateGeometry();
}

void CollisionShape::SetConvexHull(Model* model, float thickness, unsigned lodLevel, const Vector3& size, const Vector3& position, const Quaternion& rotation)
{
    PROFILE(SetConvexHullShape);
    
    if (!model)
    {
        LOGERROR("Null model, can not set convex hull");
        return;
    }
    
    ReleaseGeometry();
    
    model_ = model;
    shapeType_ = SHAPE_CONVEXHULL;
    thickness_ = thickness;
    lodLevel_ = lodLevel;
    size_ = size.Abs();
    position_ = position;
    rotation_ = rotation;
    
    CreateGeometry();
}

void CollisionShape::SetPosition(const Vector3& position)
{
    position_  = position;
    UpdateTransform();
}

void CollisionShape::SetRotation(const Quaternion& rotation)
{
    rotation_ = rotation;
    UpdateTransform();
}

void CollisionShape::SetTransform(const Vector3& position, const Quaternion& rotation)
{
    position_ = position;
    rotation_ = rotation;
    UpdateTransform();
}

void CollisionShape::SetCollisionLayer(unsigned group)
{
    collisionLayer_ = group;
    if (geometry_)
        dGeomSetCategoryBits(geometry_, group);
}

void CollisionShape::SetCollisionMask(unsigned mask)
{
    collisionMask_ = mask;
    if (geometry_)
        dGeomSetCollideBits(geometry_, mask);
}

void CollisionShape::SetFriction(float friction)
{
    friction_ = Max(friction, 0.0f);
}

void CollisionShape::SetBounce(float bounce)
{
    bounce_ = Max(bounce, 0.0f);
}

void CollisionShape::SetPhantom(bool enable)
{
    phantom_ = enable;
}

void CollisionShape::UpdateTransform(bool nodeUpdate)
{
    if (!geometry_)
        return;
    
    // Get the ODE body ID from the RigidBody component, if it exists
    RigidBody* rigidBody = GetComponent<RigidBody>();
    dBodyID body = rigidBody ? rigidBody->GetBody() : 0;
    
    // Apply an adjustment to the cylinder and capsule shapes to make them upright by default
    Quaternion offsetQuaternion = (shapeType_ == SHAPE_CYLINDER || shapeType_ == SHAPE_CAPSULE) ? CYLINDER_ROTATION * rotation_ : rotation_;
    
    if (body)
    {
        // Assign body now if necessary
        if (dGeomGetBody(geometry_) != body)
            dGeomSetBody(geometry_, body);
        else
        {
            // If the body is already assigned, and this is a node dirtying update, need to do nothing
            if (nodeUpdate)
                return;
        }
        
        // Update the offset transform
        if (position_ != Vector3::ZERO || offsetQuaternion != Quaternion::IDENTITY)
        {
            Vector3 offset(geometryScale_ * position_);
            dGeomSetOffsetPosition(geometry_, offset.x_, offset.y_, offset.z_);
            dGeomSetOffsetQuaternion(geometry_, offsetQuaternion.GetData());
        }
        else
            dGeomClearOffset(geometry_);
    }
    else
    {
        // No rigid body. Must update the geometry transform manually
        // Use the target transform in case the node has smoothed motion enabled
        Matrix3x4 transform(node_->GetWorldTargetTransform());
        Vector3 nodePos(transform.Translation());
        Quaternion nodeRot(transform.Rotation());
        Vector3 geomPos(nodePos + (nodeRot * (geometryScale_ * position_)));
        Quaternion geomRot(nodeRot * offsetQuaternion);
        
        dGeomSetPosition(geometry_, geomPos.x_, geomPos.y_, geomPos.z_);
        dGeomSetQuaternion(geometry_, geomRot.GetData());
    }
}

void CollisionShape::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (!geometry_)
        return;
    
    const Color* color;
    RigidBody* rigidBody = GetComponent<RigidBody>();
    if (rigidBody && rigidBody->IsActive())
        color = &Color::WHITE;
    else
        color = &Color::GREEN;
    unsigned uintColor = color->ToUInt();
    
    // Drawing all the debug geometries of a large world may be expensive (especially triangle meshes)
    // so check the geometry AABB against the debug geometry frustum first
    float aabb[6];
    dGeomGetAABB(geometry_, aabb);
    BoundingBox box;
    box.min_.x_ = aabb[0];
    box.max_.x_ = aabb[1];
    box.min_.y_ = aabb[2];
    box.max_.y_ = aabb[3];
    box.min_.z_ = aabb[4];
    box.max_.z_ = aabb[5];
    if (!debug->IsInside(box))
        return;
    
    const Vector3& position = *reinterpret_cast<const Vector3*>(dGeomGetPosition(geometry_));
    Quaternion rotation;
    dGeomGetQuaternion(geometry_, const_cast<float*>(rotation.GetData()));
    Matrix3x4 transform(position, rotation, 1.0f);
    
    switch (dGeomGetClass(geometry_))
    {
    case dSphereClass:
        {
            float radius = dGeomSphereGetRadius(geometry_);
            
            for (unsigned i = 0; i < 360; i += 45)
            {
                unsigned j = i + 45;
                float a = radius * sinf(i * M_DEGTORAD);
                float b = radius * cosf(i * M_DEGTORAD);
                float c = radius * sinf(j * M_DEGTORAD);
                float d = radius * cosf(j * M_DEGTORAD);
                Vector3 start, end;
                
                start = transform * Vector3(a, b, 0.0f);
                end = transform * Vector3(c, d, 0.0f);
                debug->AddLine(start, end, uintColor, depthTest);
                start = transform * Vector3(a, 0.0f, b);
                end = transform * Vector3(c, 0.0f, d);
                debug->AddLine(start, end, uintColor, depthTest);
                start = transform * Vector3(0.0f, a, b);
                end = transform * Vector3(0.0f, c, d);
                debug->AddLine(start, end, uintColor, depthTest);
            }
        }
        break;
        
    case dBoxClass:
        {
            dVector3 size;
            dGeomBoxGetLengths(geometry_, size);
            BoundingBox box(0.5f * Vector3(size[0], size[1], size[2]), 0.5f * Vector3(-size[0], -size[1], -size[2]));
            debug->AddBoundingBox(box, transform, *color, depthTest);
        }
        break;
        
    case dCapsuleClass:
        {
            float radius, length;
            
            dGeomCapsuleGetParams(geometry_, &radius, &length);
            for (unsigned i = 0; i < 360; i += 45)
            {
                unsigned j = i + 45;
                
                float a = radius * sinf(i * M_DEGTORAD);
                float b = radius * cosf(i * M_DEGTORAD);
                float c = radius * sinf(j * M_DEGTORAD);
                float d = radius * cosf(j * M_DEGTORAD);
                Vector3 start, end;
                
                start = transform * Vector3(a, b, 0.5f * length);
                end = transform * Vector3(c, d, 0.5f * length);
                debug->AddLine(start, end, uintColor, depthTest);
                start = transform * Vector3(a, b, -0.5f * length);
                end = transform * Vector3(c, d, -0.5f * length);
                debug->AddLine(start, end, uintColor, depthTest);
                if (!(i & 1))
                {
                    start = transform * Vector3(a, b, 0.5f * length);
                    end = transform * Vector3(a, b, -0.5f * length);
                    debug->AddLine(start, end, uintColor, depthTest);
                }
                if (b > -M_EPSILON)
                {
                    start = transform * Vector3(a, 0.0f, b + 0.5f * length);
                    end = transform * Vector3(c, 0.0f, d + 0.5f * length);
                    debug->AddLine(start, end, uintColor, depthTest);
                    start = transform * Vector3(0.0f, a, b + 0.5f * length);
                    end = transform * Vector3(0.0f, c, d + 0.5f * length);
                    debug->AddLine(start, end, uintColor, depthTest);
                    start = transform * Vector3(a, 0.0f, -b - 0.5f * length);
                    end = transform * Vector3(c, 0.0f, -d - 0.5f * length);
                    debug->AddLine(start, end, uintColor, depthTest);
                    start = transform * Vector3(0.0f, a, -b - 0.5f * length);
                    end = transform * Vector3(0.0f, c, -d - 0.5f * length);
                    debug->AddLine(start, end, uintColor, depthTest);
                }
            }
        }
        break;
        
    case dCylinderClass:
        {
            float radius, length;
            
            dGeomCylinderGetParams(geometry_, &radius, &length);
            for (unsigned i = 0; i < 360; i += 45)
            {
                unsigned j = i + 45;
                float a = radius * sinf(i * M_DEGTORAD);
                float b = radius * cosf(i * M_DEGTORAD);
                float c = radius * sinf(j * M_DEGTORAD);
                float d = radius * cosf(j * M_DEGTORAD);
                Vector3 start, end;
                
                start = transform * Vector3(a, b, 0.5f * length);
                end = transform * Vector3(c, d, 0.5f * length);
                debug->AddLine(start, end, uintColor, depthTest);
                start = transform * Vector3(a, b, -0.5f * length);
                end = transform * Vector3(c, d, -0.5f * length);
                debug->AddLine(start, end, uintColor, depthTest);
                start = transform * Vector3(a, b, 0.5f * length);
                end = transform * Vector3(a, b, -0.5f * length);
                debug->AddLine(start, end, uintColor, depthTest);
            }
        }
        break;
        
    case dTriMeshClass:
        {
            TriangleMeshData* data = static_cast<TriangleMeshData*>(geometryData_.Get());
            if (!data)
                return;
            
            const Vector3* vertices = data->vertexData_;
            const unsigned* indices = data->indexData_;
            const unsigned* indicesEnd = indices + data->indexCount_;
            
            while (indices < indicesEnd)
            {
                Vector3 v0 = transform * vertices[indices[0]];
                Vector3 v1 = transform * vertices[indices[1]];
                Vector3 v2 = transform * vertices[indices[2]];
                
                debug->AddLine(v0, v1, uintColor, depthTest);
                debug->AddLine(v1, v2, uintColor, depthTest);
                debug->AddLine(v2, v0, uintColor, depthTest);
                
                indices += 3;
            }
        }
        break;
        
    case dHeightfieldClass:
        {
            dHeightfieldDataID heightData = dGeomHeightfieldGetHeightfieldData(geometry_);
            unsigned xPoints = heightData->m_nWidthSamples;
            unsigned zPoints = heightData->m_nDepthSamples;
            float xWidth = heightData->m_fWidth;
            float zWidth = heightData->m_fDepth;
            float xBase = -0.5f * xWidth;
            float zBase = -0.5f * zWidth;
            float xSpacing = xWidth / (xPoints - 1);
            float zSpacing = zWidth / (zPoints - 1);
            float* heights = (float*)heightData->m_pHeightData;
            for (unsigned z = 0; z < zPoints - 1; ++z)
            {
                for (unsigned x = 0; x < xPoints - 1; ++x)
                {
                    Vector3 a = transform * Vector3(xBase + x * xSpacing, heights[z * xPoints + x], zBase + z * zSpacing);
                    Vector3 b = transform * Vector3(xBase + (x + 1) * xSpacing, heights[z * xPoints + x + 1], zBase + z * zSpacing);
                    Vector3 c = transform * Vector3(xBase + x * xSpacing, heights[(z + 1) * xPoints + x], zBase + (z + 1) * zSpacing);
                    debug->AddLine(a, b, uintColor, depthTest);
                    debug->AddLine(a, c, uintColor, depthTest);
                }
            }
            for (unsigned z = 0; z < zPoints - 1; ++z)
            {
                unsigned x = xPoints - 1;
                
                Vector3 a = transform * Vector3(xBase + x * xSpacing, heights[z * xPoints + x], zBase + z * zSpacing);
                Vector3 b = transform * Vector3(xBase + x * xSpacing, heights[(z + 1) * xPoints + x], zBase + (z + 1) * zSpacing);
                debug->AddLine(a, b, uintColor, depthTest);
            }
            for (unsigned x = 0; x < xPoints - 1; ++x)
            {
                unsigned z = zPoints - 1;
                
                Vector3 a = transform * Vector3(xBase + x * xSpacing, heights[z * xPoints + x], zBase + z * zSpacing);
                Vector3 b = transform * Vector3(xBase + (x + 1) * xSpacing, heights[z * xPoints + x + 1], zBase + z * zSpacing);
                debug->AddLine(a, b, uintColor, depthTest);
            }
        }
        break;
    }
}

void CollisionShape::OnMarkedDirty(Node* node)
{
    // If scale has changed, must recreate the geometry
    if (node->GetWorldScale() != geometryScale_)
        CreateGeometry();
    else
        UpdateTransform(true);
}

void CollisionShape::SetModelAttr(ResourceRef value)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    model_ = cache->GetResource<Model>(value.id_);
    recreateGeometry_ = true;
}

ResourceRef CollisionShape::GetModelAttr() const
{
    return GetResourceRef(model_, Model::GetTypeStatic());
}

void CollisionShape::OnNodeSet(Node* node)
{
    if (node)
    {
        Scene* scene = node->GetScene();
        if (scene)
            physicsWorld_ = scene->GetComponent<PhysicsWorld>();
        node->AddListener(this);
    }
}

void CollisionShape::CreateGeometry()
{
    PROFILE(CreateCollisionShape);
    
    if (!physicsWorld_)
    {
        LOGERROR("Null physics world, can not create collision shape");
        return;
    }
    
    // Destroy previous geometry if exists
    if (geometry_)
    {
        dGeomDestroy(geometry_);
        geometry_ = 0;
    }
    
    geometryScale_ = node_->GetWorldScale();
    Vector3 size = size_ * geometryScale_;
    dSpaceID space = physicsWorld_->GetSpace();
    
    switch (shapeType_)
    {
    case SHAPE_BOX:
        geometry_ = dCreateBox(space, size.x_, size.y_, size.z_);
        break;
        
    case SHAPE_SPHERE:
        geometry_ = dCreateSphere(space, 0.5f * size.x_);
        break;
        
    case SHAPE_CAPSULE:
        geometry_ = dCreateCapsule(space, 0.5f * size.x_, Max(size.y_ - size.x_, 0.0f));
        break;
        
    case SHAPE_CYLINDER:
        geometry_ = dCreateCylinder(space, 0.5f * size.x_, size.y_);
        break;
        
    case SHAPE_TRIANGLEMESH:
    case SHAPE_CONVEXHULL:
        {
            // For mesh cache lookup purposes, quantize size to 3 decimals only. Otherwise floating point inaccuracy from world
            // matrix multiplications and rotation/scale decomposing causes several slightly differing meshes to be created
            char sizeText[CONVERSION_BUFFER_LENGTH];
            sprintf(sizeText, "%.3f%.3f%.3f", size.x_, size.y_, size.z_);
            
            // Check the geometry cache
            String id = model_->GetName() + "_" + String(sizeText) + "_" + String(lodLevel_);
            if (shapeType_ == SHAPE_CONVEXHULL)
                id += "_" + String(thickness_);
            
            Map<String, SharedPtr<TriangleMeshData> >& cache = physicsWorld_->GetTriangleMeshCache();
            Map<String, SharedPtr<TriangleMeshData> >::Iterator j = cache.Find(id);
            if (j != cache.End())
            {
                geometry_ = dCreateTriMesh(space, j->second_->triMesh_, 0, 0, 0);
                geometryData_ = StaticCast<CollisionGeometryData>(j->second_);
            }
            else
            {
                SharedPtr<TriangleMeshData> newData(new TriangleMeshData(model_, shapeType_ == SHAPE_CONVEXHULL, thickness_,
                    lodLevel_, size));
                cache[id] = newData;
                geometry_ = dCreateTriMesh(space, newData->triMesh_, 0, 0, 0);
                geometryData_ = StaticCast<CollisionGeometryData>(newData);
            }
        }
        break;
        
    case SHAPE_HEIGHTFIELD:
        {
            char sizeText[CONVERSION_BUFFER_LENGTH];
            sprintf(sizeText, "%.3f%.3f%.3f", size.x_, size.y_, size.z_);
            
            // Check the geometry cache
            String id = model_->GetName() + "_" + String(sizeText) + "_" + String(numPoints_) + "_" + String(thickness_) + "_" +
                String(lodLevel_);
            
            Map<String, SharedPtr<HeightfieldData> >& cache = physicsWorld_->GetHeightfieldCache();
            Map<String, SharedPtr<HeightfieldData> >::Iterator j = cache.Find(id);
            if (j != cache.End())
            {
                geometry_ = dCreateHeightfield(space, j->second_->heightfield_, 1);
                geometryData_ = StaticCast<CollisionGeometryData>(j->second_);
            }
            else
            {
                SharedPtr<HeightfieldData> newData(new HeightfieldData(model_, numPoints_, thickness_, lodLevel_, size));
                cache[id] = newData;
                geometry_ = dCreateHeightfield(space, newData->heightfield_, 1);
                geometryData_ = StaticCast<CollisionGeometryData>(newData);
            }
        }
        break;
    }
    
    // Set collision group and mask & userdata
    if (geometry_)
    {
        dGeomSetCategoryBits(geometry_, collisionLayer_);
        dGeomSetCollideBits(geometry_, collisionMask_);
        dGeomSetData(geometry_, this);
    }
    
    UpdateTransform();
    
    // If rigid body component exists, let it recalculate its mass now
    RigidBody* rigidBody = GetComponent<RigidBody>();
    if (rigidBody)
        rigidBody->UpdateMass();
}

void CollisionShape::ReleaseGeometry(bool notifyBody)
{
    if (!physicsWorld_)
        return;
    
    if (geometry_)
    {
        dGeomDestroy(geometry_);
        geometry_ = 0;
    }
    
    model_.Reset();
    geometryData_.Reset();
    physicsWorld_->CleanupGeometryCache();
    
    // If rigid body component exists, let it recalculate its mass now
    if (notifyBody)
    {
        RigidBody* rigidBody = GetComponent<RigidBody>();
        if (rigidBody)
            rigidBody->UpdateMass();
    }
}
