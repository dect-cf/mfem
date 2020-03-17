#ifndef HYPSYS_ADVECTION
#define HYPSYS_ADVECTION

#include "hyperbolic_system.hpp"

class Advection : public HyperbolicSystem
{
public:
   explicit Advection(FiniteElementSpace *fes_, BlockVector &u_block,
                      Configuration &config_);
   ~Advection() { };

   virtual void EvaluateFlux(const Vector &u, DenseMatrix &FluxEval,
                             int e, int k, int i = -1) const;
   virtual double GetWaveSpeed(const Vector &u, const Vector n, int e, int k, int i) const;
   virtual double EvaluateBdrCond(const Vector &inflow, const Vector &x, const Vector &normal,
                                  int n, int e, int i, int attr, int DofInd) const;
   virtual void ComputeErrors(Array<double> &errors, const GridFunction &u,
                              double DomainSize, double t) const;

   int nqf;
   DenseTensor VelElem, VelFace;
   mutable Vector VelocityVector;
};

#endif
