/** @file
@brief Header file describing the OSVR Vulkan graphics library callback info

@date 2020

@author
Russ Taylor <russ@reliaolve.com>
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

// Required for DLL linkage on Windows
#include <osvr/RenderKit/Export.h>

namespace osvr {
namespace renderkit {

    /// @brief Describes a Vulkan rendering library being used
    ///
    /// This is one of the members of the GraphicsLibrary union
    /// from RenderManager.h.  It stores the information needed
    /// for a render callback handler in an application using Vulkan
    /// as its renderer.  It is in a separate include file so
    /// that only code that actually uses this needs to
    /// include it.  NOTE: You must #include <vulkan/vulkan.hpp> before
    /// including this file.

    class GraphicsLibraryVulkan {
      public:
        /// @todo Add device and context pointers
    };

    /// @brief Describes a Vulkan texture to be rendered
    ///
    /// This is one of the members of the RenderBuffer union
    /// from RenderManager.h.  It stores the information needed
    /// for a Vulkan texture.
    /// NOTE: You must #include <vulkan/vulkan.hpp> before
    /// including this file.

    class RenderBufferVulkan {
      public:
        /// @todo Add color buffer, view, depth buffer, and view pointers
    };

} // namespace renderkit
} // namespace osvr
