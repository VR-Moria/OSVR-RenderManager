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

        //===================================================================
        // Overloaded render functions from the base class.
        bool RenderFrameInitialize() override { return true; }
        bool RenderDisplayInitialize(size_t display) override { return true; }
        bool RenderEyeFinalize(size_t eye) override { return true; }
        bool RenderDisplayFinalize(size_t display) override { return true; }

        bool PresentDisplayInitialize(size_t display) override;
        bool PresentDisplayFinalize(size_t display) override;
        bool PresentFrameFinalize() override;

        bool SolidColorEye(size_t eye, const RGBColorf &color) override;

        friend RenderManager OSVR_RENDERMANAGER_EXPORT*
        createRenderManager(OSVR_ClientContext context,
                            const std::string& renderLibraryName,
                            GraphicsLibrary graphicsLibrary);
    };

} // namespace renderkit
} // namespace osvr
