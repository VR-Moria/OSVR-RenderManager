/** @file
@brief Source file implementing Vulkan rendering to a window.

@date 2020

@author
Russ Taylor <russ@reliasolve.com>
ReliaSolve, Inc.
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

#include "RenderManagerVulkan.h"
#include "GraphicsLibraryVulkan.h"
/// @todo If we need a singleton like the SDLInitQuit, get it here

#include <osvr/Util/Logger.h>

#include <SDL_vulkan.h>

#include <iostream>

namespace osvr {
namespace renderkit {

    RenderManagerVulkan::RenderManagerVulkan(OSVR_ClientContext context, ConstructorParameters p)
        : RenderManager(context, p) {

        // Initialize our state.
        /// @todo null the device and context pointers
        m_displayOpen = false;

        // Construct the appropriate GraphicsLibrary pointers.
        m_library.Vulkan = new GraphicsLibraryVulkan;
        m_buffers.Vulkan = new RenderBufferVulkan;
        /// @todo Initialize depth/stencil state for rendering
        /// @todo Initialize depth/stencil state for presenting
    }

    RenderManagerVulkan::~RenderManagerVulkan() {
        for (size_t i = 0; i < m_displays.size(); i++) {
            if (m_displays[i].m_window != nullptr) {
                /// @todo Destroy each window
            }
        }
        if (m_displayOpen) {
            /// @todo Clean up anything else we need to
            m_displayOpen = false;
        }
    }

    RenderManager::OpenResults RenderManagerVulkan::OpenDisplay(void) {
        OpenResults ret;

        // All public methods that use internal state should be guarded
        // by a mutex.
        std::lock_guard<std::mutex> lock(m_mutex);

        /// @todo Open the display

        auto withFailure = [&] {
            setDoingOkay(false);
            ret.status = FAILURE;
            return ret;
        };

        /// @todo How to handle window resizing?

        //======================================================
        // Get a window.

        // Figure out the flags we want
        /// @todo
        if (m_params.m_windowFullScreen) {
            /// @todo
        }

        // @todo Pull this calculation out into the base class and
        // store a separate virtual-screen and actual-screen size.
        // If we've rotated the screen by 90 or 270, then the window
        // we ask for on the screen has swapped aspect ratios.
        int heightRotated, widthRotated;
        if ((m_params.m_displayRotation ==
             ConstructorParameters::Display_Rotation::Ninety) ||
            (m_params.m_displayRotation ==
             ConstructorParameters::Display_Rotation::TwoSeventy)) {
            widthRotated = m_displayHeight;
            heightRotated = m_displayWidth;
        } else {
            widthRotated = m_displayWidth;
            heightRotated = m_displayHeight;
        }

        // Open our windows.
        for (size_t display = 0; display < GetNumDisplays(); display++) {

            // Push another display structure on our list to use
            m_displays.push_back(DisplayInfo());

            // For now, append the display ID to the title.
            /// @todo Make a different title for each window in the config file
            char displayId = '0' + static_cast<char>(display);
            std::string windowTitle = m_params.m_windowTitle + displayId;
            // For now, move the X position of the second display to the
            // right of the entire display for the left one.
            /// @todo Make the config-file entry a vector and read both
            /// from it.
            int windowX = static_cast<int>(m_params.m_windowXPosition +
                                           widthRotated * display);

            /// @todo Create window of given size and parameters
            /*
            m_displays[display].m_window = TODO(
                windowTitle.c_str(), windowX, m_params.m_windowYPosition,
                widthRotated, heightRotated, flags);
            */
            if (m_displays[display].m_window == nullptr) {
              m_log->error()
                    << "RenderManagerVulkan::OpenDisplay: Could not get window "
                    << "for display " << display;
                return withFailure();
            }

            //======================================================
            // Find out the size of the window we just created (which may
            // differ from what we asked for due to zoom factors).
            /// @todo SDL_Vulkan_GetDrawableSize()

            //======================================================
            // Create the color buffers to be used to render into.

            /// @todo

            //==================================================================
            // Get the render target view and depth/stencil view and then set
            // them as the render targets.

            // Create render target views and render targets
            /// @todo

            // Create a render target view for each buffer, so we can swap
            // through them
            /// @todo

            // Set the render targets
            /// @todo
        }

        //======================================================
        // Fill in our library with the things the application may need to
        // use to do its graphics state set-up.
        //m_library.Vulkan->device = m_VulkanDevice;
        //m_library.Vulkan->context = m_VulkanContext;

        //======================================================
        // Done, we now have an open window to use.
        m_displayOpen = true;
        ret.library = m_library;
        return ret;
    }

    bool RenderManagerVulkan::PresentDisplayInitialize(size_t display) {
        if (display >= GetNumDisplays()) {
            return false;
        }

        // We want to render to the on-screen display now.  The user will have
        // switched this to their views.
        /// @todo

        return true;
    }

    bool RenderManagerVulkan::PresentDisplayFinalize(size_t display) {
        if (display >= GetNumDisplays()) {
            return false;
        }

        // Forcefully sync device rendering to the shared surface.
        /// @todo

        // Present the just-rendered surface, waiting for vertical
        // blank if asked to.
        /// @todo

        return true;
    }

    bool RenderManagerVulkan::PresentFrameFinalize() {
        // Let the window manager handle any system events that it needs to.
        /// @todo

        return true;
    }

    bool RenderManagerVulkan::SolidColorEye(
        size_t eye, const RGBColorf &color) {
      RGBColorf colorRGBA[4] = { color.r, color.g, color.b, 1 };
      size_t d = GetDisplayUsedByEye(eye);
      /// @todo

      return true;
    }

    bool RenderManagerVulkan::GetTimingInfo(size_t whichEye,
      OSVR_RenderTimingInfo& info) {
      // This method should be thread-safe, so we don't guard with the lock.

      // Make sure we're doing okay.
      if (!doingOkay()) {
        m_log->error() << "RenderManagerVulkan::GetTimingInfo(): Display "
          "not opened.";
        return false;
      }

      // Make sure we have enough eyes.
      if (whichEye >= GetNumEyes()) {
        m_log->error()
          << "RenderManagerVulkan::GetTimingInfo(): No such eye: "
          << whichEye;
        return false;
      }

      // If our display is not open, we don't have any useful info.
      if (!m_displayOpen) {
        OSVR_RenderTimingInfo i = {};
        info = i;
        m_log->error()
          << "RenderManagerVulkan::GetTimingInfo(): Display not open for eye: "
          << whichEye;
        return false;
      }

      // Figure out which display this eye is on and get a reference to it.
      /// @todo

      // Get the display timing info.
      /// @todo

      // Convert the refresh rate into an interval and store it in the
      // info
      /// @todo

      // Fill in the time until the next presentation is required
      // to make the next vsync depending on the state of the
      // RenderManager.  For now, we ask for 1ms to do the distortion
      // correction/time warp during final presentation.
      /// @todo
      OSVR_TimeValue slackRequired = { 0, 1000 };

      return true;
    }

} // namespace renderkit
} // namespace osvr
