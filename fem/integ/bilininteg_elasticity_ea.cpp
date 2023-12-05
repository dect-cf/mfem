// Copyright (c) 2010-2023, Lawrence Livermore National Security, LLC. Produced
// at the Lawrence Livermore National Laboratory. All Rights reserved. See files
// LICENSE and NOTICE for details. LLNL-CODE-806117.
//
// This file is part of the MFEM library. For more information and source code
// availability visit https://mfem.org.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the BSD-3 license. We welcome feedback and contributions, see file
// CONTRIBUTING.md for details.

#include "../../general/forall.hpp"
#include "../bilininteg.hpp"
#include "bilininteg_elasticity_kernels.hpp"

namespace mfem
{
void ElasticityIntegrator::AssembleEA(const FiniteElementSpace &fes,
                                      Vector &emat,
                                      const bool add)
{
   MFEM_VERIFY(parent, "Element level assembly for component version only");
   MFEM_VERIFY(fespace, "Need initialized FiniteElementSpace.");
   MFEM_VERIFY(!add, "AssembleEA not implemented for add yet.");
   AssemblePA(*fespace);
   const auto &ir = q_vec->GetIntRule(0);
   internal::ElasticityAssembleEA(vdim, IBlock, JBlock, ndofs, ir, *fespace,
                                  *lambda_quad, *mu_quad, *geom, *maps, emat);
}
}
