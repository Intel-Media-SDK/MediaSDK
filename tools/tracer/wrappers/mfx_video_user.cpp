/* ****************************************************************************** *\

Copyright (C) 2012-2020 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: mfx_video_user.cpp

\* ****************************************************************************** */

#include <exception>
#include <iostream>

#include "../loggers/timer.h"
#include "../tracer/functions_table.h"
#include "mfx_structures.h"

#if TRACE_CALLBACKS
mfxStatus mfxCoreInterface_GetCoreParam(mfxHDL _pthis, mfxCoreParam *par)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_GetCoreParam_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::GetCoreParam(mfxHDL pthis=" + ToString(pthis)
            + ", mfxCoreParam* par=" + ToString(par) + ") +");

        fmfxCoreInterface_GetCoreParam proc = (fmfxCoreInterface_GetCoreParam)loader->callbacks[emfxCoreInterface_GetCoreParam_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*proc) (pthis, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::GetCoreParam called");
        if (par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("callback: mfxCoreInterface::GetCoreParam(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_GetHandle(mfxHDL _pthis, mfxHandleType type, mfxHDL *handle)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_GetHandle_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::GetHandle(mfxHDL pthis=" + ToString(pthis)
            + ", mfxHandleType type=" + ToString(type)
            + ", mfxHDL *handle=" + ToString(handle) + ") +");

        fmfxCoreInterface_GetHandle proc = (fmfxCoreInterface_GetHandle)loader->callbacks[emfxCoreInterface_GetHandle_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (handle) Log::WriteLog(context.dump("handle", *handle));

        Timer t;
        mfxStatus status = (*proc) (pthis, type, handle);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::GetHandle called");
        if (handle) Log::WriteLog(context.dump("handle", *handle));
        Log::WriteLog("callback: mfxCoreInterface::GetHandle(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_IncreaseReference(mfxHDL _pthis, mfxFrameData *fd)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_IncreaseReference_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::IncreaseReference(mfxHDL pthis=" + ToString(pthis)
            + ", mfxFrameData *fd=" + ToString(fd) + ") +");

        fmfxCoreInterface_IncreaseReference proc = (fmfxCoreInterface_IncreaseReference)loader->callbacks[emfxCoreInterface_IncreaseReference_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (fd) Log::WriteLog(context.dump("fd", *fd));

        Timer t;
        mfxStatus status = (*proc) (pthis, fd);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::IncreaseReference called");
        if (fd) Log::WriteLog(context.dump("fd", *fd));
        Log::WriteLog("callback: mfxCoreInterface::IncreaseReference(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_DecreaseReference(mfxHDL _pthis, mfxFrameData *fd)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_DecreaseReference_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::DecreaseReference(mfxHDL pthis=" + ToString(pthis)
            + ", mfxFrameData *fd=" + ToString(fd) + ") +");

        fmfxCoreInterface_DecreaseReference proc = (fmfxCoreInterface_DecreaseReference)loader->callbacks[emfxCoreInterface_DecreaseReference_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (fd) Log::WriteLog(context.dump("fd", *fd));

        Timer t;
        mfxStatus status = (*proc) (pthis, fd);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::DecreaseReference called");
        if (fd) Log::WriteLog(context.dump("fd", *fd));
        Log::WriteLog("callback: mfxCoreInterface::DecreaseReference(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_CopyFrame(mfxHDL _pthis, mfxFrameSurface1 *dst, mfxFrameSurface1 *src)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_CopyFrame_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::CopyFrame(mfxHDL pthis=" + ToString(pthis)
            + ", mfxFrameSurface1 *dst=" + ToString(dst)
            + ", mfxFrameSurface1 *src=" + ToString(src) + ") +");

        fmfxCoreInterface_CopyFrame proc = (fmfxCoreInterface_CopyFrame)loader->callbacks[emfxCoreInterface_CopyFrame_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (dst) Log::WriteLog(context.dump("dst", *dst));
        if (src) Log::WriteLog(context.dump("src", *src));

        Timer t;
        mfxStatus status = (*proc) (pthis, dst, src);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::CopyFrame called");
        Log::WriteLog("callback: mfxCoreInterface::CopyFrame(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_CopyBuffer(mfxHDL _pthis, mfxU8 *dst, mfxU32 size, mfxFrameSurface1 *src)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_CopyBuffer_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::CopyBuffer(mfxHDL pthis=" + ToString(pthis)
            + ", mfxU8 *dst=" + ToString(dst)
            + ", mfxU32 size=" + ToString(size)
            + ", mfxFrameSurface1 *src=" + ToString(src) + ") +");

        fmfxCoreInterface_CopyBuffer proc = (fmfxCoreInterface_CopyBuffer)loader->callbacks[emfxCoreInterface_CopyBuffer_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (src) Log::WriteLog(context.dump("src", *src));

        Timer t;
        mfxStatus status = (*proc) (pthis, dst, size, src);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::CopyBuffer called");
        Log::WriteLog("callback: mfxCoreInterface::CopyBuffer(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_MapOpaqueSurface(mfxHDL _pthis, mfxU32 num, mfxU32 type, mfxFrameSurface1 **op_surf)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_MapOpaqueSurface_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::MapOpaqueSurface(mfxHDL pthis=" + ToString(pthis)
            + ", mfxU32 num=" + ToString(num)
            + ", mfxU32 type=" + ToString(type)
            + ", mfxFrameSurface1 **op_surf=" + ToString(op_surf) + ") +");

        fmfxCoreInterface_MapOpaqueSurface proc = (fmfxCoreInterface_MapOpaqueSurface)loader->callbacks[emfxCoreInterface_MapOpaqueSurface_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (num && op_surf)
            for (mfxU32 i = 0; i < num; i++)
                if (op_surf[i])
                    Log::WriteLog(context.dump("op_surf[" + ToString(i) + "]=", *op_surf[i]));

        Timer t;
        mfxStatus status = (*proc) (pthis, num, type, op_surf);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::MapOpaqueSurface called");
        if (num && op_surf)
            for (mfxU32 i = 0; i < num; i++)
                if (op_surf[i])
                    Log::WriteLog(context.dump("op_surf[" + ToString(i) + "]=", *op_surf[i]));
        Log::WriteLog("callback: mfxCoreInterface::MapOpaqueSurface(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_UnmapOpaqueSurface(mfxHDL _pthis, mfxU32  num, mfxU32  type, mfxFrameSurface1 **op_surf)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_UnmapOpaqueSurface_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::UnmapOpaqueSurface(mfxHDL pthis=" + ToString(pthis)
            + ", mfxU32 num=" + ToString(num)
            + ", mfxU32 type=" + ToString(type)
            + ", mfxFrameSurface1 **op_surf=" + ToString(op_surf) + ") +");

        fmfxCoreInterface_UnmapOpaqueSurface proc = (fmfxCoreInterface_UnmapOpaqueSurface)loader->callbacks[emfxCoreInterface_UnmapOpaqueSurface_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (num && op_surf)
            for (mfxU32 i = 0; i < num; i++)
                if (op_surf[i])
                    Log::WriteLog(context.dump("op_surf[" + ToString(i) + "]=", *op_surf[i]));

        Timer t;
        mfxStatus status = (*proc) (pthis, num, type, op_surf);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::UnmapOpaqueSurface called");
        if (num && op_surf)
            for (mfxU32 i = 0; i < num; i++)
                if (op_surf[i])
                    Log::WriteLog(context.dump("op_surf[" + ToString(i) + "]=", *op_surf[i]));
        Log::WriteLog("callback: mfxCoreInterface::UnmapOpaqueSurface(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_GetRealSurface(mfxHDL _pthis, mfxFrameSurface1 *op_surf, mfxFrameSurface1 **surf)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_GetRealSurface_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::GetRealSurface(mfxHDL pthis=" + ToString(pthis)
            + ", mfxFrameSurface1 *op_surf=" + ToString(op_surf)
            + ", mfxFrameSurface1 **surf=" + ToString(surf) + ") +");

        fmfxCoreInterface_GetRealSurface proc = (fmfxCoreInterface_GetRealSurface)loader->callbacks[emfxCoreInterface_GetRealSurface_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (op_surf) Log::WriteLog(context.dump("op_surf", *op_surf));

        Timer t;
        mfxStatus status = (*proc) (pthis, op_surf, surf);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::GetRealSurface called");
        if (surf && *surf) Log::WriteLog(context.dump("surf", **surf));
        Log::WriteLog("callback: mfxCoreInterface::GetRealSurface(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_GetOpaqueSurface(mfxHDL _pthis, mfxFrameSurface1 *surf, mfxFrameSurface1 **op_surf)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_GetOpaqueSurface_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::GetOpaqueSurface(mfxHDL pthis=" + ToString(pthis)
            + ", mfxFrameSurface1 **surf=" + ToString(surf)
            + ", mfxFrameSurface1 *op_surf=" + ToString(op_surf) + ") +");

        fmfxCoreInterface_GetOpaqueSurface proc = (fmfxCoreInterface_GetOpaqueSurface)loader->callbacks[emfxCoreInterface_GetOpaqueSurface_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (surf) Log::WriteLog(context.dump("surf", *surf));

        Timer t;
        mfxStatus status = (*proc) (pthis, surf, op_surf);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::GetOpaqueSurface called");
        if (op_surf && *op_surf) Log::WriteLog(context.dump("op_surf", **op_surf));
        Log::WriteLog("callback: mfxCoreInterface::GetOpaqueSurface(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_CreateAccelerationDevice(mfxHDL _pthis, mfxHandleType type, mfxHDL *handle)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_CreateAccelerationDevice_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::CreateAccelerationDevice(mfxHDL pthis=" + ToString(pthis)
            + ", mfxHandleType type=" + ToString(type)
            + ", mfxHDL *handle=" + ToString(handle) + ") +");

        fmfxCoreInterface_CreateAccelerationDevice proc = (fmfxCoreInterface_CreateAccelerationDevice)loader->callbacks[emfxCoreInterface_CreateAccelerationDevice_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (handle) Log::WriteLog(context.dump("handle", *handle));

        Timer t;
        mfxStatus status = (*proc) (pthis, type, handle);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::CreateAccelerationDevice called");
        if (handle) Log::WriteLog(context.dump("handle", *handle));
        Log::WriteLog("callback: mfxCoreInterface::CreateAccelerationDevice(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_GetFrameHandle(mfxHDL _pthis, mfxFrameData *fd, mfxHDL *handle)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_GetFrameHandle_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::GetFrameHandle(mfxHDL pthis=" + ToString(pthis)
            + ", mfxFrameData *fd=" + ToString(fd)
            + ", mfxHDL *handle=" + ToString(handle) + ") +");

        fmfxCoreInterface_GetFrameHandle proc = (fmfxCoreInterface_GetFrameHandle)loader->callbacks[emfxCoreInterface_GetFrameHandle_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (fd) Log::WriteLog(context.dump("fd", *fd));
        if (handle) Log::WriteLog(context.dump("handle", *handle));

        Timer t;
        mfxStatus status = (*proc) (pthis, fd, handle);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::GetFrameHandle called");
        if (handle) Log::WriteLog(context.dump("handle", *handle));
        Log::WriteLog("callback: mfxCoreInterface::GetFrameHandle(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxCoreInterface_QueryPlatform(mfxHDL _pthis, mfxPlatform *platform)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader *loader = (mfxLoader*)_pthis;
        if (!loader) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = loader->callbacks[emfxCoreInterface_QueryPlatform_tracer][1];

        Log::WriteLog("callback: mfxCoreInterface::QueryPlatform(mfxHDL pthis=" + ToString(pthis)
            + ", mfxPlatform *platform=" + ToString(platform) + ") +");

        fmfxCoreInterface_QueryPlatform proc = (fmfxCoreInterface_QueryPlatform)loader->callbacks[emfxCoreInterface_QueryPlatform_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (platform) Log::WriteLog(context.dump("platform", *platform));

        Timer t;
        mfxStatus status = (*proc) (pthis, platform);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxCoreInterface::QueryPlatform called");
        if (platform) Log::WriteLog(context.dump("platform", *platform));
        Log::WriteLog("callback: mfxCoreInterface::QueryPlatform(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxPlugin_PluginInit(mfxHDL _pthis, mfxCoreInterface *core)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxPlugin_PluginInit_tracer][1];

        Log::WriteLog("callback: mfxPlugin::PluginInit(mfxHDL pthis=" + ToString(pthis)
            + ", mfxCoreInterface *core=" + ToString(core) + ") +");

        fmfxPlugin_PluginInit proc = (fmfxPlugin_PluginInit)pCtx->callbacks[emfxPlugin_PluginInit_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (core) Log::WriteLog(context.dump("core", *core));
        
        Timer t;
        mfxStatus status = (*proc) (pthis, core);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxPlugin::PluginInit called");
        Log::WriteLog("callback: mfxPlugin::PluginInit(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

        if (core && status == MFX_ERR_NONE)
        {
            INIT_CALLBACK_BACKUP(pCtx->pLoaderBase->callbacks);

            SET_CALLBACK(mfxFrameAllocator, core->FrameAllocator., Alloc,   core->FrameAllocator.pthis);
            SET_CALLBACK(mfxFrameAllocator, core->FrameAllocator., Lock,    core->FrameAllocator.pthis);
            SET_CALLBACK(mfxFrameAllocator, core->FrameAllocator., Unlock,  core->FrameAllocator.pthis);
            SET_CALLBACK(mfxFrameAllocator, core->FrameAllocator., GetHDL,  core->FrameAllocator.pthis);
            SET_CALLBACK(mfxFrameAllocator, core->FrameAllocator., Free,    core->FrameAllocator.pthis);
            if (core->FrameAllocator.pthis)
                core->FrameAllocator.pthis = pCtx->pLoaderBase;

            SET_CALLBACK(mfxCoreInterface, core->, GetCoreParam,             core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, GetHandle,                core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, IncreaseReference,        core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, DecreaseReference,        core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, CopyFrame,                core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, CopyBuffer,               core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, MapOpaqueSurface,         core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, UnmapOpaqueSurface,       core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, GetRealSurface,           core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, GetOpaqueSurface,         core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, CreateAccelerationDevice, core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, GetFrameHandle,           core->pthis);
            SET_CALLBACK(mfxCoreInterface, core->, QueryPlatform,            core->pthis);
            if (core->pthis)
                core->pthis = pCtx->pLoaderBase;
        }

        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxPlugin_PluginClose(mfxHDL _pthis)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxPlugin_PluginClose_tracer][1];

        Log::WriteLog("callback: mfxPlugin::PluginClose(mfxHDL pthis=" + ToString(pthis) + ") +");

        fmfxPlugin_PluginClose proc = (fmfxPlugin_PluginClose)pCtx->callbacks[emfxPlugin_PluginClose_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));

        Timer t;
        mfxStatus status = (*proc) (pthis);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxPlugin::PluginClose called");
        Log::WriteLog("callback: mfxPlugin::PluginClose(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxPlugin_GetPluginParam(mfxHDL _pthis, mfxPluginParam *par)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxPlugin_GetPluginParam_tracer][1];

        Log::WriteLog("callback: mfxPlugin::GetPluginParam(mfxHDL pthis=" + ToString(pthis)
            + ", mfxPluginParam *par=" + ToString(par) + ") +");

        fmfxPlugin_GetPluginParam proc = (fmfxPlugin_GetPluginParam)pCtx->callbacks[emfxPlugin_GetPluginParam_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*proc) (pthis, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxPlugin::GetPluginParam called");
        if (par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("callback: mfxPlugin::GetPluginParam(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxPlugin_Submit(mfxHDL _pthis, const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxPlugin_Submit_tracer][1];

        Log::WriteLog("callback: mfxPlugin::Submit(mfxHDL pthis=" + ToString(pthis)
            + ", const mfxHDL *in=" + ToString(in)
            + ", mfxU32 in_num=" + ToString(in_num)
            + ", const mfxHDL *out=" + ToString(out)
            + ", mfxU32 out_num=" + ToString(out_num)
            + ", mfxThreadTask *task=" + ToString(task) + ") +");

        fmfxPlugin_Submit proc = (fmfxPlugin_Submit)pCtx->callbacks[emfxPlugin_Submit_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (task) Log::WriteLog(context.dump("task", *task));

        Timer t;
        mfxStatus status = (*proc) (pthis, in, in_num, out, out_num, task);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxPlugin::Submit called");
        if (task) Log::WriteLog(context.dump("task", *task));
        Log::WriteLog("callback: mfxPlugin::Submit(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxPlugin_Execute(mfxHDL _pthis, mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxPlugin_Execute_tracer][1];

        Log::WriteLog("callback: mfxPlugin::Execute(mfxHDL pthis=" + ToString(pthis)
            + ", mfxThreadTask task=" + ToString(task)
            + ", mfxU32 uid_p=" + ToString(uid_p)
            + ", mfxU32 uid_a=" + ToString(uid_a) + ") +");

        fmfxPlugin_Execute proc = (fmfxPlugin_Execute)pCtx->callbacks[emfxPlugin_Execute_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));

        Timer t;
        mfxStatus status = (*proc) (pthis, task, uid_p, uid_a);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxPlugin::Execute called");
        Log::WriteLog("callback: mfxPlugin::Execute(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxPlugin_FreeResources(mfxHDL _pthis, mfxThreadTask task, mfxStatus sts)
{
    try
    {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxPlugin_FreeResources_tracer][1];

        Log::WriteLog("callback: mfxPlugin::FreeResources(mfxHDL pthis=" + ToString(pthis)
            + ", mfxThreadTask task=" + ToString(task)
            + ", mfxStatus sts=" + ToString(sts) + ") +");

        fmfxPlugin_FreeResources proc = (fmfxPlugin_FreeResources)pCtx->callbacks[emfxPlugin_FreeResources_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));

        Timer t;
        mfxStatus status = (*proc) (pthis, task, sts);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxPlugin::FreeResources called");
        Log::WriteLog("callback: mfxPlugin::FreeResources(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
inline bool IsVPPPlugin(mfxLoader::Plugin* pCtx)
{
    return ((mfxU8*)pCtx - (mfxU8*)pCtx->pLoaderBase)
        == ((mfxU8*)pCtx->pLoaderBase - (mfxU8*)&pCtx->pLoaderBase->plugin[MFX_PLUGINTYPE_VIDEO_VPP]);
}
mfxStatus mfxVideoCodecPlugin_Query(mfxHDL _pthis, mfxVideoParam *in, mfxVideoParam *out)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_Query_tracer][1];
        context.context = IsVPPPlugin(pCtx) ? DUMPCONTEXT_VPP : DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::Query(mfxHDL pthis=" + ToString(pthis)
            + ", mfxVideoParam *in=" + ToString(in)
            + ", mfxVideoParam *out=" + ToString(out) + ") +");

        fmfxVideoCodecPlugin_Query proc = (fmfxVideoCodecPlugin_Query)pCtx->callbacks[emfxVideoCodecPlugin_Query_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (in) Log::WriteLog(context.dump("in", *in));
        if (out) Log::WriteLog(context.dump("out", *out));

        Timer t;
        mfxStatus status = (*proc) (pthis, in, out);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::Query called");
        if (out) Log::WriteLog(context.dump("out", *out));
        Log::WriteLog("callback: mfxVideoCodecPlugin::Query(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_QueryIOSurf(mfxHDL _pthis, mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_QueryIOSurf_tracer][1];
        context.context = IsVPPPlugin(pCtx) ? DUMPCONTEXT_VPP : DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::QueryIOSurf(mfxHDL pthis=" + ToString(pthis)
            + ", mfxVideoParam *par=" + ToString(par)
            + ", mfxFrameAllocRequest *in=" + ToString(in)
            + ", mfxFrameAllocRequest *out=" + ToString(out) + ") +");

        fmfxVideoCodecPlugin_QueryIOSurf proc = (fmfxVideoCodecPlugin_QueryIOSurf)pCtx->callbacks[emfxVideoCodecPlugin_QueryIOSurf_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (par) Log::WriteLog(context.dump("par", *par));
        if (in) Log::WriteLog(context.dump("in", *in));
        if (out) Log::WriteLog(context.dump("out", *out));

        Timer t;
        mfxStatus status = (*proc) (pthis, par, in, out);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::QueryIOSurf called");
        if (in) Log::WriteLog(context.dump("in", *in));
        if (out) Log::WriteLog(context.dump("out", *out));
        Log::WriteLog("callback: mfxVideoCodecPlugin::QueryIOSurf(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_Init(mfxHDL _pthis, mfxVideoParam *par)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_Init_tracer][1];
        context.context = IsVPPPlugin(pCtx) ? DUMPCONTEXT_VPP : DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::Init(mfxHDL pthis=" + ToString(pthis)
            + ", mfxVideoParam *par" + ToString(par) + ") +");

        fmfxVideoCodecPlugin_Init proc = (fmfxVideoCodecPlugin_Init)pCtx->callbacks[emfxVideoCodecPlugin_Init_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*proc) (pthis, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::Init called");
        Log::WriteLog("callback: mfxVideoCodecPlugin::Init(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_Reset(mfxHDL _pthis, mfxVideoParam *par)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_Reset_tracer][1];
        context.context = IsVPPPlugin(pCtx) ? DUMPCONTEXT_VPP : DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::Reset(mfxHDL pthis=" + ToString(pthis)
            + ", mfxVideoParam *par" + ToString(par) + ") +");

        fmfxVideoCodecPlugin_Reset proc = (fmfxVideoCodecPlugin_Reset)pCtx->callbacks[emfxVideoCodecPlugin_Reset_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*proc) (pthis, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::Reset called");
        Log::WriteLog("callback: mfxVideoCodecPlugin::Reset(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_Close(mfxHDL _pthis)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_Close_tracer][1];
        context.context = IsVPPPlugin(pCtx) ? DUMPCONTEXT_VPP : DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::Close(mfxHDL pthis=" + ToString(pthis) + ") +");

        fmfxVideoCodecPlugin_Close proc = (fmfxVideoCodecPlugin_Close)pCtx->callbacks[emfxVideoCodecPlugin_Close_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));

        Timer t;
        mfxStatus status = (*proc) (pthis);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::Close called");
        Log::WriteLog("callback: mfxVideoCodecPlugin::Close(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_GetVideoParam(mfxHDL _pthis, mfxVideoParam *par)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_GetVideoParam_tracer][1];
        context.context = IsVPPPlugin(pCtx) ? DUMPCONTEXT_VPP : DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::GetVideoParam(mfxHDL pthis=" + ToString(pthis)
            + ", mfxVideoParam *par" + ToString(par) + ") +");

        fmfxVideoCodecPlugin_GetVideoParam proc = (fmfxVideoCodecPlugin_GetVideoParam)pCtx->callbacks[emfxVideoCodecPlugin_GetVideoParam_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*proc) (pthis, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::GetVideoParam called");
        if (par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("callback: mfxVideoCodecPlugin::GetVideoParam(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_EncodeFrameSubmit(mfxHDL _pthis, mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxThreadTask *task)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_EncodeFrameSubmit_tracer][1];
        context.context = DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::EncodeFrameSubmit(mfxHDL pthis=" + ToString(pthis)
            + ", mfxEncodeCtrl *ctrl=" + ToString(ctrl)
            + ", mfxFrameSurface1 *surface=" + ToString(surface)
            + ", mfxBitstream *bs=" + ToString(bs)
            + ", mfxThreadTask *task=" + ToString(task) + ") +");

        fmfxVideoCodecPlugin_EncodeFrameSubmit proc = (fmfxVideoCodecPlugin_EncodeFrameSubmit)pCtx->callbacks[emfxVideoCodecPlugin_EncodeFrameSubmit_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (ctrl) Log::WriteLog(context.dump("ctrl", *ctrl));
        if (surface) Log::WriteLog(context.dump("surface", *surface));
        if (bs) Log::WriteLog(context.dump("bs", *bs));
        if (task) Log::WriteLog(context.dump("bs", *task));

        Timer t;
        mfxStatus status = (*proc) (pthis, ctrl, surface, bs, task);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::EncodeFrameSubmit called");
        if (task) Log::WriteLog(context.dump("task", *task));
        Log::WriteLog("callback: mfxVideoCodecPlugin::EncodeFrameSubmit(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_DecodeHeader(mfxHDL _pthis, mfxBitstream *bs, mfxVideoParam *par)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_DecodeHeader_tracer][1];
        context.context = DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::DecodeHeader(mfxHDL pthis=" + ToString(pthis)
            + ", mfxBitstream *bs=" + ToString(bs)
            + ", mfxVideoParam *par=" + ToString(par) + ") +");

        fmfxVideoCodecPlugin_DecodeHeader proc = (fmfxVideoCodecPlugin_DecodeHeader)pCtx->callbacks[emfxVideoCodecPlugin_DecodeHeader_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (bs) Log::WriteLog(context.dump("bs", *bs));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*proc) (pthis, bs, par);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::DecodeHeader called");
        if (par) Log::WriteLog(context.dump("par", *par));
        Log::WriteLog("callback: mfxVideoCodecPlugin::DecodeHeader(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_GetPayload(mfxHDL _pthis, mfxU64 *ts, mfxPayload *payload)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_GetPayload_tracer][1];
        context.context = DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::GetPayload(mfxHDL pthis=" + ToString(pthis)
            + ", mfxU64 *ts=" + ToString(ts)
            + ", mfxPayload *payload=" + ToString(payload) + ") +");

        fmfxVideoCodecPlugin_GetPayload proc = (fmfxVideoCodecPlugin_GetPayload)pCtx->callbacks[emfxVideoCodecPlugin_GetPayload_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (ts) Log::WriteLog("ts" + ToString(*ts));
        if (payload) Log::WriteLog(context.dump("payload", *payload));

        Timer t;
        mfxStatus status = (*proc) (pthis, ts, payload);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::GetPayload called");
        if (ts) Log::WriteLog("ts" + ToString(*ts));
        if (payload) Log::WriteLog(context.dump("par", *payload));
        Log::WriteLog("callback: mfxVideoCodecPlugin::GetPayload(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_DecodeFrameSubmit(mfxHDL _pthis, mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxThreadTask *task)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_DecodeFrameSubmit_tracer][1];
        context.context = DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::DecodeFrameSubmit(mfxHDL pthis=" + ToString(pthis)
            + ", mfxBitstream *bs=" + ToString(bs)
            + ", mfxFrameSurface1 *surface_work=" + ToString(surface_work)
            + ", mfxFrameSurface1 **surface_out=" + ToString(surface_out)
            + ", mfxThreadTask *task=" + ToString(task) + ") +");

        fmfxVideoCodecPlugin_DecodeFrameSubmit proc = (fmfxVideoCodecPlugin_DecodeFrameSubmit)pCtx->callbacks[emfxVideoCodecPlugin_DecodeFrameSubmit_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (bs) Log::WriteLog(context.dump("bs", *bs));
        if (surface_work) Log::WriteLog(context.dump("surface_work", *surface_work));
        if (surface_out && *surface_out) Log::WriteLog(context.dump("surface_out", **surface_out));

        Timer t;
        mfxStatus status = (*proc) (pthis, bs, surface_work, surface_out, task);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::DecodeFrameSubmit called");
        if (surface_out && *surface_out) Log::WriteLog(context.dump("surface_out", **surface_out));
        if (task) Log::WriteLog(context.dump("task", *task));
        Log::WriteLog("callback: mfxVideoCodecPlugin::DecodeFrameSubmit(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_VPPFrameSubmit(mfxHDL _pthis, mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxThreadTask *task)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_VPPFrameSubmit_tracer][1];
        context.context = DUMPCONTEXT_VPP;

        Log::WriteLog("callback: mfxVideoCodecPlugin::VPPFrameSubmit(mfxHDL pthis=" + ToString(pthis)
            + ", mfxFrameSurface1 *in=" + ToString(in)
            + ", mfxFrameSurface1 *out=" + ToString(out)
            + ", mfxExtVppAuxData *aux=" + ToString(aux)
            + ", mfxThreadTask *task=" + ToString(task) + ") +");

        fmfxVideoCodecPlugin_VPPFrameSubmit proc = (fmfxVideoCodecPlugin_VPPFrameSubmit)pCtx->callbacks[emfxVideoCodecPlugin_VPPFrameSubmit_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (in) Log::WriteLog(context.dump("in", *in));
        if (out) Log::WriteLog(context.dump("out", *out));

        Timer t;
        mfxStatus status = (*proc) (pthis, in, out, aux, task);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::VPPFrameSubmit called");
        if (out) Log::WriteLog(context.dump("out", *out));
        if (task) Log::WriteLog(context.dump("task", *task));
        Log::WriteLog("callback: mfxVideoCodecPlugin::VPPFrameSubmit(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_VPPFrameSubmitEx(mfxHDL _pthis, mfxFrameSurface1 *in, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxThreadTask *task)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_VPPFrameSubmitEx_tracer][1];
        context.context = DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::VPPFrameSubmitEx(mfxHDL pthis=" + ToString(pthis)
            + ", mfxFrameSurface1 *in=" + ToString(in)
            + ", mfxFrameSurface1 *surface_work=" + ToString(surface_work)
            + ", mfxFrameSurface1 **surface_out=" + ToString(surface_out)
            + ", mfxThreadTask *task=" + ToString(task) + ") +");

        fmfxVideoCodecPlugin_VPPFrameSubmitEx proc = (fmfxVideoCodecPlugin_VPPFrameSubmitEx)pCtx->callbacks[emfxVideoCodecPlugin_VPPFrameSubmitEx_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));
        if (in) Log::WriteLog(context.dump("in", *in));
        if (surface_work) Log::WriteLog(context.dump("surface_work", *surface_work));
        if (surface_out && *surface_out) Log::WriteLog(context.dump("surface_out", **surface_out));

        Timer t;
        mfxStatus status = (*proc) (pthis, in, surface_work, surface_out, task);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::VPPFrameSubmitEx called");
        if (surface_out && *surface_out) Log::WriteLog(context.dump("surface_out", **surface_out));
        if (task) Log::WriteLog(context.dump("task", *task));
        Log::WriteLog("callback: mfxVideoCodecPlugin::VPPFrameSubmitEx(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
mfxStatus mfxVideoCodecPlugin_ENCFrameSubmit(mfxHDL _pthis, mfxENCInput *in, mfxENCOutput *out, mfxThreadTask *task)
{
    try
    {
        DumpContext context;

        mfxLoader::Plugin* pCtx = ((mfxLoader::Plugin*)_pthis);
        if (!pCtx) return MFX_ERR_INVALID_HANDLE;
        mfxHDL pthis = pCtx->callbacks[emfxVideoCodecPlugin_ENCFrameSubmit_tracer][1];
        context.context = DUMPCONTEXT_MFX;

        Log::WriteLog("callback: mfxVideoCodecPlugin::ENCFrameSubmit(mfxHDL pthis=" + ToString(pthis)
            + ", mfxENCInput *in=" + ToString(in)
            + ", mfxENCOutput *out=" + ToString(out)
            + ", mfxThreadTask *task=" + ToString(task) + ") +");

        fmfxVideoCodecPlugin_ENCFrameSubmit proc = (fmfxVideoCodecPlugin_ENCFrameSubmit)pCtx->callbacks[emfxVideoCodecPlugin_ENCFrameSubmit_tracer][0];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        Log::WriteLog(context.dump("pthis", pthis));

        Timer t;
        mfxStatus status = (*proc) (pthis, in, out, task);
        std::string elapsed = TimeToString(t.GetTime());
        Log::WriteLog(">> callback: mfxVideoCodecPlugin::ENCFrameSubmit called");
        if (task) Log::WriteLog(context.dump("task", *task));
        Log::WriteLog("callback: mfxVideoCodecPlugin::ENCFrameSubmit(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}
#endif //TRACE_CALLBACKS

mfxStatus MFXVideoUSER_Register(mfxSession session, mfxU32 type, const mfxPlugin *par)
{
    try {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoUSER_Register(mfxSession session=" + ToString(session) + ", mfxU32 type=" + ToString(type) + ", mfxPlugin *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoUSER_Register_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump_mfxU32("type", type));
        if (par) Log::WriteLog(context.dump("par", *par));

#if TRACE_CALLBACKS
        INIT_CALLBACK_BACKUP(loader->plugin[type].callbacks);
        mfxPlugin proxyPar;

        if (par && type < (sizeof(loader->plugin) / sizeof(loader->plugin[0])))
        {
            proxyPar = *par;
            par = &proxyPar;

            if (loader->plugin[type].pLoaderBase != loader)
                loader->plugin[type].pLoaderBase = loader;

            SET_CALLBACK(mfxPlugin, proxyPar., PluginInit,      proxyPar.pthis);
            SET_CALLBACK(mfxPlugin, proxyPar., PluginClose,     proxyPar.pthis);
            SET_CALLBACK(mfxPlugin, proxyPar., GetPluginParam,  proxyPar.pthis);
            SET_CALLBACK(mfxPlugin, proxyPar., Execute,         proxyPar.pthis);
            SET_CALLBACK(mfxPlugin, proxyPar., FreeResources,   proxyPar.pthis);

            if (type == MFX_PLUGINTYPE_VIDEO_GENERAL)
            {
                SET_CALLBACK(mfxPlugin, proxyPar., Submit, proxyPar.pthis);
            }
            else
            {
                SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, Query,          proxyPar.pthis);
                SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, QueryIOSurf,    proxyPar.pthis);
                SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, Init,           proxyPar.pthis);
                SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, Reset,          proxyPar.pthis);
                SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, Close,          proxyPar.pthis);
                SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, GetVideoParam,  proxyPar.pthis);

                switch (type)
                {
                case MFX_PLUGINTYPE_VIDEO_DECODE:
                    SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, DecodeHeader,       proxyPar.pthis);
                    SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, GetPayload,         proxyPar.pthis);
                    SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, DecodeFrameSubmit,  proxyPar.pthis);
                    break;
                case MFX_PLUGINTYPE_VIDEO_ENCODE:
                    SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, EncodeFrameSubmit, proxyPar.pthis);
                    break;
                case MFX_PLUGINTYPE_VIDEO_VPP:
                    SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, VPPFrameSubmit,     proxyPar.pthis);
                    SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, VPPFrameSubmitEx,   proxyPar.pthis);
                    break;
                case MFX_PLUGINTYPE_VIDEO_ENC:
                    SET_CALLBACK(mfxVideoCodecPlugin, proxyPar.Video->, ENCFrameSubmit, proxyPar.pthis);
                    break;
                default:
                    break;
                }
            }

            if (proxyPar.pthis)
                proxyPar.pthis = &loader->plugin[type];
        }
#endif //TRACE_CALLBACKS

        Timer t;
        mfxStatus status = (*(fMFXVideoUSER_Register) proc)(session, type, par);
        std::string elapsed = TimeToString(t.GetTime());

        Log::WriteLog(">> MFXVideoUSER_Register called");

        //No need to dump input-only parameters twice!!!
        //Log::WriteLog(context.dump("session", session));
        //Log::WriteLog(context.dump_mfxU32("type", type));
        //if (par) Log::WriteLog(context.dump("par", *par));

        Log::WriteLog("function: MFXVideoUSER_Register(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

#if TRACE_CALLBACKS
        if (status < MFX_ERR_NONE)
            callbacks.RevertAll();
#endif //TRACE_CALLBACKS

        return status;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoUSER_Unregister(mfxSession session, mfxU32 type)
{
    try {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoUSER_Unregister(mfxSession session=" + ToString(session) + ", mfxU32 type=" + ToString(type) + ", mfxPlugin *par=" + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoUSER_Unregister_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump_mfxU32("type", type));

        Timer t;
        mfxStatus status = (*(fMFXVideoUSER_Unregister) proc) (session, type);
        std::string elapsed = TimeToString(t.GetTime());

        Log::WriteLog(">> MFXVideoUSER_Unregister called");

        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump_mfxU32("type", type));

        Log::WriteLog("function: MFXVideoUSER_Unregister(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoUSER_ProcessFrameAsync(mfxSession session, const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxSyncPoint *syncp)
{
    try {
        if (Log::GetLogLevel() >= LOG_LEVEL_FULL) // call with logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_MFX;
            TracerSyncPoint sp;
            if (syncp) {
                sp.syncPoint = (*syncp);
            }
            else {
                sp.syncPoint = NULL;
            }

            Log::WriteLog("function: MFXVideoUSER_ProcessFrameAsync(mfxSession session=, const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxSyncPoint *syncp) +");
            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoUSER_ProcessFrameAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;
            Log::WriteLog(context.dump("session", session));
            Log::WriteLog(context.dump_mfxHDL("in", in));
            Log::WriteLog(context.dump_mfxU32("in_num", in_num));
            Log::WriteLog(context.dump_mfxHDL("out", out));
            Log::WriteLog(context.dump_mfxU32("out_num", out_num));
            Log::WriteLog(context.dump("syncp", sp.syncPoint));

            Timer t;
            mfxStatus status = (*(fMFXVideoUSER_ProcessFrameAsync) proc) (session, in, in_num, out, out_num, syncp);
            std::string elapsed = TimeToString(t.GetTime());
            if (syncp) {
                sp.syncPoint = (*syncp);
            }
            else {
                sp.syncPoint = NULL;
            }
            Log::WriteLog(">> MFXVideoUSER_ProcessFrameAsync called");

            Log::WriteLog(context.dump("session", session));
            Log::WriteLog(context.dump_mfxHDL("in", in));
            Log::WriteLog(context.dump_mfxU32("in_num", in_num));
            Log::WriteLog(context.dump_mfxHDL("out", out));
            Log::WriteLog(context.dump_mfxU32("out_num", out_num));
            Log::WriteLog(context.dump("syncp", sp.syncPoint));

            Log::WriteLog("function: MFXVideoUSER_ProcessFrameAsync(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");

            return status;
        }
        else // call without logging
        {
            DumpContext context;
            context.context = DUMPCONTEXT_MFX;

            mfxLoader *loader = (mfxLoader*) session;

            if (!loader) return MFX_ERR_INVALID_HANDLE;

            mfxFunctionPointer proc = loader->table[eMFXVideoUSER_ProcessFrameAsync_tracer];
            if (!proc) return MFX_ERR_INVALID_HANDLE;

            session = loader->session;
            
            mfxStatus status = (*(fMFXVideoUSER_ProcessFrameAsync) proc) (session, in, in_num, out, out_num, syncp);

            return status;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus MFXVideoUSER_GetPlugin(mfxSession session, mfxU32 type, mfxPlugin *par)
{
    try {
        DumpContext context;
        context.context = DUMPCONTEXT_MFX;
        Log::WriteLog("function: MFXVideoUSER_GetPlugin(mfxSession session=" + ToString(session) + ", mfxU32 type=" + ToString(type) + ", mfxPlugin *par=" + ToString(par) + ") +");
        mfxLoader *loader = (mfxLoader*) session;

        if (!loader) return MFX_ERR_INVALID_HANDLE;

        mfxFunctionPointer proc = loader->table[eMFXVideoUSER_GetPlugin_tracer];
        if (!proc) return MFX_ERR_INVALID_HANDLE;

        session = loader->session;
        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump_mfxU32("type", type));
        if (par) Log::WriteLog(context.dump("par", *par));

        Timer t;
        mfxStatus status = (*(fMFXVideoUSER_GetPlugin) proc)(session, type, par);
        std::string elapsed = TimeToString(t.GetTime());

        Log::WriteLog(">> MFXVideoUSER_GetPlugin called");

        Log::WriteLog(context.dump("session", session));
        Log::WriteLog(context.dump_mfxU32("type", type));
        if (par) Log::WriteLog(context.dump("par", *par));

        Log::WriteLog("function: MFXVideoUSER_GetPlugin(" + elapsed + ", " + context.dump_mfxStatus("status", status) + ") - \n\n");
        return status;
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return MFX_ERR_ABORTED;
    }
}

mfxStatus  MFXAudioUSER_Register(mfxSession session, mfxU32 type, const mfxPlugin *par) { (void)session; (void)type; (void)par; return MFX_ERR_NONE; }
mfxStatus  MFXAudioUSER_Unregister(mfxSession session, mfxU32 type) { (void)session; (void)type; return MFX_ERR_NONE; }
