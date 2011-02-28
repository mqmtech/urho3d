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

#ifndef RENDERER_RENDERSURFACE_H
#define RENDERER_RENDERSURFACE_H

#include "Rect.h"
#include "RendererDefs.h"
#include "SharedPtr.h"

class Camera;
class Scene;
class Texture;

//! A viewport definition either for a texture or the backbuffer
struct Viewport
{
    //! Construct with defaults
    Viewport();
    //! Construct with a full rectangle
    Viewport(Scene* scene, Camera* camera);
    //! Construct with a specified rectangle
    Viewport(Scene* scene, Camera* camera, const IntRect& rect);
    
    //! Scene pointer
    WeakPtr<Scene> mScene;
    //! Camera pointer
    WeakPtr<Camera> mCamera;
    //! Viewport rectangle
    IntRect mRect;
};

//! A renderable color or depth stencil surface
class RenderSurface : public RefCounted
{
    friend class Texture2D;
    friend class TextureCube;
    
public:
    //! Construct with parent texture
    RenderSurface(Texture* parentTexture);
    //! Destruct
    ~RenderSurface();
    
    //! Set viewport for auxiliary view rendering
    void setViewport(const Viewport& viewport);
    //! Set linked color buffer
    void setLinkedRenderTarget(RenderSurface* renderTarget);
    //! Set linked depth buffer
    void setLinkedDepthBuffer(RenderSurface* depthBuffer);
    //! Release surface
    void release();
    
    //! Return parent texture
    Texture* getParentTexture() const { return mParentTexture; }
    //! Return Direct3D surface
    void* getSurface() const { return mSurface; }
    //! Return width
    int getWidth() const;
    //! Return height
    int getHeight() const;
    //! Return usage
    TextureUsage getUsage() const;
    //! Return auxiliary view rendering viewport
    const Viewport& getViewport() const { return mViewport; }
    //! Return linked color buffer
    RenderSurface* getLinkedRenderTarget() const { return mLinkedRenderTarget; }
    //! Return linked depth buffer
    RenderSurface* getLinkedDepthBuffer() const { return mLinkedDepthBuffer; }
    
private:
    //! Parent texture
    Texture* mParentTexture;
    //! Direct3D surface
    void* mSurface;
    //! Viewport
    Viewport mViewport;
    //! Linked color buffer
    WeakPtr<RenderSurface> mLinkedRenderTarget;
    //! Linked depth buffer
    WeakPtr<RenderSurface> mLinkedDepthBuffer;
};

#endif // RENDERER_RENDERSURFACE_H
