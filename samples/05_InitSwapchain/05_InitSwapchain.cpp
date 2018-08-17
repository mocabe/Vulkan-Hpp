// Copyright(c) 2018, NVIDIA CORPORATION. All rights reserved.
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
//
// VulkanHpp Samples : 05_InitSwapchain
//                     Initialize a swapchain

#include "vulkan/vulkan.hpp"
#include <iostream>

static char const* AppName = "05_InitSwapchain";
static char const* EngineName = "Vulkan.hpp";

template<class T, class Compare>
constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp)
{
  return assert(!comp(hi, lo)),
    comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
  return clamp(v, lo, hi, std::less<>());
}

static std::vector<char const*> getDeviceExtensions()
{
  std::vector<char const*> extensions;

  extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  return extensions;
}

static std::vector<char const*> getInstanceExtensions()
{
  std::vector<char const*> extensions;

  extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if  defined(VK_USE_PLATFORM_ANDROID_KHR)
  extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
  extensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
  extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MIR_KHR)
  extensions.push_back(VK_KHR_MIR_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_VI_NN)
  extensions.push_back(VK_NN_VI_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
  extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
  extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
  extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT)
  extensions.push_back(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME);
#endif

  return extensions;
}

#if defined(VK_USE_PLATFORM_WIN32_KHR)
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_CLOSE:
      PostQuitMessage(0);
      break;
    default:
      break;
  }
  return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

HWND initializeWindow(std::string const& className, std::string const& windowName, LONG width, LONG height)
{
  WNDCLASSEX windowClass;
  memset(&windowClass, 0, sizeof(WNDCLASSEX));

  HINSTANCE instance = GetModuleHandle(nullptr);
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = WindowProc;
  windowClass.hInstance = instance;
  windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  windowClass.lpszClassName = className.c_str();
  windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

  if (!RegisterClassEx(&windowClass))
  {
    throw std::runtime_error("Failed to register WNDCLASSEX -> terminating");
  }

  RECT windowRect = { 0, 0, width, height };
  AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

  HWND window = CreateWindowEx(0, className.c_str(), windowName.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU, 100, 100, windowRect.right - windowRect.left,
                              windowRect.bottom - windowRect.top, nullptr, nullptr, instance, nullptr);
  if (!window)
  {
    throw std::runtime_error("Failed to create window -> terminating");
  }

  return window;
}
#else
#pragma error "unhandled platform"
#endif


int main(int /*argc*/, char * /*argv[]*/)
{
  try
  {
    vk::ApplicationInfo appInfo(AppName, 1, EngineName, 1, VK_API_VERSION_1_1);
    std::vector<char const*> instanceExtensions = getInstanceExtensions();
    vk::InstanceCreateInfo instanceCreateInfo({}, &appInfo, 0, nullptr, static_cast<uint32_t>(instanceExtensions.size()), instanceExtensions.data());
    vk::UniqueInstance instance = vk::createInstanceUnique(instanceCreateInfo);

    std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();
    assert(!physicalDevices.empty());

    /* VULKAN_HPP_KEY_START */

    uint32_t width = 50;
    uint32_t height = 50;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    HWND window = initializeWindow(AppName, "Sample", width, height);

    vk::UniqueSurfaceKHR surface = instance->createWin32SurfaceKHRUnique(vk::Win32SurfaceCreateInfoKHR(vk::Win32SurfaceCreateFlagsKHR(), GetModuleHandle(nullptr), window));
#else
#pragma error "unhandled platform"
#endif

    // determine a queueFamilyIndex that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevices[0].getQueueFamilyProperties();
    size_t graphicsQueueFamilyIndex = std::distance(queueFamilyProperties.begin(),
      std::find_if(queueFamilyProperties.begin(),
        queueFamilyProperties.end(),
        [](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; }));

    // determine a queueFamilyIndex that suports present
    // first check if the graphicsQueueFamiliyIndex is good enough
    size_t presentQueueFamilyIndex = physicalDevices[0].getSurfaceSupportKHR(static_cast<uint32_t>(graphicsQueueFamilyIndex), surface.get()) ? graphicsQueueFamilyIndex : queueFamilyProperties.size();
    if (presentQueueFamilyIndex == queueFamilyProperties.size())
    {
      // the graphicsQueueFamilyIndex doesn't support present -> look for an other family index that supports both graphics and present
      for (size_t i = 0; i < queueFamilyProperties.size(); i++)
      {
        if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && physicalDevices[0].getSurfaceSupportKHR(static_cast<uint32_t>(i), surface.get()))
        {
          graphicsQueueFamilyIndex = i;
          presentQueueFamilyIndex = i;
          break;
        }
      }
      if (presentQueueFamilyIndex == queueFamilyProperties.size())
      {
        // there's nothing like a single family index that supports both graphics and present -> look for an other family index that supports present
        for (size_t i = 0; i < queueFamilyProperties.size(); i++)
        {
          if (physicalDevices[0].getSurfaceSupportKHR(static_cast<uint32_t>(i), surface.get()))
          {
            presentQueueFamilyIndex = i;
            break;
          }
        }
      }
    }
    if ((graphicsQueueFamilyIndex == queueFamilyProperties.size()) || (presentQueueFamilyIndex == queueFamilyProperties.size()))
    {
      throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
    }

    // create a device
    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, static_cast<uint32_t>(graphicsQueueFamilyIndex), 1, &queuePriority);
    std::vector<char const*> deviceExtensionNames = getDeviceExtensions();
    vk::UniqueDevice device = physicalDevices[0].createDeviceUnique(vk::DeviceCreateInfo({}, 1, &deviceQueueCreateInfo, 0, nullptr, static_cast<uint32_t>(deviceExtensionNames.size()), deviceExtensionNames.data()));

    // get the supported VkFormats
    std::vector<vk::SurfaceFormatKHR> formats = physicalDevices[0].getSurfaceFormatsKHR(surface.get());
    assert(!formats.empty());
    vk::Format format = (formats[0].format == vk::Format::eUndefined) ? vk::Format::eB8G8R8A8Unorm : formats[0].format;

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevices[0].getSurfaceCapabilitiesKHR(surface.get());

    VkExtent2D swapchainExtent;
    if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
      // If the surface size is undefined, the size is set to the size of the images requested.
      swapchainExtent.width = clamp(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
      swapchainExtent.height = clamp(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }
    else
    {
      // If the surface size is defined, the swap chain size must match
      swapchainExtent = surfaceCapabilities.currentExtent;
    }

    // The FIFO present mode is guaranteed by the spec to be supported
    vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

    vk::SurfaceTransformFlagBitsKHR preTransform = (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ? vk::SurfaceTransformFlagBitsKHR::eIdentity : surfaceCapabilities.currentTransform;

    vk::CompositeAlphaFlagBitsKHR compositeAlpha =
      (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
      (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
      (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit : vk::CompositeAlphaFlagBitsKHR::eOpaque;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo(vk::SwapchainCreateFlagsKHR(), surface.get(), surfaceCapabilities.minImageCount, format, vk::ColorSpaceKHR::eSrgbNonlinear,
      swapchainExtent, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, preTransform, compositeAlpha, swapchainPresentMode, true, nullptr);

    uint32_t queueFamilyIndices[2] = { static_cast<uint32_t>(graphicsQueueFamilyIndex), static_cast<uint32_t>(presentQueueFamilyIndex) };
    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
    {
      // If the graphics and present queues are from different queue families, we either have to explicitly transfer ownership of images between
      // the queues, or we have to create the swapchain with imageSharingMode as VK_SHARING_MODE_CONCURRENT
      swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
      swapChainCreateInfo.queueFamilyIndexCount = 2;
      swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    vk::UniqueSwapchainKHR swapChain = device->createSwapchainKHRUnique(swapChainCreateInfo);

    std::vector<vk::Image> swapChainImages = device->getSwapchainImagesKHR(swapChain.get());

    std::vector<vk::UniqueImageView> imageViews;
    imageViews.reserve(swapChainImages.size());
    for (auto image : swapChainImages)
    {
      vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
      vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
      vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, format, componentMapping, subResourceRange);
      imageViews.push_back(device->createImageViewUnique(imageViewCreateInfo));
    }

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    DestroyWindow(window);
#else
#pragma error "unhandled platform"
#endif

    // Note: No need to explicitly destroy the ImageViews or the swapChain, as the corresponding destroy
    // functions are called by the destructor of the UniqueImageView and the UniqueSwapChainKHR on leaving this scope.

    /* VULKAN_HPP_KEY_END */
  }
  catch (vk::SystemError err)
  {
    std::cout << "vk::SystemError: " << err.what() << std::endl;
    exit(-1);
  }
  catch (std::runtime_error err)
  {
    std::cout << "std::runtime_error: " << err.what() << std::endl;
    exit(-1);
  }
  catch (...)
  {
    std::cout << "unknown error\n";
    exit(-1);
  }
  return 0;
}
