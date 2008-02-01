#define SAVEBIN

#ifdef SAVEBIN
    #define SAVE saveBin
#else
    #define SAVE saveTxt
#endif

#include <fstream>
#include <cstring>

#include "mesh3.h"
#include "integrator.h"
#include "fcontainer.h"
#include "cpuChrono.h"
#include "assemble.h"
#include "sensors.h"

using namespace std;

int GaussOrder=3;
//int GaussOrder=0;

void getOutputFilepath(char* ref_filepath, char* output_filename, char* path);
void getHelp(char** argv);

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        cerr << "Not enough arguments \nPlease try \"" << argv[0] << " -h\" or \"" << argv[0] << " --help \" \n" << endl;
        return 0;
    }

    if ((!strcmp(argv[1],"-h")) | (!strcmp(argv[1],"--help"))) getHelp(argv);

    disp_argv(argc,argv);

    // Start Chrono
    cpuChrono C;
    C.start();

    /*********************************************************************************************
    * Computation of LHS for BEM Symmetric formulation
    **********************************************************************************************/
    if (!strcmp(argv[1],"-LHS")) {
        if (argc < 3)
        {
            std::cerr << "Please set geometry filepath !" << endl;
            exit(1);
        }
        if (argc < 4)
        {
            std::cerr << "Please set conductivities filepath !" << endl;
            exit(1);
        }
	    if (argc < 5)
        {
            std::cerr << "Please set output filepath !" << endl;
            exit(1);
        }
        // Loading surfaces from geometry file
        Geometry geo;
        geo.read(argv[2],argv[3]);

        // Assembling matrix from discretization :
        LHS_matrice lhs(geo,GaussOrder);
        lhs.SAVE(argv[4]);
    }

    /*********************************************************************************************
    * Computation of general RHS from BEM Symmetric formulation
    **********************************************************************************************/
    else if (!strcmp(argv[1],"-RHS")) {

        if (argc < 3)
        {
            std::cerr << "Please set geometry filepath !" << endl;
            exit(1);
        }
        if (argc < 4)
        {
            std::cerr << "Please set conductivities filepath !" << endl;
            exit(1);
        }
        if (argc < 5)
        {
            std::cerr << "Please set 'mesh of sources' filepath !" << endl;
            exit(1);
        }

        // Loading surfaces from geometry file.
        Geometry geo;
        geo.read(argv[2],argv[3]);

        // Loading mesh for distributed sources
        Mesh mesh_sources;
        bool checkClosedSurface = false;
        mesh_sources.load(argv[4],false); // Load mesh without crashing when the surface is not closed

        // Assembling matrix from discretization :
        RHS_matrice mat(geo,mesh_sources,GaussOrder);
        mat.SAVE(argv[5]); // if outfile is specified
    }

    /*********************************************************************************************
    * Computation of RHS for discrete dipolar case
    **********************************************************************************************/
    else if(!strcmp(argv[1],"-rhsPOINT")) {

        if(argc < 3)
        {
            cerr << "Please set geometry filepath !" << endl;
            exit(1);
        }
        if (argc < 4)
        {
            std::cerr << "Please set conductivities filepath !" << endl;
            exit(1);
        }
        if(argc < 5)
        {
            cerr << "Please set dipoles filepath!" << endl;
            exit(1);
        }

        // Loading surfaces from geometry file.
        Geometry geo;
        geo.read(argv[2],argv[3]);

        // Loading matrix of dipoles :
        matrice dipoles(argv[4]);
        if(dipoles.ncol()!=6)
        {
            cerr << "Dipoles File Format Error" << endl;
            exit(1);
        }

        // Assembling matrix from discretization :
        unsigned int nd = (unsigned int) dipoles.nlin();
        std::vector<Vect3> Rs,Qs;
        for( unsigned int i=0; i<nd; i++ )
        {
            Vect3 r(3),q(3);
            for(int j=0;j<3;j++) r(j)   = dipoles(i,j);
            for(int j=3;j<6;j++) q(j-3) = dipoles(i,j);
            Rs.push_back(r); Qs.push_back(q);
        }

        RHSdip_matrice mat(geo, Rs, Qs, GaussOrder);
        // Saving RHS matrix for dipolar case :
        mat.SAVE(argv[5]);
    }

    /*********************************************************************************************
    * Computation of RHS for EIT
    **********************************************************************************************/


    else if(!strcmp(argv[1],"-EITsource")) {

        if(argc < 3)
        {
            cerr << "Please set geometry filepath !" << endl;
            exit(1);
        }
        if (argc < 4)
        {
            std::cerr << "Please set conductivities filepath !" << endl;
            exit(1);
        }
        if (argc < 5)
        {
            std::cerr << "Please set output filepath !" << endl;
            exit(1);
        }
        if (argc < 6)
        {
            std::cerr << "Please set output filepath !" << endl;
            exit(1);
        }

        // Loading surfaces from geometry file.
        Geometry geo;
        geo.read(argv[2],argv[3]);

	int taille=geo.size();
        int sourcetaille = (geo.getM(geo.nb()-1)).nbTrgs();
	int newtaille=taille-sourcetaille;
	
	matrice source(newtaille,sourcetaille);
        matrice airescalp(newtaille,sourcetaille);
        source.set(0.0);
        airescalp.set(0.0);

	assemble_EITsource( geo, source, airescalp, GaussOrder);

        source.SAVE(argv[4]);
        airescalp.SAVE(argv[5]);
    }

	/*********************************************************************************************
    * RK: Computation of RHS for discrete dipolar case: gradient wrt dipoles position and intensity!
    **********************************************************************************************/
    else if(!strcmp(argv[1],"-rhsPOINTgrad")) {

        if(argc < 3)
        {
            cerr << "Please set geometry filepath !" << endl;
            exit(1);
        }
        if (argc < 4)
        {
            std::cerr << "Please set conductivities filepath !" << endl;
            exit(1);
        }
        if(argc < 5)
        {
            cerr << "Please set dipoles filepath!" << endl;
            exit(1);
        }

        // Loading surfaces from geometry file.
        Geometry geo;
        geo.read(argv[2],argv[3]);

        // Loading matrix of dipoles :
        matrice dipoles(argv[4]);
        if(dipoles.ncol()!=6)
        {
            cerr << "Dipoles File Format Error" << endl;
            exit(1);
        }

        // Assembling matrix from discretization :
        unsigned int nd = (unsigned int) dipoles.nlin();
        std::vector<Vect3> Rs,Qs;
        for( unsigned int i=0; i<nd; i++ )
        {
            Vect3 r(3),q(3);
            for(int j=0;j<3;j++) r(j)   = dipoles(i,j);
            for(int j=3;j<6;j++) q(j-3) = dipoles(i,j);
            Rs.push_back(r); Qs.push_back(q);
        }

        RHSdip_grad_matrice mat( geo, Rs, Qs, GaussOrder);
        // Saving RHS matrix for dipolar case :
        mat.SAVE(argv[5]);
    }

    /*********************************************************************************************
    * Computation of the linear application which maps x (the unknown vector in symmetric system)
    * |----> v (potential at the electrodes)
    **********************************************************************************************/
    else if(!strcmp(argv[1],"-vToEEG")) {

        if(argc < 3)
        {
            cerr << "Please set geometry filepath !" << endl;
            exit(1);
        }
        if (argc < 4)
        {
            std::cerr << "Please set conductivities filepath !" << endl;
            exit(1);
        }
        if(argc < 5)
        {
            cerr << "Please set patches filepath !" << endl;
            exit(1);
        }

        // Loading surfaces from geometry file.
        Geometry geo;
        geo.read(argv[2],argv[3]);

        // read the file containing the positions of the EEG patches
        matrice patches(argv[4]);

        // Assembling matrix from discretization :
        // vToEEG is the linear application which maps x |----> v
        vToEEG_matrice mat(geo,patches);
        // Saving vToEEG matrix :
        mat.SAVE(argv[5]);
    }

    /*********************************************************************************************
    * Computation of the linear application which maps x (the unknown vector in symmetric system)
    * |----> bFerguson (contrib to MEG response)
    **********************************************************************************************/
    else if(!strcmp(argv[1],"-vToMEG")) {

        if(argc < 3)
        {
            cerr << "Please set geometry filepath !" << endl;
            exit(1);
        }
        if (argc < 4)
        {
            std::cerr << "Please set conductivities filepath !" << endl;
            exit(1);
        }
        if(argc < 5)
        {
            cerr << "Please set squids filepath !" << endl;
            exit(1);
        }

        // Loading surfaces from geometry file.
        Geometry geo;
        geo.read(argv[2],argv[3]);

        // Load positions and orientations of sensors  :
        Sensors sensors(argv[4]);

        // Assembling matrix from discretization :
        vToMEG_matrice mat(geo,sensors);
        // Saving xToMEGrespCont matrix :
        mat.SAVE(argv[5]); // if outfile is specified
    }

    /*********************************************************************************************
    * Computation of the linear application which maps x (the unknown vector in symmetric system)
    * |----> binf (contrib to MEG response)
    **********************************************************************************************/
    else if(!strcmp(argv[1],"-sToMEG")) {

        if(argc < 3)
        {
            cerr << "Please set 'mesh sources' filepath !" << endl;
            exit(1);
        }
        if(argc < 4)
        {
            cerr << "Please set squids filepath !" << endl;
            exit(1);
        }

        // Loading mesh for distributed sources :
        Mesh mesh_sources;
        bool checkClosedSurface = false;
        mesh_sources.load(argv[2],false); // Load mesh without crashing when the surface is not closed

        // Load positions and orientations of sensors  :
        Sensors sensors(argv[3]);

        // Assembling matrix from discretization :
        sToMEG_matrice mat(mesh_sources, sensors);
        // Saving sToMEG matrix :
        mat.SAVE(argv[4]);
    }

    /*********************************************************************************************
    * Computation of the discrete linear application which maps x (the unknown vector in a symmetric system)
    * |----> binf (contrib to MEG response)
    **********************************************************************************************/
    // arguments are the positions and orientations of the squids,
    // the position and orientations of the sources and the output name.

    else if(!strcmp(argv[1],"-sToMEG_point")) {

        if (argc < 3)
        {
            cerr << "Please set dipoles filepath !" << endl;
            exit(1);
        }
        if (argc < 4)
        {
            cerr << "Please set squids filepath !" << endl;
            exit(1);
        }

        // Loading dipoles :
        matrice dipoles(argv[2]);

        // Load positions and orientations of sensors  :
        Sensors sensors(argv[3]);

        sToMEGdip_matrice mat( dipoles, sensors );
        mat.SAVE(argv[4]);
    }
    else cerr << "unknown argument: " << argv[1] << endl;

    // Stop Chrono
    C.stop();
    C.dispEllapsed();
}

void getOutputFilepath(char* ref_filepath, char* output_filename, char* path) {
    assert(path!=ref_filepath && path!=output_filename);
    // output filename on the same path as filename referenced in ref_filepath
    // go in search on all platform of path less filename included in ref_filepath.
    char* p = strrchr(ref_filepath, '/' );
    if (p == NULL)
        strcpy(path,output_filename);
    else
    {
        strncpy(path,ref_filepath,p-ref_filepath+1);
        strcat(path,output_filename);
    }
}

void getHelp(char** argv) {
    cout << argv[0] <<" [-option] [filepaths...]" << endl << endl;

    cout << "option :" << endl;
    cout << "   -LHS :   Compute LHS from BEM symmetric formulation." << endl;
    cout << "            Arguments :" << endl;
    cout << "               geometry file (.geom)" << endl;
    cout << "               conductivity file (.cond)" << endl;
    cout << "               output LHS matrix" << endl << endl;

    cout << "   -RHS :   Compute RHS from BEM symmetric formulation. " << endl;
    cout << "            Arguments :" << endl;
    cout << "               geometry file (.geom)" << endl;
    cout << "               conductivity file (.cond)" << endl;
    cout << "               mesh of sources (.tri .vtk .mesh .bnd)" << endl;
    cout << "               output RHS matrix" << endl << endl;

    cout << "   -rhsPOINT :   Compute RHS for discrete dipolar case. " << endl;
    cout << "            Arguments :" << endl;
    cout << "               geometry file (.geom)" << endl;
    cout << "               conductivity file (.cond)" << endl;
    cout << "               dipoles positions and orientations" << endl;
    cout << "               output RHS matrix" << endl << endl;

    cout << "   -EITsource :  Compute RHS for scalp current injection. " << endl;
    cout << "            Arguments :" << endl;
    cout << "               geometry file (.geom)" << endl;
    cout << "               conductivity file (.cond)" << endl;
    cout << "               output EITsource" << endl;
    cout << "               output airescalp" << endl << endl;

    cout << "   -vToEEG :   Compute the linear application which maps the potiential" << endl;
    cout << "            on the scalp to the EEG electrodes"  << endl;
    cout << "            Arguments :" << endl;
    cout << "               geometry file (.geom)" << endl;
    cout << "               conductivity file (.cond)" << endl;
    cout << "               file containing the positions of EEG patches (.patches)" << endl;
    cout << "               output vToEEG matrice" << endl << endl;

    cout << "   -vToMEG :   Compute the linear application which maps the potential" << endl;
    cout << "            on the scalp to the MEG sensors"  << endl;
    cout << "            Arguments :" << endl;
    cout << "               geometry file (.geom)" << endl;
    cout << "               conductivity file (.cond)" << endl;
    cout << "               file containing the positions and orientations of the MEG sensors (.squids)" << endl;
    cout << "               output xToMEG matrix" << endl << endl;

    cout << "   -sToMEG :   Compute the linear application which maps the current" << endl;
    cout << "            dipoles on the source mesh to the MEG sensors" << endl;
    cout << "            Arguments :" << endl;
    cout << "               mesh file for distributed sources (.tri .vtk .mesh .bnd)" << endl;
    cout << "               positions and orientations of the MEG sensors (.squids)" << endl;
    cout << "               output sToMEG matrix" << endl << endl;

    cout << "   -sToMEG_point :   Compute the linear application which maps the current" << endl;
    cout << "            dipoles to the MEG sensors" << endl;
    cout << "            Arguments :" << endl;
    cout << "               dipoles positions and orientations" << endl;
    cout << "               positions and orientations of the MEG sensors (.squids)" << endl;
    cout << "               name of the output sToMEG matrix" << endl << endl;

    exit(0);
}

