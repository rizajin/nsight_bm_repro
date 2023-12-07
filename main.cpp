#include "NGFX_Injection.h"

#include <vulkan/vulkan.h>

#include <fstream>
#include <vector>
#include <iostream>
#include <string>

std::vector<char*> layers{
    "VK_LAYER_KHRONOS_validation",
};

std::vector<char*> extensions{
    VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
    VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
};

int main()
{
    // NSight injection
    uint32_t nsightCount{};
    NGFX_Injection_Result injectionResult = NGFX_Injection_EnumerateInstallations(&nsightCount, nullptr);
    if (injectionResult != NGFX_INJECTION_RESULT_OK)
    {
        return 1;
    }
    std::vector<NGFX_Injection_InstallationInfo> nsightInstallations(nsightCount);
    injectionResult = NGFX_Injection_EnumerateInstallations(&nsightCount, nsightInstallations.data());
    if (injectionResult != NGFX_INJECTION_RESULT_OK)
    {
        return 1;
    }

    uint32_t nsightActivityCount{};
    injectionResult = NGFX_Injection_EnumerateActivities(&nsightInstallations[0], &nsightActivityCount, nullptr);
    if (injectionResult != NGFX_INJECTION_RESULT_OK)
    {
        return 1;
    }
    std::vector<NGFX_Injection_Activity> nsightActivities(nsightActivityCount);
    injectionResult = NGFX_Injection_EnumerateActivities(&nsightInstallations[0], &nsightActivityCount, nsightActivities.data());
    if (injectionResult != NGFX_INJECTION_RESULT_OK)
    {
        return 1;
    }

    injectionResult = NGFX_Injection_InjectToProcess(&nsightInstallations[0], &nsightActivities[0]);
    if (injectionResult != NGFX_INJECTION_RESULT_OK)
    {
        std::cout << "Injection was not succesful, driver is loaded already" << std::endl;
        return 1;
    }

    // Vk
    VkInstance inst;
    VkDevice device;

    VkApplicationInfo aci{};
    aci.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    aci.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0);
    VkInstanceCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.flags = 0;
    ici.pApplicationInfo = &aci;
    uint32_t ilp{};
    vkEnumerateInstanceLayerProperties(&ilp, nullptr);
    std::vector<VkLayerProperties> lp(ilp);
    vkEnumerateInstanceLayerProperties(&ilp, lp.data());
    std::cout << "Layers: " << std::endl;
    for (const auto& layerProp : lp)
    {
        std::cout << layerProp.layerName << std::endl;

        uint32_t iepc{};
        vkEnumerateInstanceExtensionProperties(layerProp.layerName, &iepc, nullptr);
        std::vector<VkExtensionProperties> ep(iepc);
        vkEnumerateInstanceExtensionProperties(layerProp.layerName, &iepc, ep.data());
        for (const auto& extProp : ep)
        {
            std::cout << "    extension: " << extProp.extensionName << std::endl;
        }
    }

    ici.enabledExtensionCount = 0;
    ici.ppEnabledExtensionNames = nullptr;
    ici.enabledLayerCount = layers.size();
    ici.ppEnabledLayerNames = layers.data();

    auto r = vkCreateInstance(&ici, nullptr, &inst);
    if (r != VK_SUCCESS) { return 1; }

    uint32_t pdc{};
    vkEnumeratePhysicalDevices(inst, &pdc, nullptr);
    std::vector<VkPhysicalDevice> pds(pdc);
    vkEnumeratePhysicalDevices(inst, &pdc, pds.data());

    uint32_t phlc{};
    vkEnumerateDeviceLayerProperties(pds[0], &phlc, nullptr);
    std::vector<VkLayerProperties> pdlp(phlc);
    vkEnumerateDeviceLayerProperties(pds[0], &phlc, pdlp.data());
    std::cout << "\nDevice Layers: " << std::endl;
    for (const auto& physDevLayerProp : pdlp)
    {
        std::cout << physDevLayerProp.layerName << std::endl;
        uint32_t pdlpc{};
        vkEnumerateDeviceExtensionProperties(pds[0], physDevLayerProp.layerName, &pdlpc, nullptr);
        std::vector<VkExtensionProperties> pdep(pdlpc);
        vkEnumerateDeviceExtensionProperties(pds[0], physDevLayerProp.layerName, &pdlpc, pdep.data());
        for (const auto& physDevLayerExtProp : pdep)
        {
            std::cout << "    " << physDevLayerExtProp.extensionName << std::endl;
        }
    }

    VkDeviceDiagnosticsConfigCreateInfoNV diagnosticsConfig{};
    diagnosticsConfig.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
    diagnosticsConfig.flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV |
                              VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
                              VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV;


    VkPhysicalDeviceBufferDeviceAddressFeatures pdbdaf{};
    pdbdaf.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    pdbdaf.bufferDeviceAddressCaptureReplay = VK_TRUE;
    pdbdaf.bufferDeviceAddress = VK_TRUE;
    pdbdaf.pNext = &diagnosticsConfig;

    VkDeviceQueueCreateInfo dqci{};
    dqci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    dqci.flags = 0;
    dqci.queueCount = 1;
    float prio = 1.0f;
    dqci.pQueuePriorities = &prio;

    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.flags = 0;
    dci.enabledExtensionCount = extensions.size();
    dci.ppEnabledExtensionNames = extensions.data();
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &dqci;
    dci.pNext = &pdbdaf;

    r = vkCreateDevice(pds[0], &dci, nullptr, &device);
    if (r != VK_SUCCESS)
    {
        return 1;
    }

    NGFX_Injection_ExecuteActivityCommand();

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(inst, nullptr);

    return 0;
}