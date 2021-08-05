// Copyright (c) 2010-2021, Lawrence Livermore National Security, LLC. Produced
// at the Lawrence Livermore National Laboratory. All Rights reserved. See files
// LICENSE and NOTICE for details. LLNL-CODE-806117.
//
// This file is part of the MFEM library. For more information and source code
// availability visit https://mfem.org.
//
// MFEM is free software; you can redistribute it and/or modify it under the
// terms of the BSD-3 license. We welcome feedback and contributions, see file
// CONTRIBUTING.md for details.

#include "../general/forall.hpp"
#include "gridfunc.hpp"
#include "restriction.hpp"

using namespace std;

namespace mfem
{

template<int D1D, int Q1D>
void NDK_SmemPAMassApply3D(const int ndofs,
                           const int NE,
                           const int *map,
                           const double *b_,
                           const double *d_,
                           const double *x_,
                           double *y_)
{
   const auto MAP = Reshape(map, D1D,D1D,D1D, NE);
   const auto b = Reshape(b_, Q1D, D1D);
   const auto D = Reshape(d_, Q1D, Q1D, Q1D, NE);
   const auto X = Reshape(x_, ndofs);
   const auto X1 = Reshape(x_, D1D,D1D,D1D, NE);
   auto Y = Reshape(y_, ndofs);
   auto Y1 = Reshape(y_, D1D,D1D,D1D, NE);
   MFEM_FORALL_3D(e, NE, Q1D, Q1D, 1,
   {
      MFEM_SHARED double sDQ[Q1D*Q1D];
      double (*B)[D1D] = (double (*)[D1D]) sDQ;
      double (*Bt)[Q1D] = (double (*)[Q1D]) sDQ;
      MFEM_SHARED double sm0[Q1D*Q1D*Q1D];
      MFEM_SHARED double sm1[Q1D*Q1D*Q1D];
      double (*DDQ)[D1D][Q1D] = (double (*)[D1D][Q1D]) sm1;
      double (*DQQ)[Q1D][Q1D] = (double (*)[Q1D][Q1D]) sm0;
      double (*QQQ)[Q1D][Q1D] = (double (*)[Q1D][Q1D]) sm1;
      double (*QQD)[Q1D][D1D] = (double (*)[Q1D][D1D]) sm0;
      double (*QDD)[D1D][D1D] = (double (*)[D1D][D1D]) sm1;
      MFEM_FOREACH_THREAD(dy,y,D1D)
      {
         MFEM_FOREACH_THREAD(dx,x,Q1D)
         {
            B[dx][dy] = b(dx,dy);
         }
      }
      MFEM_SYNC_THREAD;
      MFEM_FOREACH_THREAD(dy,y,D1D)
      {
         MFEM_FOREACH_THREAD(qx,x,Q1D)
         {
            double u[D1D];
            MFEM_UNROLL(D1D)
            for (int dz = 0; dz < D1D; dz++)
            {
               u[dz] = 0;
            }
            MFEM_UNROLL(D1D)
            for (int dx = 0; dx < D1D; ++dx)
            {
               MFEM_UNROLL(D1D)
               for (int dz = 0; dz < D1D; ++dz)
               {
                  if (map)
                  {
                     const int gid = MAP(dx, dy, dz, e);
                     const int idx = gid >= 0 ? gid : -1 - gid;
                     u[dz] += X(idx) * B[qx][dx];
                  }
                  else
                  {
                     u[dz] += X1(dx,dy,dz,e) * B[qx][dx];
                  }
               }
            }
            MFEM_UNROLL(D1D)
            for (int dz = 0; dz < D1D; ++dz)
            {
               DDQ[dz][dy][qx] = u[dz];
            }
         }
      }
      MFEM_SYNC_THREAD;
      MFEM_FOREACH_THREAD(qy,y,Q1D)
      {
         MFEM_FOREACH_THREAD(qx,x,Q1D)
         {
            double u[D1D];
            MFEM_UNROLL(D1D)
            for (int dz = 0; dz < D1D; dz++)
            {
               u[dz] = 0;
            }
            MFEM_UNROLL(D1D)
            for (int dy = 0; dy < D1D; ++dy)
            {
               MFEM_UNROLL(D1D)
               for (int dz = 0; dz < D1D; dz++)
               {
                  u[dz] += DDQ[dz][dy][qx] * B[qy][dy];
               }
            }
            MFEM_UNROLL(D1D)
            for (int dz = 0; dz < D1D; dz++)
            {
               DQQ[dz][qy][qx] = u[dz];
            }
         }
      }
      MFEM_SYNC_THREAD;
      MFEM_FOREACH_THREAD(qy,y,Q1D)
      {
         MFEM_FOREACH_THREAD(qx,x,Q1D)
         {
            double u[Q1D];
            MFEM_UNROLL(Q1D)
            for (int qz = 0; qz < Q1D; qz++)
            {
               u[qz] = 0;
            }
            MFEM_UNROLL(D1D)
            for (int dz = 0; dz < D1D; ++dz)
            {
               MFEM_UNROLL(Q1D)
               for (int qz = 0; qz < Q1D; qz++)
               {
                  u[qz] += DQQ[dz][qy][qx] * B[qz][dz];
               }
            }
            MFEM_UNROLL(Q1D)
            for (int qz = 0; qz < Q1D; qz++)
            {
               QQQ[qz][qy][qx] = u[qz] * D(qx,qy,qz,e);
            }
         }
      }
      MFEM_SYNC_THREAD;
      MFEM_FOREACH_THREAD(d,y,D1D)
      {
         MFEM_FOREACH_THREAD(q,x,Q1D)
         {
            Bt[d][q] = b(q,d);
         }
      }
      MFEM_SYNC_THREAD;
      MFEM_FOREACH_THREAD(qy,y,Q1D)
      {
         MFEM_FOREACH_THREAD(dx,x,D1D)
         {
            double u[Q1D];
            MFEM_UNROLL(Q1D)
            for (int qz = 0; qz < Q1D; ++qz)
            {
               u[qz] = 0;
            }
            MFEM_UNROLL(Q1D)
            for (int qx = 0; qx < Q1D; ++qx)
            {
               MFEM_UNROLL(Q1D)
               for (int qz = 0; qz < Q1D; ++qz)
               {
                  u[qz] += QQQ[qz][qy][qx] * Bt[dx][qx];
               }
            }
            MFEM_UNROLL(Q1D)
            for (int qz = 0; qz < Q1D; ++qz)
            {
               QQD[qz][qy][dx] = u[qz];
            }
         }
      }
      MFEM_SYNC_THREAD;
      MFEM_FOREACH_THREAD(dy,y,D1D)
      {
         MFEM_FOREACH_THREAD(dx,x,D1D)
         {
            double u[Q1D];
            MFEM_UNROLL(Q1D)
            for (int qz = 0; qz < Q1D; ++qz)
            {
               u[qz] = 0;
            }
            MFEM_UNROLL(Q1D)
            for (int qy = 0; qy < Q1D; ++qy)
            {
               MFEM_UNROLL(Q1D)
               for (int qz = 0; qz < Q1D; ++qz)
               {
                  u[qz] += QQD[qz][qy][dx] * Bt[dy][qy];
               }
            }
            MFEM_UNROLL(Q1D)
            for (int qz = 0; qz < Q1D; ++qz)
            {
               QDD[qz][dy][dx] = u[qz];
            }
         }
      }
      MFEM_SYNC_THREAD;
      MFEM_FOREACH_THREAD(dy,y,D1D)
      {
         MFEM_FOREACH_THREAD(dx,x,D1D)
         {
            double u[D1D];
            MFEM_UNROLL(D1D)
            for (int dz = 0; dz < D1D; ++dz)
            {
               u[dz] = 0;
            }
            MFEM_UNROLL(Q1D)
            for (int qz = 0; qz < Q1D; ++qz)
            {
               MFEM_UNROLL(D1D)
               for (int dz = 0; dz < D1D; ++dz)
               {
                  u[dz] += QDD[qz][dy][dx] * Bt[dz][qz];
               }
            }
            MFEM_UNROLL(D1D)
            for (int dz = 0; dz < D1D; ++dz)
            {
               if (map)
               {
                  const int gid = MAP(dx, dy, dz, e);
                  const int idx = gid >= 0 ? gid : -1 - gid;
                  AtomicAdd(Y(idx), u[dz]);
               }
               else
               {
                  Y1(dx,dy,dz,e) += u[dz];
               }
            }
         }
      }
   });
}

template<int D1D, int Q1D>
void NDK_RegsPAMassApply3D(const int ndofs,
                           const int NE,
                           const int *map,
                           const double *b_,
                           const double *d_,
                           const double *x_,
                           double *y_)
{
   const auto MAP = Reshape(map, D1D,D1D,D1D, NE);
   const auto B = Reshape(b_, Q1D, D1D);
   const auto D = Reshape(d_, Q1D, Q1D, Q1D, NE);
   const auto X = Reshape(x_, ndofs);
   const auto X1 = Reshape(x_, D1D,D1D,D1D, NE);
   auto Y = Reshape(y_, ndofs);
   auto Y1 = Reshape(y_, D1D,D1D,D1D, NE);

   MFEM_FORALL_3D(e, NE, Q1D, Q1D, 1,
   {
      double r_wk[Q1D];
      MFEM_SHARED double s_B[Q1D][D1D];
      MFEM_SHARED double s_q[Q1D][Q1D][Q1D];

      MFEM_FOREACH_THREAD(b,y,Q1D)
      {
         MFEM_FOREACH_THREAD(a,x,Q1D)
         {
            if (a<D1D) { s_B[b][a] = B(b,a); }

            for (int i=0; i<Q1D; ++i) { r_wk[i] = 0.0; }

            if (a<D1D && b<D1D)
            {
               MFEM_UNROLL(D1D)
               for (int c=0; c<D1D; ++c)
               {
                  if (map)
                  {
                     const int gid = MAP(a,b,c,e);
                     const int idx = gid >= 0 ? gid : -1 - gid;
                     s_q[c][b][a] = X(idx);
                  }
                  else
                  {
                     s_q[c][b][a] = X1(a,b,c,e);
                  }
               }
            }
         }
      }
      MFEM_SYNC_THREAD;

      MFEM_FOREACH_THREAD(c,y,Q1D)
      {
         MFEM_FOREACH_THREAD(b,x,Q1D)
         {
            if (b<D1D && c<D1D)
            {
               MFEM_UNROLL(D1D)
               for (int a=0; a<D1D; ++a)
               {
                  const double q_cba = s_q[c][b][a];
                  MFEM_UNROLL(Q1D)
                  for (int i=0; i<Q1D; ++i) { r_wk[i] += s_B[i][a]*q_cba; }
               }
               MFEM_UNROLL(Q1D)
               for (int i=0; i<Q1D; ++i) { s_q[c][b][i] = r_wk[i]; }
            }
            MFEM_UNROLL(Q1D)
            for (int j=0; j<Q1D; ++j) { r_wk[j] = 0.0; }
         }
      }
      MFEM_SYNC_THREAD;

      MFEM_FOREACH_THREAD(c,y,D1D)
      {
         MFEM_FOREACH_THREAD(i,x,Q1D)
         {
            if (i<Q1D && c<D1D)
            {
               MFEM_UNROLL(D1D)
               for (int b=0; b<D1D; ++b)
               {
                  const double q_cbi = s_q[c][b][i];
                  MFEM_UNROLL(Q1D)
                  for (int j=0; j<Q1D; ++j) { r_wk[j] += s_B[j][b]*q_cbi; }
               }
               MFEM_UNROLL(Q1D)
               for (int j=0; j<Q1D; ++j) { s_q[c][j][i] = r_wk[j]; }
            }
            MFEM_UNROLL(Q1D)
            for (int k=0; k<Q1D; ++k) { r_wk[k] = 0.0; }
         }
      }
      MFEM_SYNC_THREAD;

      MFEM_FOREACH_THREAD(j,y,Q1D)
      {
         MFEM_FOREACH_THREAD(i,x,Q1D)
         {
            MFEM_UNROLL(D1D)
            for (int c=0; c<D1D; ++c)
            {
               const double q_cji = s_q[c][j][i];
               MFEM_UNROLL(Q1D)
               for (int k=0; k<Q1D; ++k) { r_wk[k] += s_B[k][c]*q_cji; }
            }
            MFEM_UNROLL(Q1D)
            for (int k=0; k<Q1D; ++k) { r_wk[k] *= D(i,j,k,e); }
            for (int c=0; c<D1D; ++c)
            {
               double q_cji = 0.0;
               MFEM_UNROLL(Q1D)
               for (int k=0; k<Q1D; ++k) { q_cji += s_B[k][c] * r_wk[k]; }
               s_q[c][j][i] = q_cji;
            }
         }
      }
      MFEM_SYNC_THREAD;

      MFEM_FOREACH_THREAD(c,y,D1D)
      {
         MFEM_FOREACH_THREAD(i,x,Q1D)
         {
            for (int j=0; j<Q1D; ++j) { r_wk[j] = s_q[c][j][i]; }
            MFEM_UNROLL(D1D)
            for (int b=0; b<D1D; ++b)
            {
               double q_cbi = 0.0;
               MFEM_UNROLL(Q1D)
               for (int j=0; j<Q1D; ++j) { q_cbi += s_B[j][b] * r_wk[j]; }
               s_q[c][b][i] = q_cbi;
            }
         }
      }
      MFEM_SYNC_THREAD;

      MFEM_FOREACH_THREAD(c,y,D1D)
      {
         MFEM_FOREACH_THREAD(b,x,D1D)
         {
            for (int i=0; i<Q1D; ++i) { r_wk[i] = s_q[c][b][i]; }
            MFEM_UNROLL(D1D)
            for (int a=0; a<D1D; ++a)
            {
               double q_cba = 0.0;
               MFEM_UNROLL(Q1D)
               for (int i=0; i<Q1D; ++i) { q_cba += s_B[i][a] * r_wk[i]; }
               s_q[c][b][a] = q_cba;
            }
         }
      }
      MFEM_SYNC_THREAD;

      MFEM_FOREACH_THREAD(b,y,D1D)
      {
         MFEM_FOREACH_THREAD(a,x,D1D)
         {
            MFEM_UNROLL(D1D)
            for (int c=0; c<D1D; ++c)
            {
               const double q_cba = s_q[c][b][a];
               if (map)
               {
                  const int gid = MAP(a,b,c,e);
                  const int idx = gid >= 0 ? gid : -1 - gid;
                  AtomicAdd(Y(idx), q_cba);
               }
               else
               {
                  Y1(a,b,c,e) += q_cba;
               }
            }
         }
      }
      MFEM_SYNC_THREAD;
   });
}

void NDK_PAMassApply(const int dim,
                     const int D1D,
                     const int Q1D,
                     const int NE,
                     const FiniteElementSpace *fes,
                     const DofToQuad *maps,
                     const Vector &D,
                     const Vector &X,
                     Vector &Y)
{
   const int ndofs = fes->GetNDofs();
   constexpr ElementDofOrdering ordering = ElementDofOrdering::LEXICOGRAPHIC;
   const Operator *ERop = fes->GetElementRestriction(ordering);
   const ElementRestriction* ER = dynamic_cast<const ElementRestriction*>(ERop);
   const int *map = ER ? ER->GatherMap().Read() : nullptr;
   const double *b = maps->B.Read();
   const double *d = D.Read();
   const double *x = X.Read();
   double *y = Y.ReadWrite();

   assert(dim == 3);
   const int ver = DeviceKernelsVersion();
   const int id = (ver << 8) | (D1D << 4) | Q1D;

   switch (id) // orders 1~6
   {
      case 0x023: return NDK_SmemPAMassApply3D<2,3>(ndofs,NE,map,b,d,x,y);
      case 0x024: return NDK_SmemPAMassApply3D<2,4>(ndofs,NE,map,b,d,x,y);
      case 0x034: return NDK_SmemPAMassApply3D<3,4>(ndofs,NE,map,b,d,x,y);
      case 0x036: return NDK_SmemPAMassApply3D<3,6>(ndofs,NE,map,b,d,x,y);
      case 0x045: return NDK_SmemPAMassApply3D<4,5>(ndofs,NE,map,b,d,x,y);
      case 0x046: return NDK_SmemPAMassApply3D<4,6>(ndofs,NE,map,b,d,x,y);
      case 0x048: return NDK_SmemPAMassApply3D<4,8>(ndofs,NE,map,b,d,x,y);
      case 0x056: return NDK_SmemPAMassApply3D<5,6>(ndofs,NE,map,b,d,x,y);
      case 0x058: return NDK_SmemPAMassApply3D<5,8>(ndofs,NE,map,b,d,x,y);
      case 0x067: return NDK_SmemPAMassApply3D<6,7>(ndofs,NE,map,b,d,x,y);
      case 0x078: return NDK_SmemPAMassApply3D<7,8>(ndofs,NE,map,b,d,x,y);

      case 0x123: return NDK_RegsPAMassApply3D<2,3>(ndofs,NE,map,b,d,x,y);
      case 0x124: return NDK_RegsPAMassApply3D<2,4>(ndofs,NE,map,b,d,x,y);
      case 0x134: return NDK_RegsPAMassApply3D<3,4>(ndofs,NE,map,b,d,x,y);
      case 0x136: return NDK_RegsPAMassApply3D<3,6>(ndofs,NE,map,b,d,x,y);
      case 0x145: return NDK_RegsPAMassApply3D<4,5>(ndofs,NE,map,b,d,x,y);
      case 0x146: return NDK_RegsPAMassApply3D<4,6>(ndofs,NE,map,b,d,x,y);
      case 0x148: return NDK_RegsPAMassApply3D<4,8>(ndofs,NE,map,b,d,x,y);
      case 0x156: return NDK_RegsPAMassApply3D<5,6>(ndofs,NE,map,b,d,x,y);
      case 0x158: return NDK_RegsPAMassApply3D<5,8>(ndofs,NE,map,b,d,x,y);
      case 0x167: return NDK_RegsPAMassApply3D<6,7>(ndofs,NE,map,b,d,x,y);
      case 0x178: return NDK_RegsPAMassApply3D<7,8>(ndofs,NE,map,b,d,x,y);
      default: break;
   }

   MFEM_ABORT("Unknown kernel 0x" << std::hex << id);
}

} // namespace mfem
