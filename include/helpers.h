
#include <stdlib.h>
#include <cuda.h>

#ifndef __helpers_H
#define __helpers_H

#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif

CUresult errr = CUDA_SUCCESS;
CUcontext ctx = NULL;

#define CU_ERR(e) \
  if ((errr = e) != CUDA_SUCCESS) { \
    const char *errstr; \
    cuGetErrorString(errr, &errstr); \
    fprintf(stderr, "CUDA driver err at `%s' %d: %s\n", #e, errr, errstr); \
    if (ctx) \
      cuCtxDestroy(ctx); \
    exit(-1); \
  } \

inline int _ConvertSMVer2CoresDRV(int major, int minor)
{
    // Defines for GPU Architecture types (using the SM version to determine the # of cores per SM
    typedef struct
    {
        int SM; // 0xMm (hexidecimal notation), M = SM Major version, and m = SM minor version
        int Cores;
    } sSMtoCores;

    sSMtoCores nGpuArchCoresPerSM[] =
    {
        { 0x10,  8 }, // Tesla Generation (SM 1.0) G80 class
        { 0x11,  8 }, // Tesla Generation (SM 1.1) G8x class
        { 0x12,  8 }, // Tesla Generation (SM 1.2) G9x class
        { 0x13,  8 }, // Tesla Generation (SM 1.3) GT200 class
        { 0x20, 32 }, // Fermi Generation (SM 2.0) GF100 class
        { 0x21, 48 }, // Fermi Generation (SM 2.1) GF10x class
        { 0x30, 192}, // Kepler Generation (SM 3.0) GK10x class
        { 0x32, 192}, // Kepler Generation (SM 3.2) GK10x class
        { 0x35, 192}, // Kepler Generation (SM 3.5) GK11x class
        { 0x37, 192}, // Kepler Generation (SM 3.7) GK21x class
        { 0x50, 128}, // Maxwell Generation (SM 5.0) GM10x class
        {   -1, -1 }
    };

    int index = 0;

    while (nGpuArchCoresPerSM[index].SM != -1)
    {
        if (nGpuArchCoresPerSM[index].SM == ((major << 4) + minor))
        {
            return nGpuArchCoresPerSM[index].Cores;
        }

        index++;
    }

    // If we don't find the values, we default use the previous one to run properly
    printf("MapSMtoCores for SM %d.%d is undefined.  Default to use %d Cores/SM\n", major, minor, nGpuArchCoresPerSM[index-1].Cores);
    return nGpuArchCoresPerSM[index-1].Cores;
}

inline int fetch_best_device()
{
    CUresult errr = CUDA_SUCCESS;
    CUdevice current_device = 0, max_perf_device = 0;
    int device_count        = 0, sm_per_multiproc = 0;
    int max_compute_perf    = 0, best_SM_arch     = 0;
    int major = 0, minor = 0   , multiProcessorCount, clockRate;
    int devices_prohibited = 0;

    cuInit(0);
    CU_ERR(cuDeviceGetCount(&device_count));

    if (device_count == 0)
    {
        fprintf(stderr, "gpuGetMaxGflopsDeviceIdDRV error: no devices supporting CUDA\n");
        exit(EXIT_FAILURE);
    }

    // Find the best major SM Architecture GPU device
    while (current_device < device_count)
    {
        CU_ERR(cuDeviceComputeCapability(&major, &minor, current_device));

        if (major > 0 && major < 9999)
        {
            best_SM_arch = MAX(best_SM_arch, major);
        }

        current_device++;
    }

    // Find the best CUDA capable GPU device
    current_device = 0;

    while (current_device < device_count)
    {
        CU_ERR(cuDeviceGetAttribute(&multiProcessorCount,
                                             CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT,
                                             current_device));
        CU_ERR(cuDeviceGetAttribute(&clockRate,
                                             CU_DEVICE_ATTRIBUTE_CLOCK_RATE,
                                             current_device));
        CU_ERR(cuDeviceComputeCapability(&major, &minor, current_device));

        int computeMode;
        CU_ERR(cuDeviceGetAttribute(&computeMode, CU_DEVICE_ATTRIBUTE_COMPUTE_MODE, current_device));

        if (computeMode != CU_COMPUTEMODE_PROHIBITED)
        {
            if (major == 9999 && minor == 9999)
            {
                sm_per_multiproc = 1;
            }
            else
            {
                sm_per_multiproc = _ConvertSMVer2CoresDRV(major, minor);
            }

            int compute_perf  = multiProcessorCount * sm_per_multiproc * clockRate;

            if (compute_perf  > max_compute_perf)
            {
                // If we find GPU with SM major > 2, search only these
                if (best_SM_arch > 2)
                {
                    // If our device==dest_SM_arch, choose this, or else pass
                    if (major == best_SM_arch)
                    {
                        max_compute_perf  = compute_perf;
                        max_perf_device   = current_device;
                    }
                }
                else
                {
                    max_compute_perf  = compute_perf;
                    max_perf_device   = current_device;
                }
            }
        }
        else
        {
            devices_prohibited++;
        }

        ++current_device;
    }

    if (devices_prohibited == device_count)
    {    
        fprintf(stderr, "gpuGetMaxGflopsDeviceIdDRV error: all devices have compute mode prohibited.\n");
        exit(EXIT_FAILURE);
    }    

    return max_perf_device;
}

#endif

