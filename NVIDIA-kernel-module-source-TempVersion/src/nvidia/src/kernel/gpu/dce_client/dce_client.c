/*
 * SPDX-FileCopyrightText: Copyright (c) 2020-2021 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "gpu/dce_client/dce_client.h"

ROOT roots[MAX_RM_CLIENTS];
DEVICE devices[MAX_RM_CLIENTS];
SUBDEVICE subdevices[MAX_RM_CLIENTS];
DISPLAY_COMMON display;
DISPLAY_SW displaySW;

NV_STATUS
dceclientConstructEngine_IMPL
(
    OBJGPU        *pGpu,
    DceClient     *pDceClient,
    ENGDESCRIPTOR engDesc
)
{
    NV_PRINTF(LEVEL_INFO, "dceclientConstructEngine_IMPL Called\n");

    return dceclientInitRpcInfra(pGpu, pDceClient);
}

NV_STATUS
dceclientStateLoad_IMPL
(
    OBJGPU        *pGpu,
    DceClient     *pDceClient,
    NvU32         flags
)
{
    NV_STATUS nvStatus = NV_OK;
    NV_PRINTF(LEVEL_INFO, "dceclientStateLoad_IMPL Called\n");

    if (!(flags & GPU_STATE_FLAGS_PM_TRANSITION))
        return NV_OK;

    nvStatus = dceclientInitRpcInfra(pGpu, pDceClient);
    if (nvStatus != NV_OK)
    {
        NV_PRINTF(LEVEL_ERROR, "dceclientInitRpcInfra failed\n");
        goto out;
    }

    nvStatus = dceclientDceRmInit(pGpu, GPU_GET_DCECLIENTRM(pGpu), NV_TRUE);
    if (nvStatus != NV_OK)
    {
        NV_PRINTF(LEVEL_ERROR, "Cannot load DCE firmware RM\n");
        nvStatus = NV_ERR_GENERIC;
        goto out;
    }

    if (pGpu->getProperty(pGpu, PDB_PROP_GPU_IN_PM_RESUME_CODEPATH))
    {
        RM_API *pRmApi = GPU_GET_PHYSICAL_RMAPI(pGpu);
        NvU32  i = 0;

        for (i = 0; i < MAX_RM_CLIENTS; i++)
        {
            if (roots[i].valid)
            {
                nvStatus = rpcRmApiAlloc_dce(pRmApi, roots[i].hClient, roots[i].hParent,
                                             roots[i].hObject, roots[i].hClass, &roots[i].rootAllocParams);
                if (nvStatus != NV_OK)
                {
                    NV_PRINTF(LEVEL_ERROR, "Cannot alloc roots[%u] object during resume\n",i);
                    nvStatus = NV_ERR_GENERIC;
                    goto out;
                }
            }

            if (devices[i].valid)
            {
                nvStatus = rpcRmApiAlloc_dce(pRmApi, devices[i].hClient, devices[i].hParent,
                                             devices[i].hObject, devices[i].hClass, &devices[i].deviceAllocParams);
                if (nvStatus != NV_OK)
                {
                    NV_PRINTF(LEVEL_ERROR, "Cannot alloc devices[%u] object during resume\n",i);
                    nvStatus = NV_ERR_GENERIC;
                    goto out;
                }
            }

            if (subdevices[i].valid)
            {
                nvStatus = rpcRmApiAlloc_dce(pRmApi, subdevices[i].hClient, subdevices[i].hParent,
                                             subdevices[i].hObject, subdevices[i].hClass, &subdevices[i].subdeviceAllocParams);
                if (nvStatus != NV_OK)
                {
                    NV_PRINTF(LEVEL_ERROR, "Cannot alloc subdevices[%u] object during resume\n",i);
                    nvStatus = NV_ERR_GENERIC;
                    goto out;
                }
            }
        }

        if (display.valid)
        {
            nvStatus = rpcRmApiAlloc_dce(pRmApi, display.hClient, display.hParent,
                                         display.hObject, display.hClass, &display.displayCommonAllocParams);
            if (nvStatus != NV_OK)
            {
                NV_PRINTF(LEVEL_ERROR, "Cannot alloc display_common object during resume\n");
                nvStatus = NV_ERR_GENERIC;
                goto out;
            }

        }

        if (displaySW.valid)
        {
            nvStatus = rpcRmApiAlloc_dce(pRmApi, displaySW.hClient, displaySW.hParent,
                                         displaySW.hObject, displaySW.hClass, &displaySW.displaySWAllocParams);
            if (nvStatus != NV_OK)
            {
                NV_PRINTF(LEVEL_ERROR, "Cannot alloc displaySW object during resume\n");
                nvStatus = NV_ERR_GENERIC;
                goto out;
            }
        }
    }

out:
    return nvStatus;
}

NV_STATUS
dceclientStateUnload_IMPL
(
    OBJGPU        *pGpu,
    DceClient     *pDceClient,
    NvU32         flags
)
{
    NV_STATUS nvStatus = NV_OK;
    NV_PRINTF(LEVEL_INFO, "dceclientStateUnload_IMPL Called\n");

    if (!(flags & GPU_STATE_FLAGS_PM_TRANSITION))
        return NV_OK;

    nvStatus = dceclientDceRmInit(pGpu, GPU_GET_DCECLIENTRM(pGpu), NV_FALSE);
    if (nvStatus != NV_OK)
    {
        NV_PRINTF(LEVEL_ERROR, "Cannot unload DCE firmware RM\n");
    }

    dceclientDeinitRpcInfra(pDceClient);

    return nvStatus;
}

void
dceclientStateDestroy_IMPL
(
    OBJGPU *pGpu,
    DceClient *pDceClient
)
{
    NvU32 i = 0;

    NV_PRINTF(LEVEL_INFO, "Destroy DCE Client Object Called\n");

    dceclientDeinitRpcInfra(pDceClient);

    for (i = 0; i < MAX_RM_CLIENTS; i++)
    {
        roots[i].valid      = NV_FALSE;
        devices[i].valid    = NV_FALSE;
        subdevices[i].valid = NV_FALSE;
    }
    display.valid   = NV_FALSE;
    displaySW.valid = NV_FALSE;
}
