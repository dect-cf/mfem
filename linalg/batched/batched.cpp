// Copyright (c) 2010-2024, Lawrence Livermore National Security, LLC. Produced
// at the Lawrence Livermore National Laboratory. All Rights reserved. See files
// LICENSE and NOTICE for details. LLNL-CODE-806117.
//
// This file is part of the MFEM library. For more information and source code
// availability visit https://mfem.org.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the BSD-3 license. We welcome feedback and contributions, see file
// CONTRIBUTING.md for details.

#include "batched.hpp"
#include "native.hpp"
#include "gpu_blas.hpp"
#include "magma.hpp"

namespace mfem
{

BatchedLinAlg::BatchedLinAlg()
{
   backends[NATIVE].reset(new NativeBatchedLinAlg);

   if (Device::Allows(~mfem::Backend::CPU_MASK))
   {
#ifdef MFEM_USE_CUDA_OR_HIP
      backends[GPU_BLAS].reset(new GPUBlasBatchedLinAlg);
#endif

#ifdef MFEM_USE_MAGMA
      backends[MAGMA].reset(new MagmaBatchedLinAlg);
#endif

#if defined(MFEM_USE_MAGMA)
      preferred_backend = MAGMA;
#elif defined(MFEM_USE_CUDA_OR_HIP)
      preferred_backend = GPU_BLAS;
#else
      preferred_backend = NATIVE;
#endif
   }
   else
   {
      preferred_backend = NATIVE;
   }
}

BatchedLinAlg &BatchedLinAlg::Instance()
{
   static BatchedLinAlg instance;
   return instance;
}

void BatchedLinAlg::Mult(const DenseTensor &A, const Vector &x, Vector &y)
{
   Get(Instance().preferred_backend).Mult(A, x, y);
}

void BatchedLinAlg::Invert(DenseTensor &A)
{
   Get(Instance().preferred_backend).Invert(A);
}

void BatchedLinAlg::LUFactor(DenseTensor &A, Array<int> &P)
{
   Get(Instance().preferred_backend).LUFactor(A, P);
}

void BatchedLinAlg::LUSolve(const DenseTensor &A, const Array<int> &P,
                            Vector &x)
{
   Get(Instance().preferred_backend).LUSolve(A, P, x);
}

bool BatchedLinAlg::IsAvailable(BatchedLinAlg::Backend backend)
{
   return Instance().backends[backend] != nullptr;
}

void BatchedLinAlg::SetPreferredBackend(BatchedLinAlg::Backend backend)
{
   MFEM_VERIFY(IsAvailable(backend), "Requested backend not supported.");
   Instance().preferred_backend = backend;
}

BatchedLinAlg::Backend BatchedLinAlg::GetPreferredBackend()
{
   return Instance().preferred_backend;
}

const BatchedLinAlgBase &BatchedLinAlg::Get(BatchedLinAlg::Backend backend)
{
   auto &backend_ptr = Instance().backends[backend];
   MFEM_VERIFY(backend_ptr, "Requested backend not supported.")
   return *backend_ptr;
}

}
