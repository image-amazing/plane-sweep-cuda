/**
 *  \file memory.h
 *  \brief Header file containing templated class for memory management on GPU / CPU
 */
#ifndef MEMORY_H
#define MEMORY_H

#include <helper_cuda.h>
#include <cuda_runtime_api.h>
#include <cuda.h>
#include "cuda_exception.h"

/** \addtogroup memory Memory Management
 *
 *  \brief Group for managing memory
* @{
*/

/**
 *  \brief Enum for indicating memory location
 */
typedef enum MemoryKind{
#if CUDA_VERSION_MAJOR >= 6
    Managed,
#endif // CUDA_VERSION_MAJOR >= 6
    Standard,
    Host,
    Device
}  MemoryKind;


/**
 *  \brief Class containing overloaded \a new and \a delete operators that work with
 * managed memory.
 *
 *  \details Operator overloads enable simple passing of classes/structs to
 * kernel by reference or by value without extra code. Accessing managed memory from CPU
 * while it is still being accessed by GPU will cause \a segfault. Call
 * \a cudaDeviceSynchronize() before accessing it. Only works with \a CUDA
 * libraries of major version 6 or greater.
 */
class Manage {
public:
#if CUDA_VERSION_MAJOR >= 6
    /** \brief Operator \a new overload to allocate managed memory instead of host. */
    void *operator new(size_t len) {
        void *ptr;
        CHECK_CUDA_ERRORS_AUTO(cudaMallocManaged((void **)&ptr, len, cudaMemAttachGlobal));
        return ptr;
    }

    void operator delete(void *ptr) {
        CHECK_CUDA_ERRORS_AUTO(cudaFree(ptr));
    }
#endif // CUDA_VERSION_MAJOR >= 6
};

/**
 *  \brief Templated class for managing memory allocation and deallocation on host / device
 *
 *  \tparam T      type of data to work with
 *  \tparam memT   data location
 *
 *  \details This is a base class for all other classes that need to manage memory on device and/or host.
 */
template<typename T, MemoryKind memT = Device>
class MemoryManagement
{
public:

    /**
     *  \brief Deallocate memory pointed to by \a ptr
     *
     *  \param ptr pointer to memory to deallocate
     *
     *  \details Pointer has to point to a memory location indicated by \p memT
     */
    __host__ inline
    static void CleanUp(T * ptr)
    {
        if (memT == Standard) delete[] ptr;
        else if (memT == Host) CHECK_CUDA_ERRORS_AUTO(cudaFreeHost(ptr));
        else CHECK_CUDA_ERRORS_AUTO(cudaFree(ptr));
        ptr = 0;
    }

    /**
     *  \brief Allocate 1D memory and CHECK_CUDA_ERRORS_AUTO(pointer \a ptr to it
     *
     *  \param ptr   pointer to allocated memory returned by reference
     *  \param len   number of elements
     */
    __host__ inline
    static void Malloc(T *&ptr, size_t len)
    {
        if (memT == Standard) ptr = new T[len];
        if (memT == Device) CHECK_CUDA_ERRORS_AUTO(cudaMalloc((void **)&ptr, len * sizeof(T)));
#if CUDA_VERSION_MAJOR >= 6
        if (memT == Managed) CHECK_CUDA_ERRORS_AUTO(cudaMallocManaged((void **)&ptr, len * sizeof(T), cudaMemAttachGlobal));
#endif
        if (memT == Host) CHECK_CUDA_ERRORS_AUTO(cudaMallocHost((void **)&ptr, len * sizeof(T)));
    }

    /**
     *  \brief Allocate 2D memory and CHECK_CUDA_ERRORS_AUTO(pointer \a ptr to it
     *
     *  \param ptr   pointer to allocated memory returned by reference
     *  \param w     width in number of elements
     *  \param h     height in number of elements
     *  \param pitch step size in bytes returned by reference
     */
    __host__ inline
    static void Malloc(T *&ptr, size_t w, size_t h, size_t &pitch)
    {
        if (memT == Device) CHECK_CUDA_ERRORS_AUTO(cudaMallocPitch((void **)&ptr, &pitch, w * sizeof(T), h));
        pitch = w * sizeof(T);
        if (memT == Standard) ptr = new T[w * h];
#if CUDA_VERSION_MAJOR >= 6
        if (memT == Managed) CHECK_CUDA_ERRORS_AUTO(cudaMallocManaged((void **)&ptr, pitch * h, cudaMemAttachGlobal));
#endif
        if (memT == Host) CHECK_CUDA_ERRORS_AUTO(cudaMallocHost((void **)&ptr, pitch * h));
    }

    /**
     *  \brief This is an overloaded function for 3D memory allocation.
     *
     *  \param ptr    pointer to allocated memory returned by reference
     *  \param w      width in number of elements
     *  \param h      height in number of elements
     *  \param d      depth in number of elements
     *  \param pitch  step size in bytes returned by reference
     *  \param spitch single slice size in bytes returned by reference
     *
     *  \details Allocate 3D memory and CHECK_CUDA_ERRORS_AUTO(pointer \a ptr to it.
     */
    __host__ inline
    static void Malloc(T *&ptr, size_t w, size_t h, size_t d, size_t &pitch, size_t &spitch)
    {
        if (memT == Device) CHECK_CUDA_ERRORS_AUTO(cudaMallocPitch((void **)&ptr, &pitch, w * sizeof(T), h * d));
        pitch = w * sizeof(T);
        if (memT == Standard) ptr = new T[w*h*d];
#if CUDA_VERSION_MAJOR >= 6
        if (memT == MemoryKind::Managed) CHECK_CUDA_ERRORS_AUTO(cudaMallocManaged((void **)&ptr, pitch * h * d, cudaMemAttachGlobal));
#endif
        if (memT == Host) CHECK_CUDA_ERRORS_AUTO(cudaMallocHost((void **)&ptr, pitch * h * d));
        spitch = h * pitch;
    }

    /**
     *  \brief Device to device 1D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param pSrc     pointer to source memory
     *  \param len      number of elements to copy
     */
    __host__ inline
    static void Device2DeviceCopy(T * pDst, const T * pSrc, size_t len)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy(pDst, pSrc, len * sizeof(T), cudaMemcpyDeviceToDevice));
    }

    /**
     *  \brief Device to host 1D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param pSrc     pointer to source memory
     *  \param len      number of elements to copy
     */
    __host__ inline
    static void Device2HostCopy(T *pDst, const T *pSrc, size_t len)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy(pDst, pSrc, len * sizeof(T), cudaMemcpyDeviceToHost));
    }

    /**
     *  \brief Host to device 1D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param pSrc     pointer to source memory
     *  \param len      number of elements to copy
     */
    __host__ inline
    static void Host2DeviceCopy(T *pDst, const T *pSrc, size_t len)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy(pDst, pSrc, len * sizeof(T), cudaMemcpyHostToDevice));
    }

    /**
     *  \brief Host to host 1D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param pSrc     pointer to source memory
     *  \param len      number of elements to copy 
     */
    __host__ inline
    static void Host2HostCopy(T *pDst, const T *pSrc, size_t len)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy(pDst, pSrc, len * sizeof(T), cudaMemcpyHostToHost));
    }

    /**
     *  \brief Device to device 2D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param DstPitch step size in bytes of destination memory
     *  \param pSrc     pointer to source memory
     *  \param SrcPitch step size in bytes of source memory
     *  \param width    width in number of elements
     *  \param height   height in number of elements
     */
    __host__ inline
    static void Device2DeviceCopy(T *pDst, size_t DstPitch, const T *pSrc, size_t SrcPitch, size_t width, size_t height)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy2D(pDst, DstPitch, pSrc, SrcPitch, width * sizeof(T), height, cudaMemcpyDeviceToDevice));
    }

    /**
     *  \brief Device to host 2D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param DstPitch step size in bytes of destination memory
     *  \param pSrc     pointer to source memory
     *  \param SrcPitch step size in bytes of source memory
     *  \param width    width in number of elements
     *  \param height   height in number of elements
     */
    __host__ inline
    static void Device2HostCopy(T *pDst, size_t DstPitch, const T *pSrc, size_t SrcPitch, size_t width, size_t height)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy2D(pDst, DstPitch, pSrc, SrcPitch, width * sizeof(T), height, cudaMemcpyDeviceToHost));
    }

    /**
     *  \brief Host to device 2D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param DstPitch step size in bytes of destination memory
     *  \param pSrc     pointer to source memory
     *  \param SrcPitch step size in bytes of source memory
     *  \param width    width in number of elements
     *  \param height   height in number of elements
     */
    __host__ inline
    static void Host2DeviceCopy(T *pDst, size_t DstPitch, const T *pSrc, size_t SrcPitch, size_t width, size_t height)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy2D(pDst, DstPitch, pSrc, SrcPitch, width * sizeof(T), height, cudaMemcpyHostToDevice));
    }

    /**
     *  \brief Host to host 2D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param DstPitch step size in bytes of destination memory
     *  \param pSrc     pointer to source memory
     *  \param SrcPitch step size in bytes of source memory
     *  \param width    width in number of elements
     *  \param height   height in number of elements
     */
    __host__ inline
    static void Host2HostCopy(T *pDst, size_t DstPitch, const T *pSrc, size_t SrcPitch, size_t width, size_t height)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy2D(pDst, DstPitch, pSrc, SrcPitch, width * sizeof(T), height, cudaMemcpyHostToHost));
    }

    /**
     *  \brief Device to device 3D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param DstPitch step size in bytes of destination memory
     *  \param pSrc     pointer to source memory
     *  \param SrcPitch step size in bytes of source memory
     *  \param width    width in number of elements
     *  \param height   height in number of elements
     *  \param depth    depth in number of elements
     */
    __host__ inline
    static void Device2DeviceCopy(T *pDst, size_t DstPitch, const T *pSrc, size_t SrcPitch, size_t width, size_t height, size_t depth)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy2D(pDst, DstPitch, pSrc, SrcPitch, width * sizeof(T), height * depth, cudaMemcpyDeviceToDevice));
    }

    /**
     *  \brief Device to host 3D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param DstPitch step size in bytes of destination memory
     *  \param pSrc     pointer to source memory
     *  \param SrcPitch step size in bytes of source memory
     *  \param width    width in number of elements
     *  \param height   height in number of elements
     *  \param depth    depth in number of elements
     */
    __host__ inline
    static void Device2HostCopy(T *pDst, size_t DstPitch, const T *pSrc, size_t SrcPitch, size_t width, size_t height, size_t depth)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy2D(pDst, DstPitch, pSrc, SrcPitch, width * sizeof(T), height * depth, cudaMemcpyDeviceToHost));
    }

    /**
     *  \brief Host to device 3D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param DstPitch step size in bytes of destination memory
     *  \param pSrc     pointer to source memory
     *  \param SrcPitch step size in bytes of source memory
     *  \param width    width in number of elements
     *  \param height   height in number of elements
     *  \param depth    depth in number of elements
     */
    __host__ inline
    static void Host2DeviceCopy(T *pDst, size_t DstPitch, const T *pSrc, size_t SrcPitch, size_t width, size_t height, size_t depth)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy2D(pDst, DstPitch, pSrc, SrcPitch, width * sizeof(T), height * depth, cudaMemcpyHostToDevice));
    }

    /**
     *  \brief Host to host 3D memory copy
     *
     *  \param pDst     pointer to destination memory
     *  \param DstPitch step size in bytes of destination memory
     *  \param pSrc     pointer to source memory
     *  \param SrcPitch step size in bytes of source memory
     *  \param width    width in number of elements
     *  \param height   height in number of elements
     *  \param depth    depth in number of elements
     */
    __host__ inline
    static void Host2HostCopy(T *pDst, size_t DstPitch, const T *pSrc, size_t SrcPitch, size_t width, size_t height, size_t depth)
    {
        CHECK_CUDA_ERRORS_AUTO(cudaMemcpy2D(pDst, DstPitch, pSrc, SrcPitch, width * sizeof(T), height * depth, cudaMemcpyHostToHost));
    }
};

/** @} */ // group memory

#endif // MEMORY_H
