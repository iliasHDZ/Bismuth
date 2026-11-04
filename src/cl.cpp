#include "cl.hpp"

using namespace geode::prelude;

#define CL_NO_CORE_PROTOTYPES
#define CL_TARGET_OPENCL_VERSION 310
#include <CL/cl_icd.h>

static HMODULE openclDll = NULL;

static clGetPlatformIDs_t* clGetPlatformIDs = nullptr;
static clGetExtensionFunctionAddressForPlatform_t* clGetExtensionFunctionAddressForPlatform = nullptr;
static clGetPlatformInfo_t* clGetPlatformInfo = nullptr;
static clGetDeviceIDs_t* clGetDeviceIDs = nullptr;
static clGetDeviceInfo_t* clGetDeviceInfo = nullptr;

bool fetchOpenCLPlatformIds() {
    if (!openclDll)
        openclDll = LoadLibraryA("OpenCL.dll");

    if (!openclDll)
        return false;

    clGetPlatformIDs = (clGetPlatformIDs_t*)GetProcAddress(openclDll, "clGetPlatformIDs");
    clGetExtensionFunctionAddressForPlatform = (clGetExtensionFunctionAddressForPlatform_t*)GetProcAddress(openclDll, "clGetExtensionFunctionAddressForPlatform");
    clGetPlatformInfo = (clGetPlatformInfo_t*)GetProcAddress(openclDll, "clGetPlatformInfo");
    clGetDeviceIDs = (clGetDeviceIDs_t*)GetProcAddress(openclDll, "clGetDeviceIDs");
    clGetDeviceInfo = (clGetDeviceInfo_t*)GetProcAddress(openclDll, "clGetDeviceInfo");

    return clGetPlatformIDs && clGetExtensionFunctionAddressForPlatform && clGetPlatformInfo && clGetDeviceIDs && clGetDeviceInfo;
}

static clGetGLContextInfoKHR_t* my_clGetGLContextInfoKHR = nullptr;

static bool gpuCoreCountFetched = false;
static bool fetchFailed = false;
static isize gpuCoreCount = -1;

static std::string getPlatformName(cl_platform_id id) {
    size_t count;
    cl_int err = clGetPlatformInfo(id, CL_PLATFORM_NAME, 0, NULL, &count);
    if (err != CL_SUCCESS)
        return "<invalid>";

    std::string name;
    name.resize(count, ' ');
    err = clGetPlatformInfo(id, CL_PLATFORM_NAME, count, name.data(), NULL);
    if (err != CL_SUCCESS)
        return "<invalid>";

    return name;
}

static std::string getDeviceName(cl_device_id id) {
    size_t count;
    cl_int err = clGetDeviceInfo(id, CL_DEVICE_NAME, 0, NULL, &count);
    if (err != CL_SUCCESS)
        return "<invalid>";

    std::string name;
    name.resize(count, ' ');
    err = clGetDeviceInfo(id, CL_DEVICE_NAME, count, name.data(), NULL);
    if (err != CL_SUCCESS)
        return "<invalid>";

    return name;
}

usize getNVidiaCudaCoresPerSM(cl_uint major, cl_uint minor) {
    switch (major) {
        case 2: return (minor == 1) ? 48 : 32;
        case 3: return 192;
        case 5: return 128;
        case 6: return (minor == 1 || minor == 2) ? 128 : 64;
        case 7: return 64;
        case 8: return (minor == 6 || minor == 9) ? 128 : 64;
        case 9: return 128;
        default: return 128;
    }
}

bool fetchGPUCoreCountRaw(cl_device_id deviceID) {
    cl_uint coreCount;
    cl_int err = clGetDeviceInfo(
        deviceID, 
        CL_DEVICE_MAX_COMPUTE_UNITS, 
        sizeof(coreCount), 
        &coreCount, 
        NULL
    );

    if (err != CL_SUCCESS)
        return false;

    log::info("CL_DEVICE_MAX_COMPUTE_UNITS: {}", coreCount);
    
    cl_uint nvMajor, nvMinor;
    cl_int err1 = clGetDeviceInfo(deviceID, CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV, sizeof(cl_uint), &nvMajor, NULL);
    cl_int err2 = clGetDeviceInfo(deviceID, CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV, sizeof(cl_uint), &nvMinor, NULL);

    if (err1 == CL_SUCCESS && err2 == CL_SUCCESS) {
        usize cudaCoresPerSM = getNVidiaCudaCoresPerSM(nvMajor, nvMinor);
        log::info("NVidia GPU detected, CUDA core count: {} * {} = {}", coreCount, cudaCoresPerSM, coreCount * cudaCoresPerSM);
        coreCount *= cudaCoresPerSM;
    }

    if (isIntelGPU())
        coreCount *= 2; // TODO: Check if this is a stupid thing to do

    log::info("Detected core count: {}", coreCount);

    gpuCoreCount = coreCount;
    return true;
}

bool fetchGPUCoreCount() {
    if (!fetchOpenCLPlatformIds()) {
        fetchFailed = true;
        log::warn("Failed to fetch core count: Could not load OpenCL");
        return false;
    }

    cl_uint platformCount;
    clGetPlatformIDs(0, NULL, &platformCount);

    std::vector<cl_platform_id> platformIDs;
    platformIDs.resize(platformCount);
    clGetPlatformIDs(platformCount, platformIDs.data(), NULL);

    for (auto platformID : platformIDs) {
        std::string name = getPlatformName(platformID);

        my_clGetGLContextInfoKHR = (clGetGLContextInfoKHR_t*)clGetExtensionFunctionAddressForPlatform(platformID, "clGetGLContextInfoKHR");
        
        if (!my_clGetGLContextInfoKHR) {
            log::warn("Failed to fetch 'clGetGLContextInfoKHR' from platform {}", name);
            continue;
        }

        cl_context_properties props[] = {
            CL_GL_CONTEXT_KHR,   (cl_context_properties) wglGetCurrentContext(),
            CL_WGL_HDC_KHR,      (cl_context_properties) wglGetCurrentDC(),
            CL_CONTEXT_PLATFORM, (cl_context_properties) platformID,
            0
        };

        cl_device_id deviceID;
        size_t size;
        cl_int ret = my_clGetGLContextInfoKHR(props, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &deviceID, &size);

        if (ret != CL_SUCCESS || size == 0 || deviceID == NULL)
            continue;

        log::info("OpenCL device found for current OpenGL context: {}", getDeviceName(deviceID));

        if (fetchGPUCoreCountRaw(deviceID)) {
            gpuCoreCountFetched = true;
            return true;
        }
    }

    return false;
}

isize getGPUCoreCount() {
    int setting = Mod::get()->getSettingValue<int>("gpu_core_count");
    if (setting != 0)
        return setting;

    if (isAMDGPU())
        return -1;

    if (fetchFailed)
        return -1;

    if (!gpuCoreCountFetched)
        fetchFailed = !fetchGPUCoreCount();

    return gpuCoreCount;
}

$on_mod(Loaded) {
    getGPUCoreCount();
}