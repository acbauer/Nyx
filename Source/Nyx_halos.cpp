#ifdef AGN
#include <iomanip>
#include <algorithm>
#include <vector>
#include <iostream>
#include <string>
#include <unistd.h>
#include <math.h>

using std::cout;
using std::cerr;
using std::endl;
using std::istream;
using std::ostream;
using std::pair;
using std::string;

#include <AMReX_CONSTANTS.H>
#include <Nyx.H>
#include <Nyx_F.H>
#include <Derive_F.H>

#include "AGNParticleContainer.H"

#if BL_USE_MPI
#include "MemInfo.H"
#endif

#ifdef REEBER
#include <ReeberAnalysis.H>
#endif /* REEBER */

const int NyxHaloFinderSignal = 42;

// For debugging.
const int nghost0 = 0;
const int nghost1 = 1;
const int ncomp1 = 1;
const int comp0 = 0;

using namespace amrex;

void
Nyx::conserved_to_primitive(amrex::MultiFab& state)
{ // divide every component but Density by Density
  int nghost = state.nGrow();
  for (int comp = 0; comp < state.nComp(); comp++)
    {
      if (comp != Density)
        {
          MultiFab::Divide(state, state, Density, comp, ncomp1, nghost);
        }
    }
}

void
Nyx::primitive_to_conserved(amrex::MultiFab& state)
{
   BL_PROFILE("Nyx::primitive_to_conserved()");
   int nghost = state.nGrow();

   // Multiply every component but Density by Density
   for (int comp = 0; comp < state.nComp(); comp++)
   {
      if (comp != Density)
        {
          MultiFab::Multiply(state, state, Density, comp, ncomp1, nghost);
        }
   }
}

void
Nyx::halo_find (Real dt)
{
   BL_PROFILE("Nyx::halo_find()");

   const int whichSidecar(0);

   const Real * dx = geom.CellSize();

   amrex::MultiFab& new_state = get_new_data(State_Type);
   const BoxArray& simBA = new_state.boxArray();
   const DistributionMapping& simDM = new_state.DistributionMap();
   int simComp = new_state.nComp();

   // First copy the existing state into orig_state.
   MultiFab orig_state(simBA, simDM, simComp, nghost1);
   MultiFab::Copy(orig_state, new_state,
                  comp0, comp0, simComp, nghost1);

   // These are passed into the AGN particles' Redistribute
   int lev_min = 0;
   int lev_max = 0;
   int ngrow   = 0;

#ifdef REEBER
   const auto& reeber_density_var_list = getReeberHaloDensityVars();
   bool do_analysis(doAnalysisNow());

   bool created_file = false;

   if (do_analysis || (reeber_int > 0 && nStep() % reeber_int == 0)) 
   {

     // Before creating new AGN particles, check if any of the existing AGN particles should be merged
     halo_merge();

     // Before creating new AGN particles, accrete mass onto existing particles 
     halo_accrete(dt);

     if (ParallelDescriptor::NProcsSidecar(0) <= 0) 
     { // we have no sidecars, so do everything in situ

       BoxArray reeberBA;
       DistributionMapping reeberDM;
       getAnalysisDecomposition(Geom(), ParallelDescriptor::NProcs(), reeberBA, reeberDM);
       cout << "getAnalysisDecomposition returns BoxArray size " << reeberBA.size() << endl;
       cout << "getAnalysisDecomposition returns DistributionMapping ProcessorMap size " << reeberDM.ProcessorMap().size() << endl;
       amrex::MultiFab reeberMF(reeberBA, reeberDM, reeber_density_var_list.size() + 1, nghost0);
       int cnt = 1;

       Real cur_time = state[State_Type].curTime();

       // Derive quantities and store in components 1... of MultiFAB
       for (auto it = reeber_density_var_list.begin(); it != reeber_density_var_list.end(); ++it)
       {
           std::unique_ptr<MultiFab> derive_dat = particle_derive(*it, cur_time, 0);
           reeberMF.copy(*derive_dat, comp0, cnt, ncomp1, nghost0, nghost0);
           cnt++;
       }

       reeberMF.setVal(0.0, comp0, ncomp1, nghost0);
       for (int comp = 1; comp < reeberMF.nComp(); ++comp)
         {
           amrex::MultiFab::Add(reeberMF, reeberMF,
                                comp, comp0, ncomp1, nghost0);
         }

       std::vector<Halo> reeber_halos;
       runReeberAnalysis(reeberMF, Geom(), nStep(), do_analysis, &reeber_halos);

#else

       // Before creating new AGN particles, check if any of the existing AGN particles should be merged
       halo_merge();

       // Before creating new AGN particles,
       // accrete mass and momentum onto existing particles.
       // No change to state.
       halo_accrete(dt);

       // Here we just create place-holders for the halos which should come from REEBER
       std::vector<IntVect> reeber_halos_pos;
       std::vector<Real>    reeber_halos_mass;

       Real haloMass = 1.1e11;
       const Box& domainBox = grids.minimalBox();
       const IntVect& lo = domainBox.smallEnd();
       const IntVect& hi = domainBox.bigEnd();
       // With box 0:31, will have 0 and 31/2 = 15 and 31.
       int numSplits = 2;
       IntVect stepVect = (hi - lo) / numSplits;
       std::vector<IntVect> vertices(numSplits+1);
       for (int i = 0; i < numSplits; i++)
         {
           vertices[i] = lo + i * stepVect;
         }
       vertices[numSplits] = hi;
       Box vertBox(IntVect::Zero, numSplits*IntVect::Unit);
       for (BoxIterator bit(vertBox); bit.ok(); ++bit)
         {
           IntVect vert = bit();
           int i = vert[0];
           int j = vert[1];
           int k = vert[2];
           IntVect iv(D_DECL(vertices[vert[0]][0],
                             vertices[vert[1]][1],
                             vertices[vert[2]][2]));
           reeber_halos_pos.push_back(iv);
           reeber_halos_mass.push_back(haloMass);
         }

#endif // ifdef REEBER

       amrex::Real    halo_mass;
       amrex::IntVect halo_pos ;

       std::ofstream os;

       std::cout << "  " << std::endl;
       std::cout << " *************************************** " << std::endl;

#ifdef REEBER
       for (const Halo& h : reeber_halos)
       {
#if 0
           // We aren't actually writing to this file so don't create it
           if (reeber_halos_pos.size() > 0)
           {
              if (!created_file)
                 os.open(amrex::Concatenate(amrex::Concatenate("debug-halos-", nStep(), 5), ParallelDescriptor::MyProc(), 2));
              created_file = true;
           }
#endif
           halo_mass = h.totalMass;
           halo_pos  = h.position;
#else

       // Now loop over the halos
       for (int i = 0; i < reeber_halos_pos.size(); i++)
       {
           halo_mass = reeber_halos_mass[i];
           halo_pos  = reeber_halos_pos[i];
#endif

           if (halo_mass > mass_halo_min)
           {
                amrex::Real x = (halo_pos[0]+0.5) * dx[0];
                amrex::Real y = (halo_pos[1]+0.5) * dx[1];
                amrex::Real z = (halo_pos[2]+0.5) * dx[2];
   
                amrex::Real mass = mass_seed;

                int lev = 0;
                int grid = 0;
                int tile = 0;

                // Note that we are going to add the particle into grid 0 and tile 0 at level 0 -- 
                //      this is not actually where the particle belongs, but we will let the Redistribute call
                //      put it in the right place

                Nyx::theAPC()->AddOneParticle(lev,grid,tile,mass,x,y,z); // ,u,v,w);
                std::cout << "ADDED A PARTICLE AT " << x << " " << y << " " << z << " WITH MASS " << mass << std::endl;
           }
       } // end of loop over creating new particles from halos

       std::cout << " *************************************** " << std::endl;
       std::cout << "  " << std::endl;

       // At this point the particles have all been created on the same process as the halo they came from,
       // but they are not on the "right" process for going forward

       // Call Redistribute so that the new particles get their cell, grid and process defined
       Nyx::theAPC()->Redistribute(lev_min, lev_max, ngrow);

       Nyx::theAPC()->fillNeighbors(level);
       Nyx::theAPC()->ComputeOverlap(level);
       Nyx::theAPC()->clearNeighbors(level);

       Nyx::theAPC()->Redistribute(lev_min, lev_max, ngrow);

       // agn_density will hold the density we're going to remove from the grid.
       MultiFab agn_density(simBA, simDM, ncomp1, nghost1);
       agn_density.setVal(0.0);

       // Deposit the mass now in the particles onto the grid.
       // (No change to mass of particles.)
       Nyx::theAPC()->AssignDensitySingleLevel(agn_density, level);

       // Make sure the density put into ghost cells is added to valid regions
       agn_density.SumBoundary(geom.periodicity());

       // Take away the density from the gas that was added to the AGN particle.
       amrex::MultiFab::Subtract(new_state, agn_density,
                                 comp0, Density, ncomp1, nghost0);

       cout << "Going into ComputeParticleVelocity (no energy), number of AGN particles on this proc is "
            << Nyx::theAPC()->TotalNumberOfParticles(true, true) << endl;

       // Re-set the particle velocity (but not energy) after accretion,
       // using change of momentum density in state.
       // No change to state, other than filling ghost cells.
       int add_energy = 0;
       Nyx::theAPC()->ComputeParticleVelocity(level, orig_state, new_state, add_energy);

       cout << "Going into ReleaseEnergy, number of AGN particles on this proc is "
            << Nyx::theAPC()->TotalNumberOfParticles(true, true) << endl;
       // AGN particles: may zero out energy.
       // new_state: may increase internal and total energy.
       MultiFab& D_new = get_new_data(DiagEOS_Type);
       Real a = get_comoving_a(new_a_time);
       Nyx::theAPC()->ReleaseEnergy(level, new_state, D_new, a);
       // Now new_state = get_new_data(State_Type) has been updated.

       //       pout() << "Step " << nStep() << " at end of Nyx_halos:" << endl;
       //       Nyx::theAPC()->writeAllAtLevel(level);

#ifdef REEBER
     } 
#if 0
     else { // we have sidecars, so do everything in-transit

       int sidecarSignal(NyxHaloFinderSignal);
       const int MPI_IntraGroup_Broadcast_Rank = ParallelDescriptor::IOProcessor() ? MPI_ROOT : MPI_PROC_NULL;
       ParallelDescriptor::Bcast(&sidecarSignal, 1, MPI_IntraGroup_Broadcast_Rank,
                                 ParallelDescriptor::CommunicatorInter(whichSidecar));

       Geometry geom(Geom());
       Geometry::SendGeometryToSidecar(&geom, whichSidecar);

       // FIXME: What is distribution mapping?
       amrex::MultiFab reeberMF(grids, reeber_density_var_list.size(), 0);
       int cnt = 0;
       // Derive quantities and store in components 1... of MultiFAB
       for (auto it = reeber_density_var_list.begin(); it != reeber_density_var_list.end(); ++it)
       {
           std::unique_ptr<MultiFab> derive_dat = particle_derive(*it, cur_time, 0);
           reeberMF.copy(*derive_dat, comp0, cnt, ncomp1, nghost0, nghost0);
           cnt++;
       }

       int time_step(nStep()), nComp(reeberMF.nComp());

       ParallelDescriptor::Bcast(&nComp, 1, MPI_IntraGroup_Broadcast_Rank,
                                 ParallelDescriptor::CommunicatorInter(whichSidecar));

       amrex::MultiFab *mfSource = &reeberMF;
       amrex::MultiFab *mfDest = 0;
       int srcComp(0), destComp(1);
       int srcNGhost(0), destNGhost(0);
       MPI_Comm commSrc(ParallelDescriptor::CommunicatorComp());
       MPI_Comm commDest(ParallelDescriptor::CommunicatorSidecar());
       MPI_Comm commInter(ParallelDescriptor::CommunicatorInter(whichSidecar));
       MPI_Comm commBoth(ParallelDescriptor::CommunicatorBoth(whichSidecar));
       bool isSrc(true);

       amrex::MultiFab::copyInter(mfSource, mfDest, srcComp, destComp, nComp,
                           srcNGhost, destNGhost,
                           commSrc, commDest, commInter, commBoth,
                           isSrc);


       ParallelDescriptor::Bcast(&time_step, 1, MPI_IntraGroup_Broadcast_Rank,
                                 ParallelDescriptor::CommunicatorInter(whichSidecar));

       int do_analysis_bcast(do_analysis);
       ParallelDescriptor::Bcast(&do_analysis_bcast, 1, MPI_IntraGroup_Broadcast_Rank,
                                 ParallelDescriptor::CommunicatorInter(whichSidecar));

#endif // if 0
     }
#endif // ifdef REEBER
}

void
Nyx::halo_merge ()
{
   Nyx::theAPC()->fillNeighbors(level);
   Nyx::theAPC()->Merge(level);
   Nyx::theAPC()->clearNeighbors(level);

   // Call Redistribute to remove any particles with id = -1 (as set inside the Merge call)
   Nyx::theAPC()->Redistribute(level, level, nghost0);
}

void
Nyx::halo_accrete (Real dt)
{
   amrex::MultiFab& new_state = get_new_data(State_Type);
   const BoxArray& simBA = new_state.boxArray();
   const DistributionMapping& simDM = new_state.DistributionMap();
   int ncomp = new_state.nComp();

   // First copy the existing state into orig_state.
   MultiFab orig_state(simBA, simDM, ncomp, nghost1);
   MultiFab::Copy(orig_state, new_state,
                  comp0, comp0, ncomp, nghost1);

   // Convert new_state to primitive variables: rho, velocity, energy/rho.
   conserved_to_primitive(new_state);

   // Create a MultiFab to hold the density we're going to remove from the grid
   MultiFab agn_density_lost(simBA, simDM, ncomp1, nghost1);
   agn_density_lost.setVal(0.0);

   cout << "Going into AccreteMass, number of AGN particles on this proc is "
        << Nyx::theAPC()->TotalNumberOfParticles(true, true) << endl;
   // AGN particles: increase mass and energy.
   // new_state: no change, other than filling in ghost cells.
   // agn_density_lost: gets filled in.
   Nyx::theAPC()->AccreteMass(level, new_state, agn_density_lost, dt);

   // Make sure the density put into ghost cells is added to valid regions
   agn_density_lost.SumBoundary(geom.periodicity());

   // Take away the density from the gas that was added to the AGN particle.
   amrex::MultiFab::Subtract(new_state, agn_density_lost,
                             comp0, Density, ncomp1, nghost0);

   // Convert new_state to conserved variables: rho, momentum, energy.
   primitive_to_conserved(new_state);

   // Re-set the particle velocity and energy after accretion,
   // using change of momentum density in state.
   // No change to state, other than filling ghost cells.
   int add_energy = 1;
   cout << "Going into ComputeParticleVelocity (and energy), number of AGN particles on this proc is "
        << Nyx::theAPC()->TotalNumberOfParticles(true, true) << endl;
   Nyx::theAPC()->ComputeParticleVelocity(level, orig_state, new_state, add_energy);
   // Now new_state = get_new_data(State_Type) has been updated.
}
#endif // AGN
