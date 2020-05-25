#ifndef __UE_VULKAN_TOOLS_H__
#define __UE_VULKAN_TOOLS_H__

#ifndef __UE_VK_VERBOSE__
#define __UE_VK_VERBOSE__ 1
#endif // __UE_VK_VERBOSE__

#if __UE_VK_VERBOSE__
#define MAX_VKVERBOSE_LEN 256
char _vkverbose_buffer[MAX_VKVERBOSE_LEN];
char _message_buffer[MAX_VKVERBOSE_LEN];
#define uVkVerbose(...)                                         \
    snprintf(_message_buffer, MAX_VKVERBOSE_LEN, __VA_ARGS__);  \
    snprintf(_vkverbose_buffer,                                 \
             MAX_VKVERBOSE_LEN,                                 \
             "[ vulkan ] %s",                                   \
             _message_buffer);                                  \
    fputs(_vkverbose_buffer, stdout);                           \
    fflush(stdout);
#else
#define uVkVerbose(...) /* uVKVerbose() REMOVED */
#endif // __UE_VK_VERBOSE__

#include <vulkan/vulkan.h>
#include <engine_tools/memory_tools.h>
#include <data_structures/data_structures.h>



typedef struct
{
    u32 graphics_index;
} uVulkanQueueFamilyIndices;


typedef struct
{
    const VkInstance       instance;
    const VkPhysicalDevice physical_device;
    const VkDevice         logical_device;
    const VkQueue          graphics_queue;
} uVulkanInfo;


VkDebugUtilsMessengerEXT           vulkan_main_debug_messenger;
VkDebugUtilsMessengerCreateInfoEXT vulkan_main_debug_messenger_info  = { 0 };
VkDebugUtilsMessengerCreateInfoEXT vulkan_setup_debug_messenger_info = { 0 };


__UE_internal__ __UE_call__ void
uCreateVulkanLogicalDevice(_mut_ uVulkanInfo*               const restrict v_info,
                           const uVulkanQueueFamilyIndices* const restrict queue_family_indices)
{
    uAssertMsg_v(v_info,                  "[ vulkan ] Null vulkan info ptr provided.\n");
    uAssertMsg_v(v_info->physical_device, "[ vulkan ] Physical device must be non null.\n");
    uAssertMsg_v(!v_info->logical_device, "[ vulkan ] Logical device must be null; will be overwritten.\n");
    uAssertMsg_v(queue_family_indices,    "[ vulkan ] Queue family indices ptr must be non null\n");


    VkDeviceQueueCreateInfo device_queue_create_info = { 0 };
    r32 device_queue_priorities = 1.0f;
    device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    device_queue_create_info.queueFamilyIndex = queue_family_indices->graphics_index;
    device_queue_create_info.queueCount = 1;
    device_queue_create_info.pQueuePriorities = &device_queue_priorities;

    // [ cfarvin::TODO ] When modified, don't forget to add checks to uSelectVulkanPhysicalDevice();
    VkPhysicalDeviceFeatures physical_device_features = { 0 };

    VkDeviceCreateInfo logical_device_create_info = { 0 };
    logical_device_create_info.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logical_device_create_info.pQueueCreateInfos    = &device_queue_create_info;
    logical_device_create_info.queueCreateInfoCount = 1;
    logical_device_create_info.pEnabledFeatures     = &physical_device_features;

    // Note: modern Vulkan implementations no longer separate instance and device layers.
    VkResult device_creation_success = vkCreateDevice(v_info->physical_device,
                                                      &logical_device_create_info,
                                                      NULL,
                                                      (VkDevice*)&v_info->logical_device);

    uAssertMsg_v(device_creation_success == VK_SUCCESS, "[ vulkan ] Unable to create logical device.\n");
    if (device_creation_success != VK_SUCCESS)
    {
        VkDevice* non_const_logical_device = (VkDevice*) &(v_info->logical_device);
        *non_const_logical_device = NULL;
    }

    // Get queue handles
    vkGetDeviceQueue(v_info->logical_device,
                     queue_family_indices->graphics_index,
                     0,
                     (VkQueue*)(&v_info->graphics_queue));
}


__UE_internal__ __UE_call__ void
uSelectVulkanPhysicalDevice(const VkPhysicalDevice**         const const restrict physical_device_list,
                            _mut_ VkPhysicalDevice*          _mut_       restrict return_device,
                            _mut_ uVulkanQueueFamilyIndices* const       restrict queue_family_indices,
                            const u32                                             num_physical_devices)
{
    uAssertMsg_v(physical_device_list, "[ vulkan ] Null vulkan device list pointer provided.\n");
    uAssertMsg_v(!(*return_device)   , "[ vulkan ] Return device ptr must be NULL; will be overwritten\n");
    uAssertMsg_v(queue_family_indices, "[ vulkan ] Queue family indices ptr must be non null\n");
    uAssertMsg_v(num_physical_devices, "[ vulkan ] A minimum of one physical devices is required.\n");
    for (u32 device_idx = 0; device_idx < num_physical_devices; device_idx++)
    {
        uAssertMsg_v(physical_device_list[device_idx],
                     "[ vulkan ] Indices of physical_device_list must be non-null.\n");
    }

    for (u32 device_idx = 0; device_idx < num_physical_devices; device_idx++)
    {
        VkPhysicalDevice physical_device = *physical_device_list[device_idx];
        if (!physical_device)
        {
            continue;
        }

        // [ cfarvin::TODO ]
        // Aquire physical device features
        /* VkPhysicalDeviceFeatures device_features; */
        /* vkGetPhysicalDeviceFeatures(physical_device, */
        /*                             &device_features); */

        // [ cfarvin::TODO ]
        // Aquire physical device properties
        /* VkPhysicalDeviceProperties device_properties; */
        /* vkGetPhysicalDeviceProperties(physical_device, */
        /*                               &device_properties); */

        // Inspect physical device queue families
        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
        if (!queue_family_count)
        {
            continue;
        }

        VkQueueFamilyProperties* queue_family_props = (VkQueueFamilyProperties*) calloc(queue_family_count,
                                                                                        sizeof(VkQueueFamilyProperties));
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_props);

        // Note: add more as necessary, don't forget to add to break condition.
        typedef struct
        {
            u32  index;
            bool validated;
        } queue_family_pair;

        bool queue_family_validated = false;
        queue_family_pair graphics_pair = { 0 };
        for (u32 queue_idx = 0; queue_idx < queue_family_count; queue_idx++)
        {
            // Require graphics bit
            if (queue_family_props[queue_idx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphics_pair.index = queue_idx;
                graphics_pair.validated = true;
            }

            // Break when all conditions are met
            if (graphics_pair.validated)
            {
                queue_family_validated = true;
                break;
            }
        }

        if (!queue_family_validated)
        {
            continue;
        }

        //
        // Device is deemed suitable
        //
        queue_family_indices->graphics_index = graphics_pair.index;
        *return_device = physical_device;
        break;
    }
}


__UE_internal__ __UE_call__ void
uCreateVulkanPysicalDevice(_mut_ uVulkanInfo*               const restrict v_info,
                           _mut_ uVulkanQueueFamilyIndices* const restrict queue_family_indices)
{
    uAssertMsg_v(v_info,                   "[ vulkan ] Null vulkan info ptr provided.\n");
    uAssertMsg_v(!v_info->physical_device, "[ vulkan ] Physical device must be null; will be overwritten.\n");
    uAssertMsg_v(!v_info->logical_device,  "[ vulkan ] Logical device must be null; will be overwritten.\n");
    uAssertMsg_v(queue_family_indices,     "[ vulkan ] Queue family indices ptr must be non null.\n");


    u32 num_physical_devices = 0;
    vkEnumeratePhysicalDevices(v_info->instance, &num_physical_devices, NULL);

    uAssertMsg_v(num_physical_devices, "[ vulkan ] No physical devices found.\n");
    if (!num_physical_devices)
    {
        return;
    }

    VkPhysicalDevice* physical_device_list = (VkPhysicalDevice*)calloc(num_physical_devices,
                                                                       sizeof(VkPhysicalDevice));
    uAssertMsg_v(physical_device_list, "[ vulkan ] Unable to allocate physical device list.\n");

    vkEnumeratePhysicalDevices(v_info->instance, &num_physical_devices, physical_device_list);
    uVkVerbose("Found %d physical devices.\n", num_physical_devices);

    VkPhysicalDevice          candidate_device = NULL;
    uSelectVulkanPhysicalDevice(&physical_device_list,
                                &candidate_device,
                                queue_family_indices,
                                num_physical_devices);
    uAssertMsg_v(candidate_device != NULL, "[ vulkan ] Unable to select candidate device.\n");
    if (!candidate_device)
    {
        return;
    }

    VkPhysicalDevice* non_const_physical_device = (VkPhysicalDevice*)&(v_info->physical_device);
    *non_const_physical_device = candidate_device;
}


// Note: no function/argument decorations to conform w/ Vulkan spec.
static VKAPI_ATTR VkBool32 VKAPI_CALL
uVkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity_bits,
                 VkDebugUtilsMessageTypeFlagsEXT             message_type_bits,
                 const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                 void*                                       user_data)
{
    VkBool32 should_abort_calling_process = VK_FALSE;
    if(user_data) {} // Silence warnings

    if (message_severity_bits >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
        message_type_bits     >= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        printf("[ vulkan ] [ validation ] %s\n", callback_data->pMessage);
        fflush(stdout);
    }

    return should_abort_calling_process;
}


__UE_internal__ __UE_call__ void
uCreateVulkanDebugMessengerInfo(_mut_ VkDebugUtilsMessengerCreateInfoEXT* const restrict debug_message_create_info)
{
    uAssertMsg_v(debug_message_create_info,
                 "[ vulkan ] Null VkDebugUtilsMessengerCreateInfoEXT ptr provided.\n");


    debug_message_create_info->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    debug_message_create_info->messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    debug_message_create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_message_create_info->pfnUserCallback = uVkDebugCallback;
    debug_message_create_info->pUserData = NULL;
}


__UE_internal__ __UE_call__ void
uCreateVulkanDebugMessenger(const uVulkanInfo* const restrict v_info,
                            _mut_ VkDebugUtilsMessengerCreateInfoEXT* const restrict debug_message_create_info,
                            _mut_ VkDebugUtilsMessengerEXT*           const restrict debug_messenger)
{
    uAssertMsg_v(v_info,           "[ vulkan ] Null uVulkanInfo ptr provided.\n");
    uAssertMsg_v(v_info->instance, "[ vulkan ] Null uVulkanInfo->instance ptr provided.\n");
    uAssertMsg_v(debug_message_create_info,
                 "[ vulkan ] Null VkDebugUtilsMessengerCreateInfoEXT ptr provided.\n");


    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(v_info->instance,
                                                                  "vkCreateDebugUtilsMessengerEXT");

    uAssertMsg_v(vkCreateDebugUtilsMessengerEXT,
                 "[ vulkan ] Failed to acquire pfn: vkCreateDebugUtilsMessengerEXT\n");

    if (vkCreateDebugUtilsMessengerEXT)
    {
        uCreateVulkanDebugMessengerInfo((VkDebugUtilsMessengerCreateInfoEXT*)debug_message_create_info);

        VkResult success =
            vkCreateDebugUtilsMessengerEXT(v_info->instance,
                                           debug_message_create_info,
                                           NULL,
                                           debug_messenger);
        uAssertMsg_v(((success == VK_SUCCESS) && debug_messenger),
                     "[ vulkan ] Failed to create debug messenger callback.\n");
    }
}


__UE_internal__ __UE_call__ void
uQueryVulkanLayers(_mut_ s8***                 _mut_ const restrict layer_names,
                   _mut_ VkLayerProperties*    _mut_       restrict layer_properties,
                   _mut_ VkInstanceCreateInfo* const       restrict instance_create_info,
                   const s8**                  const const restrict user_validation_layers,
                   const u32 num_user_layers)
{
    uAssertMsg_v(instance_create_info,   "[ vulkan ] Null InstanceCreateInfo ptr provided.\n");
    uAssertMsg_v(!(*layer_names),        "[ vulkan ] Layer names ptr ptr must be null; will be overwritten.\n");
    uAssertMsg_v(!layer_properties,      "[ vulkan ] VkLayerProperties ptr must be null; will be overwritten.\n");
    uAssertMsg_v(user_validation_layers, "[ vulkan ] Null requested validation layer ptr.\n");


    if (num_user_layers == 0)
    {
        return;
    }

    // Query Layer Count
    VkResult success = VK_SUCCESS;
    size_t num_available_layers = 0;
    success = vkEnumerateInstanceLayerProperties(&(u32)num_available_layers,
                                                 NULL);
    uAssertMsg_v((success == VK_SUCCESS), "[ vulkan ] Unable to enumerate layers.\n");
    uAssertMsg_v((num_available_layers >= num_user_layers),
                 "[ vulkan ] Number of requested validation layers [ %d ] exceeds total avaialbe count [ %zd ].\n",
                 num_user_layers,
                 num_available_layers);


    layer_properties =
        (VkLayerProperties*)malloc(num_available_layers * sizeof(VkLayerProperties));

    // Query Layer Names
    success = vkEnumerateInstanceLayerProperties(&(u32)num_available_layers,
                                                 layer_properties);
    uAssertMsg_v((success == VK_SUCCESS), "[ vulkan ] Unable to enumerate layers.\n");


    // Set Layer Names
    uVkVerbose("Searching for validation layers...\n");
    u32 num_added_layers = 0;
    *layer_names = (s8**)malloc(num_available_layers * sizeof(s8**));
    for (u32 available_layer_idx = 0;
         available_layer_idx < num_available_layers;
         available_layer_idx++)
    {
        for (u32 user_layer_idx = 0;
             user_layer_idx < num_user_layers;
             user_layer_idx++)
        {
            uVkVerbose("\tLayer found: %s\n", layer_properties[available_layer_idx].layerName);
            if (strcmp((const char*)user_validation_layers[user_layer_idx],
                       (const char*)layer_properties[available_layer_idx].layerName) == 0)
            {
                (*layer_names)[num_added_layers] = (s8*)layer_properties[available_layer_idx].layerName;
                num_added_layers++;
            }
        }
    }

    uAssertMsg_v((num_added_layers == num_user_layers), "[ vulkan ] Unable to load all requested layers.\n");
    instance_create_info->enabledLayerCount   = num_added_layers;
    instance_create_info->ppEnabledLayerNames = *layer_names;
}



__UE_internal__ __UE_call__ void
uQueryVulkanExtensions(_mut_ s8***                  _mut_       restrict extension_names,
                       _mut_ VkExtensionProperties* _mut_       restrict extension_properties,
                       _mut_ VkInstanceCreateInfo*  const       restrict instance_create_info,
                       const s8**                   const const restrict user_extensions,
                       const u16                                         num_user_extensions)
{
    // Note (error checking): It is legal to request no required validation layers and no requried extensions.
    uAssertMsg_v(instance_create_info,  "[ vulkan ] Null InstanceCreateInfo ptr provided.\n");
    uAssertMsg_v(!(*extension_names),   "[ vulkan ] Extension names ptr ptr must be null; will be overwritten.\n");
    uAssertMsg_v(!extension_properties, "[ vulkan ] VkExtensionProperties ptr must be null; will be overwritten.\n");


    // Query Extension Count
    VkResult success = VK_SUCCESS;
    success = vkEnumerateInstanceExtensionProperties(NULL,
                                                     &instance_create_info->enabledExtensionCount,
                                                     NULL);
    uAssertMsg_v((success == VK_SUCCESS), "[ vulkan ] Unable to enumerate extensions.\n");

    extension_properties =
        (VkExtensionProperties*)malloc(instance_create_info->enabledExtensionCount * sizeof(VkExtensionProperties));

    // Query Extension Names
    success = vkEnumerateInstanceExtensionProperties(NULL,
                                                     &instance_create_info->enabledExtensionCount,
                                                     extension_properties);
    uAssertMsg_v((success == VK_SUCCESS), "[ vulkan ] Unable to enumerate extensions.\n");


    // Set Extension Names
    uVkVerbose("Searching for extensions...\n");
    u32 num_added_extensions = 0;
    *extension_names = (s8**)malloc(instance_create_info->enabledExtensionCount * sizeof(s8**));
    for (u32 ext_idx = 0; ext_idx < instance_create_info->enabledExtensionCount; ext_idx++)
    {
        uVkVerbose("\tExtension found: %s\n", extension_properties[ext_idx].extensionName);
        for (u32 user_ext_idx = 0;
             user_ext_idx < num_user_extensions;
             user_ext_idx++)
        {
            if (strcmp((const char*)user_extensions[user_ext_idx],
                       (const char*)extension_properties[ext_idx].extensionName) == 0)
            {
                (*extension_names)[num_added_extensions] = (s8*)extension_properties[ext_idx].extensionName;
                num_added_extensions++;
            }
        }
    }

    uAssertMsg_v((num_added_extensions == num_user_extensions), "[ vulkan ] Unable to load all requested extensions.\n");
    instance_create_info->enabledExtensionCount   = num_added_extensions;
    instance_create_info->ppEnabledExtensionNames = *extension_names;
}


__UE_internal__ __UE_call__ void
uCreateVulkanInstance(const uVulkanInfo*       const       restrict v_info,
                      const VkApplicationInfo* const       restrict application_info,
                      const s8**               const const restrict user_validation_layers,
                      const u16                                     num_user_layers,
                      const s8**               const const restrict user_extensions,
                      const u16                                     num_user_extensions)

{
    // Note (error checking): It is legal to request no required validation layers and no requried extensions.
    uAssertMsg_v(application_info, "[ vulkan ] Null application info ptr provided.\n");
    VkExtensionProperties* extension_properties = NULL;
    VkLayerProperties*     layer_properties     = NULL;
    s8**                   extension_names      = NULL;
    s8**                   layer_names          = NULL;


    uCreateVulkanDebugMessengerInfo(&vulkan_setup_debug_messenger_info);

    VkResult success = VK_SUCCESS;
    VkInstanceCreateInfo instance_create_info = { 0 };

    uQueryVulkanExtensions(&extension_names,
                           extension_properties,
                           &instance_create_info,
                           user_extensions,
                           num_user_extensions);

    uQueryVulkanLayers(&layer_names,
                       layer_properties,
                       &instance_create_info,
                       user_validation_layers,
                       num_user_layers);

    instance_create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = application_info;
    instance_create_info.pNext            = &vulkan_setup_debug_messenger_info;
    success = vkCreateInstance(&instance_create_info,
                               NULL,
                               (VkInstance*)&v_info->instance);

    uAssertMsg_v((success == VK_SUCCESS), "[ vulkan ] Unable to create vulkan instance.\n");
    uCreateVulkanDebugMessenger(v_info,
                                (VkDebugUtilsMessengerCreateInfoEXT*)&vulkan_main_debug_messenger_info,
                                (VkDebugUtilsMessengerEXT*)&vulkan_main_debug_messenger);

    if (extension_names)
    {
        free(extension_names);
    }

    if (layer_names)
    {
        free(layer_names);
    }

}


__UE_internal__ __UE_call__ void
uCreateVulkanApplicationInfo(const s8*                const restrict application_name,
                             _mut_ VkApplicationInfo* const restrict application_info)
{
    uAssertMsg_v(application_name, "[ vulkan ] Null application name ptr provided.\n");
    uAssertMsg_v(application_info, "[ vulkan ] Null application info ptr provided.\n");


    application_info->sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info->pApplicationName   = application_name;
    application_info->applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info->pEngineName        = "Understone Engine";
    application_info->apiVersion         = VK_API_VERSION_1_0;
}


__UE_internal__ __UE_call__ void
uInitializeVulkan(_mut_ uVulkanInfo* const       restrict v_info,
                  const s8*          const       restrict application_name,
                  const s8**         const const restrict validation_layers,
                  const u16                               num_layers,
                  const s8**         const const restrict extensions,
                  const u16                               num_extensions)
{
    // Note (error checking): It is legal to request no required validation layers and no requried extensions.
    uAssertMsg_v(v_info,                   "[ vulkan ] Null v_info ptr provided.\n");
    uAssertMsg_v(!v_info->instance,        "[ vulkan ] Instance must be null; will be overwritten.\n");
    uAssertMsg_v(!v_info->physical_device, "[ vulkan ] Physical device must be null; will be overwritten.\n");
    uAssertMsg_v(!v_info->logical_device,  "[ vulkan ] Logical device must be null; will be overwritten.\n");
    uAssertMsg_v(application_name,         "[ vulkan ] Null application name ptr provided.\n");
    VkApplicationInfo application_info = { 0 };


    uCreateVulkanApplicationInfo(application_name, &application_info);
    uCreateVulkanInstance(v_info,
                          &application_info,
                          validation_layers,
                          num_layers,
                          extensions,
                          num_extensions);

    uVulkanQueueFamilyIndices queue_family_indices = { 0 };
    uCreateVulkanPysicalDevice(v_info, &queue_family_indices);
    uCreateVulkanLogicalDevice(v_info, &queue_family_indices);
}


__UE_internal__ __UE_call__ void
uDestroyVulkan(const uVulkanInfo* const restrict v_info)
{
    uAssertMsg_v(v_info,           "Null uVulkanInfo ptr provided.\n");
    uAssertMsg_v(v_info->instance, "Null instance ptr provided.\n");


    if (v_info && v_info->instance)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(v_info->instance,
                                                                       "vkDestroyDebugUtilsMessengerEXT");

        uAssertMsg_v(vkDestroyDebugUtilsMessengerEXT,
                     "[ vulkan ] Unable to acquire ptr to function: vkDestroyDebugUtilsMessengerEXT().\n");

        if (vkDestroyDebugUtilsMessengerEXT && vulkan_main_debug_messenger)
        {
            vkDestroyDebugUtilsMessengerEXT(v_info->instance,
                                            vulkan_main_debug_messenger,
                                            NULL);
        }
    }

    if (v_info && v_info->logical_device)
    {
        vkDestroyDevice(v_info->logical_device, NULL);

    }

    if (v_info && v_info->instance)
    {
        vkDestroyInstance(v_info->instance, NULL);
    }
}

#endif // __UE_VULKAN_TOOLS_H__

#if 0
/*

  To Draw A Triangle:
  - Create a VkInstance
  - Select a supported graphics card (VkPhysicalDevice)
  - Create a VkDevice and VkQueue for drawing and presentation
  - Create a window, window surface and swap chain
  - Wrap the swap chain images into VkImageView
  - Create a render pass that specifies the render targets and usage
  - Create framebuffers for the render pass
  - Set up the graphics pipeline
  - Allocate and record a command buffer with the draw commands for every possible swap chain image
  - Draw frames by acquiring images, submitting the right draw command buffer and returning the images back to the swap chain

  NOTES:
  - Skip window manager (or build a cusom one) with VK_KHR_display && VK_KHR_display_swapchain
  This will render fullscreen.

*/
#endif // if 0
