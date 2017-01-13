#include <iomanip>
#include <Nyx.H>
#include <Nyx_F.H>
#include <AMReX_Particles_F.H>

#ifdef GRAVITY
#include <Gravity.H>
#endif

namespace
{
    std::string ascii_particle_file;
    std::string binary_particle_file;
    std::string    sph_particle_file;

#ifdef AGN
    std::string agn_particle_file;
#endif

#ifdef NEUTRINO_PARTICLES
    std::string neutrino_particle_file;
#endif
}

static int  do_santa_barbara = 0;
static int  init_sb_vels     = 1;
static int  do_readin_ics    = 0;
std::string readin_ics_fname;

void
Nyx::read_init_params ()
{
    BL_PROFILE("Nyx::read_init_params()");
    ParmParse pp("nyx");

    pp.query("do_santa_barbara", do_santa_barbara);
    pp.query("init_sb_vels", init_sb_vels);

    if (do_hydro == 0 && do_santa_barbara == 1)
           BoxLib::Error("Nyx::cant have do_hydro == 0 and do_santa_barbara == 1");
    if (do_santa_barbara == 0 && init_with_sph_particles == 1)
           BoxLib::Error("Nyx::cant have do_santa_barbara == 0 and init_with_sph_particles == 1");
    if (do_santa_barbara == 0 && init_sb_vels == 1)
    {
       init_sb_vels = 0;
       if (ParallelDescriptor::IOProcessor())
           std::cout << "Nyx::setting init_sb_vels to 0 since do_santa_barbara = 0\n";
    }

    pp.query("do_readin_ics",       do_readin_ics);
    pp.query("readin_ics_fname", readin_ics_fname);
    pp.query("ascii_particle_file", ascii_particle_file);

    // Input error check
    if (do_dm_particles && !ascii_particle_file.empty() && particle_init_type != "AsciiFile")
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::particle_init_type is not AsciiFile but you specified ascii_particle_file" << std::endl;;
        BoxLib::Error();
    }

    pp.query("sph_particle_file", sph_particle_file);

    // Input error check
    if (init_with_sph_particles != 1 && !sph_particle_file.empty())
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::init_with_sph_particles is not 1 but you specified sph_particle_file" << std::endl;;
        BoxLib::Error();
    }

    // Input error check
    if (init_with_sph_particles == 1 && sph_particle_file.empty())
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::init_with_sph_particles is 1 but you did not specify sph_particle_file" << std::endl;;
        BoxLib::Error();
    }

    pp.query("binary_particle_file", binary_particle_file);

    // Input error check
    if (!binary_particle_file.empty() && (particle_init_type != "BinaryFile" &&
                                          particle_init_type != "BinaryMetaFile"))
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::particle_init_type is not BinaryFile or BinaryMetaFile but you specified binary_particle_file" << std::endl;
        BoxLib::Error();
    }

#ifdef AGN
    pp.query("agn_particle_file", agn_particle_file);
    if (!agn_particle_file.empty() && particle_init_type != "AsciiFile")
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::particle_init_type is not AsciiFile but you specified agn_particle_file" << std::endl;;
        BoxLib::Error();
    }
#endif

#ifdef NEUTRINO_PARTICLES
    pp.query("neutrino_particle_file", neutrino_particle_file);
    if (!neutrino_particle_file.empty() && particle_init_type != "AsciiFile")
    {
        if (ParallelDescriptor::IOProcessor())
            std::cerr << "ERROR::particle_init_type is not AsciiFile but you specified neutrino_particle_file" << std::endl;;
        BoxLib::Error();
    }
#endif
}

void
Nyx::initData ()
{
    BL_PROFILE("Nyx::initData()");

    // Here we initialize the grid data and the particles from a plotfile.
    if (!parent->theRestartPlotFile().empty())
    {
        init_from_plotfile();
        return;
    }

#ifndef NO_HYDRO
    // We need this because otherwise we might operate on uninitialized data.
    MultiFab&   S_new    = get_new_data(State_Type);
    S_new.setVal(0.0);
#endif

    // If you run a pure N-body simulation and Nyx segfaults here, then
    // please check Prob_3d.f90... You might set other variables then density...
    if (verbose && ParallelDescriptor::IOProcessor())
        std::cout << "Initializing the data at level " << level << '\n';

    const Real* dx = geom.CellSize();

    // Make sure dx = dy = dz -- that's all we guarantee to support
    const Real SMALL = 1.e-13;
    if ( (fabs(dx[0] - dx[1]) > SMALL) || (fabs(dx[0] - dx[2]) > SMALL) )
        BoxLib::Abort("We don't support dx != dy != dz");

#ifndef NO_HYDRO
    int         ns       = S_new.nComp();

    Real  cur_time = state[State_Type].curTime();

    if ( (do_santa_barbara == 0) && (do_readin_ics == 0) && (particle_init_type != "Cosmological") )
    {
        if (do_hydro == 1) 
        {
            MultiFab&   D_new    = get_new_data(DiagEOS_Type);
            int         nd       = D_new.nComp();
            D_new.setVal(0., Temp_comp);
            D_new.setVal(0.,   Ne_comp);

            for (MFIter mfi(S_new,true); mfi.isValid(); ++mfi)
            {
                const Box& bx = mfi.tilebox();
                RealBox gridloc = RealBox(bx, geom.CellSize(), geom.ProbLo());

                BL_FORT_PROC_CALL(CA_INITDATA, ca_initdata)
                    (level, cur_time, bx.loVect(), bx.hiVect(), 
                     ns, BL_TO_FORTRAN(S_new[mfi]), 
                     nd, BL_TO_FORTRAN(D_new[mfi]), 
                     dx, gridloc.lo(), gridloc.hi());
            }

           compute_new_temp();
           enforce_consistent_e(S_new);
        }
        else
        {
            for (MFIter mfi(S_new,true); mfi.isValid(); ++mfi)
            {
                const Box& bx = mfi.tilebox();
                RealBox gridloc = RealBox(bx, geom.CellSize(), geom.ProbLo());
    
                BL_FORT_PROC_CALL(CA_INITDATA, ca_initdata)
                    (level, cur_time, bx.loVect(), bx.hiVect(), 
                     ns, BL_TO_FORTRAN(S_new[mfi]), 
                     ns, BL_TO_FORTRAN(S_new[mfi]), 
                     dx, gridloc.lo(), gridloc.hi());
            }
        }
    }

#endif // end NO_HYDRO

#ifdef GRAVITY

    if (!do_grav)
    {
        //
        // Set these to zero so they're defined for the plotfile.
        //
        MultiFab& G_new = get_new_data(Gravity_Type);
        G_new.setVal(0);
    }
    else 
    {
        //
        // Initialize this to zero before first solve.
        //
        MultiFab& Phi_new = get_new_data(PhiGrav_Type);
        Phi_new.setVal(0.);
    }

#endif

#ifndef NO_HYDRO
    //
    // Read in initial conditions from a file.
    // By now only for fixed grid ics.
    // Layout and units have to be as in \vec U.
    //
    if (do_readin_ics)
    {
	std::string mfDirName(readin_ics_fname);

	MultiFab mf;
	mfDirName.append("/Level_0/Cell");

	VisMF::Read(mf, mfDirName.c_str());

        MultiFab& S_new = get_level(0).get_new_data(State_Type);
	
	S_new.copy(mf, 0, 0, 6);
	S_new.copy(mf, 0, FirstSpec, 1);

        if (do_hydro == 1) 
        {
            MultiFab&  D_new = get_new_data(DiagEOS_Type);
            D_new.setVal(0.);
        }

	if (ParallelDescriptor::IOProcessor())
	    std::cout << "Readin stuff...done\n";
    }
#endif

    if (level == 0)
        init_particles();

    if ( particle_init_type == "Cosmological")
        initcosmo();

    //
    // Must redistribute particles before calling `init_santa_barbara` so that
    // the particles already live on the higher level when we go to put some of
    // the mass onto the grid.
    //
    if (level > 0)
        particle_redistribute();

    //
    // With this call we define the initial data on the current level but we
    // also may need to modify the data on the level below since particles
    // previous at the coarser level may now live at the finer level and
    // distribute their mass differently.
    //
#ifndef NO_HYDRO
#ifdef GRAVITY
    if (do_santa_barbara == 1)
        init_santa_barbara(init_sb_vels);
#endif

    if (do_hydro)
    {
        // Verify that the sum of (rho X)_i = rho at every cell.
        for (MFIter mfi(S_new,true); mfi.isValid(); ++mfi)
        {
            const Box& bx = mfi.tilebox();

            BL_FORT_PROC_CALL(FORT_CHECK_INITIAL_SPECIES, fort_check_initial_species)
                (bx.loVect(), bx.hiVect(), BL_TO_FORTRAN(S_new[mfi]));
        }
    }

    //
    // Need to compute this in case we want to use overdensity for regridding.
    //
    if (level == 0) 
    {
        compute_average_density();
//      compute_level_averages();
    }
#endif

    if (verbose && ParallelDescriptor::IOProcessor())
        std::cout << "Done initializing the level " << level << " data\n";
}

void
Nyx::init_from_plotfile ()
{
    BL_PROFILE("Nyx::init_from_plotfile()");
    if (verbose && ParallelDescriptor::IOProcessor())
    {
        std::cout << " " << std::endl; 
        std::cout << "Initializing the data from " << parent->theRestartPlotFile() << std::endl;
    }

    if (parent->maxLevel() > 0)
        BoxLib::Abort("We can only restart from single-level plotfiles");

    // Make sure to read in "a" before we call ReadPlotFile since we will use a 
    //      when we construct e from T.

    bool is_checkpoint = false;

    // Now read in the time as well as the grid and particle data
    bool first = true;
    bool rhoe_infile;
    ReadPlotFile(first,parent->theRestartPlotFile(),rhoe_infile);

    // This is just a dummy value so that we can set the current time of the StateData
    Real dummy_dt = 1.e100;
    setTimeLevel(parent->cumTime(), dummy_dt, dummy_dt);

    comoving_a_post_restart(parent->theRestartPlotFile());

#ifndef NO_HYDRO
    // Sanity check
    //if (use_const_species == 0)
    //    BoxLib::Error("init_from_plotfile assumes we are using constant species");

    // Construct internal energy given density, temperature and species
    for (int lev = 0; lev <= parent->finestLevel(); ++lev)
    {
        Nyx& nyx_lev = get_level(lev);
        MultiFab& S_new = nyx_lev.get_new_data(State_Type);
        MultiFab& D_new = nyx_lev.get_new_data(DiagEOS_Type);
        int ns = S_new.nComp();
        int nd = D_new.nComp();

#ifdef _OPENMP
#pragma omp parallel
#endif
        for (MFIter mfi(S_new,true); mfi.isValid(); ++mfi)
        {
            const Box& bx = mfi.tilebox();

            if (rhoe_infile)
            {
                BL_FORT_PROC_CALL(INIT_E_FROM_RHOE, init_e_from_rhoe)
                    (BL_TO_FORTRAN(S_new[mfi]), &ns, bx.loVect(), bx.hiVect(), &old_a);
            }
            else 
            {
                BL_FORT_PROC_CALL(INIT_E_FROM_T, init_e_from_t)
                    (BL_TO_FORTRAN(S_new[mfi]), &ns,
                    BL_TO_FORTRAN(D_new[mfi]), &nd, bx.loVect(), bx.hiVect(), &old_a);
            }
        }

        // Define (rho E) given (rho e) and the momenta
        nyx_lev.enforce_consistent_e(S_new);
    }

    if (verbose && ParallelDescriptor::IOProcessor())
        std::cout << "Done initializing the grid data from " << parent->theRestartPlotFile() << std::endl;
#endif

    // Now read the particles from the plotfile
    particle_post_restart(parent->theRestartPlotFile(),is_checkpoint);

    if (verbose && ParallelDescriptor::IOProcessor())
    {
        std::cout << "Done initializing the particles from the plotfile " << std::endl;
        std::cout << " " << std::endl; 
    }
}
