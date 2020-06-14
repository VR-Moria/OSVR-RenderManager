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
#include <vulkan/vulkan.h>

#include <iostream>

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace osvr {
namespace renderkit {

    RenderManagerVulkan::RenderManagerVulkan(OSVR_ClientContext context, ConstructorParameters p)
        : RenderManager(context, p) {

        m_displayOpen = false;

        // Construct the appropriate GraphicsLibrary pointers.
        m_library.Vulkan = new GraphicsLibraryVulkan;
        m_buffers.Vulkan = new RenderBufferVulkan;

        // Initialize our state.
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "OSVR";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
#if defined(_WIN32)
        createInfo.enabledExtensionCount = 2;
        const char* eNames[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
#else
        createInfo.enabledExtensionCount = 1;
        const char* eNames[] = { VK_KHR_SURFACE_EXTENSION_NAME };
#endif
        createInfo.ppEnabledExtensionNames = eNames;
        VkResult result = vkCreateInstance(&createInfo, 0, &m_library.Vulkan->instance);
        if (result != VK_SUCCESS) {
            m_log->error()
                << "RenderManagerVulkan::RenderManagerVulkan: Could not get "
                << "instance, code " << result;
            setDoingOkay(false);
            return;
        }
        /// @todo null the device and context pointers

        /// @todo Initialize depth/stencil state for rendering
        /// @todo Initialize depth/stencil state for presenting
    }

    RenderManagerVulkan::~RenderManagerVulkan() {
        // Release any prior buffers we allocated
        m_distortionMeshBuffer.clear();

        if (m_library.Vulkan->instance) {
            for (size_t i = 0; i < m_displays.size(); i++) {
                if (m_displays[i].m_surface != nullptr) {
                    vkDestroySurfaceKHR(m_library.Vulkan->instance,
                        m_displays[i].m_surface, 0);
                }
                if (m_displays[i].m_window != nullptr) {
                    SDL_DestroyWindow(m_displays[i].m_window);
                }
            }
            if (m_displayOpen) {
                /// @todo Clean up anything else we need to
            }
        }
        m_displayOpen = false;

        // Done with Vulkan
        if (m_library.Vulkan && m_library.Vulkan->instance) {
            vkDestroyInstance(m_library.Vulkan->instance, 0);
        }
    }

    RenderManager::OpenResults RenderManagerVulkan::OpenDisplay(void) {
        OpenResults ret;
        ret.status = COMPLETE; // Until we hear otherwise

        // All public methods that use internal state should be guarded
        // by a mutex.
        std::lock_guard<std::mutex> lock(m_mutex);

        /// @todo Open the display

        auto withFailure = [&] {
            setDoingOkay(false);
            ret.status = FAILURE;
            return ret;
        };

        // Make sure we have an instance.
        if (!m_library.Vulkan->instance) {
            m_log->error() << "RenderManagerVulkan::OpenDisplay: No "
                "Vulkan instance";
            return withFailure();
        }

        /// @todo How to handle window resizing?

        //======================================================
        // Use SDL to get us a window.
        /// @todo Enable DirectMode windows when they are asked for, using
        // OS-native calls.

        // Initialize the SDL video subsystem.
        if (!SDLInitQuit()) {
            m_log->error() << "RenderManagerVulkan::OpenDisplay: Could not "
                "initialize SDL";
            return withFailure();
        }

        // Figure out the flags we want
        Uint32 flags = SDL_WINDOW_RESIZABLE;
        flags |= SDL_WINDOW_VULKAN;
        if (m_params.m_windowFullScreen) {
            //        flags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
            flags |= SDL_WINDOW_BORDERLESS;
        }
        if (true) {
            flags |= SDL_WINDOW_SHOWN;
        } else {
            flags |= SDL_WINDOW_HIDDEN;
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

            m_displays[display].m_window = SDL_CreateWindow(
                windowTitle.c_str(), windowX, m_params.m_windowYPosition,
                widthRotated, heightRotated, flags);
            if (m_displays[display].m_window == nullptr) {
              m_log->error()
                    << "RenderManagerVulkan::OpenDisplay: Could not get window "
                    << "for display " << display;
                return withFailure();
            }
            if (!SDL_Vulkan_CreateSurface(m_displays[display].m_window,
                    m_library.Vulkan->instance, &m_displays[display].m_surface)) {
                m_log->error()
                    << "RenderManagerVulkan::OpenDisplay: Could not get surface "
                    << "for display " << display;
                return withFailure();
            }

            //======================================================
            // Find out the size of the window we just created (which may
            // differ from what we asked for due to zoom factors).
            int width, height;
            SDL_GetWindowSize(m_displays[display].m_window, &width, &height);

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
        // Let SDL handle any system events that it needs to.
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            // If SDL has been given a quit event, what should we do?
            // We return false to let the app know that something went wrong.
            if (e.window.event == SDL_QUIT) {
                return false;
            }
            else if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                return false;
            }
        }

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

    bool RenderManagerVulkan::constructRenderBuffers() {
        bool ret = true;

        for (size_t i = 0; i < GetNumEyes(); i++) {

            OSVR_ViewportDescription v;
            ConstructViewportForRender(i, v);
            unsigned width = static_cast<unsigned>(v.width);
            unsigned height = static_cast<unsigned>(v.height);

            // The color buffer for this eye.  We need to put this into
            // a generic structure for the Present function, but we only need
            // to fill in the Vulkan portion.
            //  Note that this texture format must be RGBA and unsigned byte,
            // so that we can present it to for DirectMode.
            /// @todo

            // Initialize a new render target texture description.
            /// @todo
            // We need it to be both a render target and a shader resource
            /// @todo

            // Create a new render target texture to use.
            /// @todo

            // Fill in the resource view for your render texture buffer here
            /// @todo
            // This must match what was created in the texture to be rendered
            /// @todo

            // Create the render target view.
            /// @todo

            // Push the filled-in RenderBuffer onto the vector.
            /// @todo

            //==================================================================
            // Create a depth buffer

            // Make the depth/stencil texture.
            /// @todo

            // Create the depth/stencil view description
            /// @todo
        }

        // Create depth stencil state for the render path.
        // Describe how depth and stencil tests should be performed.
        /// @todo

        // Front-facing stencil operations (draw front faces)
        /// @todo

        // Back-facing stencil operations (cull back faces)
        /// @todo

        // Store the info about the buffers for the render callbacks.
        // Start with the 0th eye.
        /// @todo

        // Register the render buffers we're going to use to present
        /// @todo

        return ret;
    }

    bool RenderManagerVulkan::RenderPathSetup() {
        //======================================================
        // Construct the present buffers we're going to use when in Render()
        // mode, to wrap the PresentMode interface.
        if (!constructRenderBuffers()) {
            m_log->error() << "RenderManagerVulkan::RenderPathSetup: Could not "
                "construct present buffers to wrap Render() path";
            return false;
        }
        return true;
    }

    bool RenderManagerVulkan::RenderEyeInitialize(size_t eye) {
        // Bind our render target view to the appropriate one.
        /// @todo

        // Set the viewport for rendering to this eye.
        /// @todo

        // Call the display set-up callback for each eye, because they each
        // have their own frame buffer.
        /// @todo

        return true;
    }

    bool RenderManagerVulkan::RenderFrameFinalize() {
        return PresentRenderBuffersInternal(
            m_renderBuffers, m_renderInfoForRender, m_renderParamsForRender);
    }

    bool RenderManagerVulkan::PresentEye(PresentEyeParameters params) {

        if (params.m_buffer.Vulkan == nullptr) {
            m_log->error() << "RenderManagerVulkan::PresentEye(): NULL buffer pointer";
            return false;
        }

        /// @todo

        return true;
    }

    bool RenderManagerVulkan::PresentRenderBuffersInternal(
            const std::vector<RenderBuffer>& buffers,
            const std::vector<RenderInfo>& renderInfoUsed,
            const RenderParams& renderParams,
            const std::vector<OSVR_ViewportDescription>&
            normalizedCroppingViewports,
            bool flipInY) {
        /// @todo
        return true;
    }

    bool RenderManagerVulkan::UpdateDistortionMeshesInternal(
        DistortionMeshType type //< Type of mesh to produce
        ,
        std::vector<DistortionParameters> const&
        distort //< Distortion parameters
    ) {
        /// @todo

        return true;
    }

    bool RenderManagerVulkan::RenderSpace(size_t whichSpace, size_t whichEye,
        OSVR_PoseState pose,
        OSVR_ViewportDescription viewport,
        OSVR_ProjectionMatrix projection) {
        /// @todo Fill in the timing information
        OSVR_TimeValue deadline;
        deadline.microseconds = 0;
        deadline.seconds = 0;

        /// Fill in the information we pass to the render callback.
        RenderCallbackInfo& cb = m_callbacks[whichSpace];
        cb.m_callback(cb.m_userData, m_library, m_buffers, viewport, pose,
            projection, deadline);

        /// @todo Keep track of timing information

        return true;
    }

} // namespace renderkit
} // namespace osvr
