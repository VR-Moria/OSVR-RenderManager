/** @file
@brief Header file describing the OSVR direct-to-device rendering interface for
Vulkan

@date 2020

@author
ReliaSolnve, Inc.
<http://reliasolve.com>
*/

// Copyright 2020 ReliaSolve, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include "RenderManager.h"
#include <osvr/Util/PlatformConfig.h>
#ifndef OSVR_ANDROID
#include <SDL.h>
#include "RenderManagerSDLInitQuit.h"
#endif

/// @todo Implement a C API as well, as with OpenGL and DirectX11

namespace osvr {
namespace renderkit {

    class RenderManagerVulkan : public RenderManager {
      public:
        virtual ~RenderManagerVulkan();

        // Opens the Vulkan renderer we're going to use.
        OpenResults OpenDisplay() override;

        bool OSVR_RENDERMANAGER_EXPORT
          GetTimingInfo(size_t whichEye, OSVR_RenderTimingInfo& info) override;

      protected:
        bool m_displayOpen; ///< Has our display been opened?

        /// Construct a Vulkan RenderManager.
        RenderManagerVulkan(
            OSVR_ClientContext context,
            ConstructorParameters p);

        // Classes and structures needed to do our rendering.
        class DisplayInfo {
          public:
            /// @todo Shared pointers to objects needed to do our rendering that
            /// will be deleted in the destructor.
            SDL_Window* m_window = nullptr; ///< SDL window pointer
        };
        std::vector<DisplayInfo> m_displays;

        //============================================================================
        // Information needed to render to the final output buffer.  Render
        // state and geometries needed to go from the presented buffers to the
        // screen.
        struct XMFLOAT3 {
            float x;
            float y;
            float z;
        };
        struct XMFLOAT2 {
            float x;
            float y;
        };
        struct DistortionVertex {
            XMFLOAT3 Pos;
            XMFLOAT2 TexR;
            XMFLOAT2 TexG;
            XMFLOAT2 TexB;
        };
        struct DistortionMeshBuffer {
            /// Used to render quads for present mode
            /// @todo vertex buffer
            /// Backing data for vertexBuffer
            /// @todo vertex buffer
            std::vector<DistortionVertex> vertices;
            /// Vertex indices, for DrawIndexed
            /// @todo Index buffer
            /// Backing data for indexBuffer
            std::vector<uint16_t> indices;
        };

        /// @todo One per eye/display combination in case of multiple displays
        /// per eye
        std::vector<DistortionMeshBuffer> m_distortionMeshBuffer;

        //============================================================================
        // Information needed to provide render and depth/stencil buffers for
        // each of the eyes we give to the user to use when rendering.  This is
        // for user code to render into.
        //   This is only used in the non-present-mode interface.
        std::vector<osvr::renderkit::RenderBuffer> m_renderBuffers;
        bool OSVR_RENDERMANAGER_EXPORT constructRenderBuffers();

        //===================================================================
        // Overloaded render functions from the base class.
        bool OSVR_RENDERMANAGER_EXPORT RenderPathSetup() override;

        bool OSVR_RENDERMANAGER_EXPORT RenderDisplayInitialize(size_t display) override { return true; }
        bool OSVR_RENDERMANAGER_EXPORT RenderDisplayFinalize(size_t display) override { return true; }

        bool OSVR_RENDERMANAGER_EXPORT RenderEyeInitialize(size_t eye) override;
        bool OSVR_RENDERMANAGER_EXPORT RenderEyeFinalize(size_t eye) override { return true; }

        bool OSVR_RENDERMANAGER_EXPORT RenderFrameInitialize() override { return true; }
        bool OSVR_RENDERMANAGER_EXPORT RenderFrameFinalize() override;

        bool OSVR_RENDERMANAGER_EXPORT PresentDisplayInitialize(size_t display) override;
        bool OSVR_RENDERMANAGER_EXPORT PresentDisplayFinalize(size_t display) override;

        bool OSVR_RENDERMANAGER_EXPORT PresentFrameInitialize() override { return true; }
        bool OSVR_RENDERMANAGER_EXPORT PresentFrameFinalize() override;

        bool OSVR_RENDERMANAGER_EXPORT PresentEye(PresentEyeParameters params) override;
        bool OSVR_RENDERMANAGER_EXPORT SolidColorEye(size_t eye, const RGBColorf &color) override;

        bool OSVR_RENDERMANAGER_EXPORT PresentRenderBuffersInternal(
            const std::vector<RenderBuffer>& buffers,
            const std::vector<RenderInfo>& renderInfoUsed,
            const RenderParams& renderParams = RenderParams(),
            const std::vector<OSVR_ViewportDescription>&
            normalizedCroppingViewports =
            std::vector<OSVR_ViewportDescription>(),
            bool flipInY = false) override;

        bool OSVR_RENDERMANAGER_EXPORT UpdateDistortionMeshesInternal(
            DistortionMeshType type ///< Type of mesh to produce
            ,
            std::vector<DistortionParameters> const&
            distort ///< Distortion parameters
        ) override;

        bool OSVR_RENDERMANAGER_EXPORT
            RenderSpace(size_t whichSpace, ///< Index into m_callbacks vector
                size_t whichEye, ///< Which eye are we rendering for?
                OSVR_PoseState pose, ///< ModelView transform to use
                OSVR_ViewportDescription viewport, ///< Viewport to use
                OSVR_ProjectionMatrix projection ///< Projection to use
            ) override;

        friend RenderManager OSVR_RENDERMANAGER_EXPORT*
        createRenderManager(OSVR_ClientContext context,
                            const std::string& renderLibraryName,
                            GraphicsLibrary graphicsLibrary);
    };

} // namespace renderkit
} // namespace osvr
