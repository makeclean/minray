#include "minray.h"

// Prints Section titles in center of 80 char terminal
void center_print(const char *s, int width)
{
  int length = strlen(s);
  int i;
  for (i=0; i<=(width-length)/2; i++) {
    fputs(" ", stdout);
  }
  fputs(s, stdout);
  fputs("\n", stdout);
}

// Prints a border
void border_print(void)
{
  printf(
      "==================================================================="
      "=============\n");
}

// Prints comma separated integers - for ease of reading
void fancy_int( int a )
{
  if( a < 1000 )
    printf("%d\n",a);

  else if( a >= 1000 && a < 1000000 )
    printf("%d,%03d\n", a / 1000, a % 1000);

  else if( a >= 1000000 && a < 1000000000 )
    printf("%d,%03d,%03d\n", a / 1000000, (a % 1000000) / 1000, a % 1000 );

  else if( a >= 1000000000 )
    printf("%d,%03d,%03d,%03d\n",
        a / 1000000000,
        (a % 1000000000) / 1000000,
        (a % 1000000) / 1000,
        a % 1000 );
  else
    printf("%d\n",a);
}
  
// Prints program logo
void logo(void)
{
  border_print();
  printf(
         "        .___  ___.  __  .__   __. .______        ___   ____    ____ \n"
         "        |   \\/   | |  | |  \\ |  | |   _  \\      /   \\  \\   \\  /   / \n"
         "        |  \\  /  | |  | |   \\|  | |  |_)  |    /  ^  \\  \\   \\/   /  \n"
         "        |  |\\/|  | |  | |  . `  | |      /    /  /_\\  \\  \\_    _/   \n"
         "        |  |  |  | |  | |  |\\   | |  |\\  \\   /  _____  \\   |  |    \n"
         "        |__|  |__| |__| |__| \\__| |  | \\  \\ /__/     \\__\\  |__|    \\\n"
         "                                  |  |  \\  \\                        \\ \n"
         "                                  |__|   \\__\\________________________\\\n"
         "                                                                     /\n"
         "                                                                    /\n"
         "                                                                   /\n\n"
);
  border_print();
  printf("\n");
  //center_print("Developed at", 79);
  //center_print("Argonne National Laboratory", 79);
  char v[100];
  sprintf(v, "Version: %s", VERSION);
  center_print(v, 79);
  printf("\n");
  border_print();
}

void print_user_inputs(Parameters P)
{
  logo();
  center_print("INPUT SUMMARY", 79);
  border_print();
  printf("Number of Cells per Dimension     = %d\n",    P.n_cells_per_dimension);
  printf("Total Number of Cells (FSRs)      = %lu\n",   P.n_cells);
  printf("Number of Rays per Iteration      = %lu\n",   P.n_rays);
  printf("Length of each ray [cm]           = %.2lf\n", P.distance_per_ray);
  printf("Energy Groups                     = %d\n",    P.n_energy_groups);
  printf("Number of Inactive Iterations     = %d\n",    P.n_inactive_iterations);
  printf("Number of Active Iterations       = %d\n",    P.n_active_iterations);
  printf("Pseudorandom Seed                 = %lu\n",   P.seed);
  printf("Maximum Intersections per Ray     = %d\n",    P.max_intersections_per_ray);
  size_t bytes = estimate_memory_usage(P);
  double MB = (double) bytes / 1024.0 /1024.0;
  printf("Estimated Memory Usage            = %.2lf [MB]\n", MB);
  if( P.plotting_enabled )
    printf("Plotting                          = Enabled\n");
  else
    printf("Plotting                          = Disabled\n");
  
  char * validation_strings[3] = {"small", "medium", "large"};
  if( P.validation_problem_id )
    printf("Validation problem                = %s\n", validation_strings[P.validation_problem_id-1]);

  #ifdef OPENMP
  printf("Number of Threads                 = %d\n", omp_get_max_threads());
  #endif
}

int print_results(Parameters P, SimulationResult SR)
{
  border_print();
  center_print("RESULTS", 79);
  border_print();
  printf("k-effective                       = %.5f\n", SR.k_eff);
  printf("k-effective std. dev.             = %.5f\n", SR.k_eff_std_dev);
  printf("Simulation Runtime                = %.3le [s]\n", SR.runtime_total);
  printf("    Transport Sweep Time          = %.3le [s] (%.2lf%%)\n", SR.runtime_transport_sweep, 100.0 * SR.runtime_transport_sweep / SR.runtime_total);
  printf("    Iteration Time                = %.3le [s] (%.2lf%%)\n", SR.runtime_total - SR.runtime_transport_sweep, 100.0* (1.0 - SR.runtime_transport_sweep / SR.runtime_total));
  printf("Number of Geometric Intersections = %.3le\n", (double) SR.n_geometric_intersections);
  printf("Avg. Geom. Intersections per Ray  = %.1lf\n", SR.n_geometric_intersections / ((double)P.n_rays * P.n_iterations));
  printf("Number of Integrations            = %.3le\n", (double) SR.n_geometric_intersections * P.n_energy_groups);
  double time_per_integration = SR.runtime_total * 1.0e9 / ( SR.n_geometric_intersections * P.n_energy_groups);
  printf("Time per Integration (TPI)        = %.3lf [ns]\n", time_per_integration);
  printf("Est. Total Time Req. to Converge  = %.3le [s]\n", (SR.runtime_total / P.n_iterations) * 2000.0);
  int is_valid_result = validate_results(P.validation_problem_id, SR.k_eff);
  border_print();
  return is_valid_result;
}

void print_status_data(int iter, double k_eff, double percent_missed, int is_active_region, double k_eff_total_accumulator, double k_eff_sum_of_squares_accumulator, int n_active_iterations)
{
  char color[64] = "";
  char color_reset[64] = "";
  if( percent_missed > 0.01 )
  {
    sprintf(color,       "\033[0;31m");
    sprintf(color_reset, "\033[0m");
  }
  else
  {
    //sprintf(color,       "");
   // sprintf(color_reset, "");
  }

  double k_eff_avg, k_eff_std_dev;
  compute_statistics(k_eff_total_accumulator, k_eff_sum_of_squares_accumulator, n_active_iterations, &k_eff_avg, &k_eff_std_dev);

  char active_info[256];
  if(is_active_region)
  {
    sprintf(active_info, "k-avg = %.5lf +/- %.5lf", k_eff_avg, k_eff_std_dev);
  }
  else
    sprintf(active_info, "Inactive");

  // Print status data
  printf("Iter %5d   k = %.5lf   %sMiss Rate = %.2le%s   %s\n", iter, k_eff, color, percent_missed / 100.0, color_reset, active_info);
}

// print error to screen, inform program options
void print_CLI_error(void)
{
  printf("Usage: ./minray <options>\n");
  printf("Options:\n");
  printf("    -r <rays>                    Number of discrete rays\n");
  printf("    -d <distance per ray>        Travel distance per ray (cm)\n");
  printf("    -i <inactive iterations>     Set fixed number of inactive power iterations\n");
  printf("    -a <active iterations>       Set fixed number of active power iterations\n");
  printf("    -s <seed>                    Random number generator seed (for reproducibility)\n");
  printf("    -m <problem size multiplier> Multiplioer to increase problem size/resolution\n");
  printf("    -p                           Enables plotting\n");
  printf("    -v <small, medium, large>    Executes a specific validation probem to test for correctness\n");

  printf("See readme for full description of default run values\n");
  exit(1);
}

Parameters read_CLI(int argc, char * argv[])
{
  int problem_size_multiplier = 16;

  // Set Defaults
  Parameters P;
  P.length_per_dimension = 64.26;
  P.distance_per_ray = 10.0;
  P.n_inactive_iterations = 10;
  P.n_active_iterations = 10;
  P.seed = time(NULL);
  P.n_materials = 8;
  P.n_energy_groups = 7;
  P.plotting_enabled = 0;
  P.validation_problem_id = NONE;

  P.boundary_conditions[1][1] = NONE;
  P.boundary_conditions[1][2] = REFLECTIVE; // x+
  P.boundary_conditions[1][0] = VACUUM;     // x-
  P.boundary_conditions[2][1] = VACUUM;     // y+
  P.boundary_conditions[0][1] = REFLECTIVE; // y-

  int has_user_set_rays = 0;

  // Collect Raw Input
  for( int i = 1; i < argc; i++ )
  {
    char * arg = argv[i];

    // rays (-r)
    if( strcmp(arg, "-r") == 0 )
    {
      if( ++i < argc )
      {
        P.n_rays = atoi(argv[i]);
        has_user_set_rays = 1;
      }
      else
        print_CLI_error();
    }
    // seed (-s)
    else if( strcmp(arg, "-s") == 0 )
    {
      if( ++i < argc )
        P.seed = atoi(argv[i]);
      else
        print_CLI_error();
    }
    // problem size multiplier (-m)
    else if( strcmp(arg, "-m") == 0 )
    {
      if( ++i < argc )
        problem_size_multiplier = atoi(argv[i]);
      else
        print_CLI_error();
    }
    // plotting enabled
    else if( strcmp(arg, "-p") == 0 )
    {
      P.plotting_enabled = 1;
    }
    // distance per ray (-d)
    else if( strcmp(arg, "-d") == 0 )
    {
      if( ++i < argc )
        P.distance_per_ray = atof(argv[i]);
      else
        print_CLI_error();
    }
    // inactive iterations (-i)
    else if( strcmp(arg, "-i") == 0 )
    {
      if( ++i < argc )
        P.n_inactive_iterations = atoi(argv[i]);
      else
        print_CLI_error();
    }
    // active iterations (-a)
    else if( strcmp(arg, "-a") == 0 )
    {
      if( ++i < argc )
        P.n_active_iterations = atoi(argv[i]);
      else
        print_CLI_error();
    }
    // validation problem selection
    else if( strcmp(arg, "-v") == 0 )
    {
      char * size;
      if( ++i < argc )
        size = argv[i];
      else
        print_CLI_error();

      if( strcmp(size, "small") == 0 )
      {
        P.validation_problem_id = SMALL;
      }
      else if( strcmp(size, "medium") == 0 )
      {
        P.validation_problem_id = MEDIUM;
      }
      else if( strcmp(size, "large") == 0 )
      {
        P.validation_problem_id = LARGE;
      }
      else
        print_CLI_error();
    }
    else
      print_CLI_error();
  }

  if( P.validation_problem_id == SMALL )
  {
    problem_size_multiplier = 1;
    P.n_inactive_iterations = 10;
    P.n_active_iterations = 10;
    P.seed = 42;
  }
  else if( P.validation_problem_id == MEDIUM )
  {
    problem_size_multiplier = 4;
    P.n_inactive_iterations = 100;
    P.n_active_iterations = 100;
    P.seed = 2001;
  }
  else if( P.validation_problem_id == LARGE )
  {
    problem_size_multiplier = 16;
    P.n_inactive_iterations = 1000;
    P.n_active_iterations = 1000;
    P.seed = 123456789;
  }

  // Derived Values
  P.n_cells_per_dimension = 102 * problem_size_multiplier;
  P.max_intersections_per_ray = 30 * problem_size_multiplier;
  if( !has_user_set_rays)
    P.n_rays = 6170.0 * problem_size_multiplier + 1955.0;
  P.cell_width = P.length_per_dimension / P.n_cells_per_dimension;
  P.inverse_cell_width = 1.0 / P.cell_width;
  P.n_cells = P.n_cells_per_dimension * P.n_cells_per_dimension;
  P.cell_expected_track_length = (P.distance_per_ray * P.n_rays) / P.n_cells;
  P.inverse_total_track_length = 1.0 / (P.distance_per_ray * P.n_rays);
  P.inverse_length_per_dimension = 1.0 / P.length_per_dimension;
  P.n_iterations = P.n_inactive_iterations + P.n_active_iterations;
  P.cell_volume = 1.0 / P.n_cells;

  return P;
}

// KEY
// 0 - UO2
// 1 - MOX 4.3
// 2 - MOX 7.0
// 3 - MOX 8.7
// 4 - Fission Chamber
// 5 - Guide Tube
// 6 - Moderator 
// 7 - Control Rod
ReadOnlyData load_2D_C5G7_XS(Parameters P)
{
  size_t sz = P.n_materials * P.n_energy_groups;

  // Allocate Data on Host
  float * nu_Sigma_f  = (float *) calloc(sz, sizeof(float));
  float * Sigma_f     = (float *) calloc(sz, sizeof(float));
  float * Sigma_t     = (float *) calloc(sz, sizeof(float));
  float * Chi         = (float *) calloc(sz, sizeof(float));

  sz = sz * P.n_energy_groups;
  float * Sigma_s =     (float *) calloc(sz, sizeof(float));

  // UO2 fuel-clad mixture
  FILE * UO2_scatter   = fopen("../data/C5G7_2D/UO2_scatter.txt",   "r");
  FILE * UO2_transport = fopen("../data/C5G7_2D/UO2_transport.txt", "r");
  FILE * UO2_nufission = fopen("../data/C5G7_2D/UO2_nufission.txt", "r");
  FILE * UO2_fission   = fopen("../data/C5G7_2D/UO2_fission.txt", "r");
  FILE * UO2_chi       = fopen("../data/C5G7_2D/UO2_chi.txt", "r");

  // MOX 4.3% fuel-clad mixture
  FILE * MOX_43_scatter   = fopen("../data/C5G7_2D/MOX_43_scatter.txt",   "r");
  FILE * MOX_43_transport = fopen("../data/C5G7_2D/MOX_43_transport.txt", "r");
  FILE * MOX_43_nufission = fopen("../data/C5G7_2D/MOX_43_nufission.txt", "r");
  FILE * MOX_43_fission   = fopen("../data/C5G7_2D/MOX_43_fission.txt", "r");
  FILE * MOX_43_chi       = fopen("../data/C5G7_2D/MOX_43_chi.txt", "r");

  // MOX 7.0% fuel-clad mixture
  FILE * MOX_70_scatter   = fopen("../data/C5G7_2D/MOX_70_scatter.txt",   "r");
  FILE * MOX_70_transport = fopen("../data/C5G7_2D/MOX_70_transport.txt", "r");
  FILE * MOX_70_nufission = fopen("../data/C5G7_2D/MOX_70_nufission.txt", "r");
  FILE * MOX_70_fission   = fopen("../data/C5G7_2D/MOX_70_fission.txt", "r");
  FILE * MOX_70_chi       = fopen("../data/C5G7_2D/MOX_70_chi.txt", "r");

  // MOX 8.7% fuel-clad mixture
  FILE * MOX_87_scatter   = fopen("../data/C5G7_2D/MOX_87_scatter.txt",   "r");
  FILE * MOX_87_transport = fopen("../data/C5G7_2D/MOX_87_transport.txt", "r");
  FILE * MOX_87_nufission = fopen("../data/C5G7_2D/MOX_87_nufission.txt", "r");
  FILE * MOX_87_fission   = fopen("../data/C5G7_2D/MOX_87_fission.txt", "r");
  FILE * MOX_87_chi       = fopen("../data/C5G7_2D/MOX_87_chi.txt", "r");

  // Fission Chamber
  FILE * FC_scatter   = fopen("../data/C5G7_2D/FC_scatter.txt",   "r");
  FILE * FC_transport = fopen("../data/C5G7_2D/FC_transport.txt", "r");
  FILE * FC_nufission = fopen("../data/C5G7_2D/FC_nufission.txt", "r");
  FILE * FC_fission   = fopen("../data/C5G7_2D/FC_fission.txt", "r");
  FILE * FC_chi       = fopen("../data/C5G7_2D/FC_chi.txt", "r");

  // Guide Tube
  FILE * GT_scatter   = fopen("../data/C5G7_2D/GT_scatter.txt",   "r");
  FILE * GT_transport = fopen("../data/C5G7_2D/GT_transport.txt", "r");

  // Moderator
  FILE * Mod_scatter   = fopen("../data/C5G7_2D/Mod_scatter.txt",   "r");
  FILE * Mod_transport = fopen("../data/C5G7_2D/Mod_transport.txt", "r");

  // Control Rod
  FILE * CR_scatter   = fopen("../data/C5G7_2D/CR_scatter.txt",   "r");
  FILE * CR_transport = fopen("../data/C5G7_2D/CR_transport.txt", "r");
  
  if( CR_transport == NULL )
  {
    printf("Cross section data files not found at ../data/C5G7_2D/\n");
    exit(1);
  }

  int ret = 0;

  for( int i = 0; i < P.n_energy_groups; i++ )
  {
    // Transport XS
    ret = fscanf(UO2_transport,    "%f", Sigma_t + 0 * P.n_energy_groups + i);
    ret = fscanf(MOX_43_transport, "%f", Sigma_t + 1 * P.n_energy_groups + i);
    ret = fscanf(MOX_70_transport, "%f", Sigma_t + 2 * P.n_energy_groups + i);
    ret = fscanf(MOX_87_transport, "%f", Sigma_t + 3 * P.n_energy_groups + i);
    ret = fscanf(FC_transport,     "%f", Sigma_t + 4 * P.n_energy_groups + i);
    ret = fscanf(GT_transport,     "%f", Sigma_t + 5 * P.n_energy_groups + i);
    ret = fscanf(Mod_transport,    "%f", Sigma_t + 6 * P.n_energy_groups + i);
    ret = fscanf(CR_transport,     "%f", Sigma_t + 7 * P.n_energy_groups + i);

    // Nu-Fission XS
    ret = fscanf(UO2_nufission,    "%f", nu_Sigma_f + 0 * P.n_energy_groups + i);
    ret = fscanf(MOX_43_nufission, "%f", nu_Sigma_f + 1 * P.n_energy_groups + i);
    ret = fscanf(MOX_70_nufission, "%f", nu_Sigma_f + 2 * P.n_energy_groups + i);
    ret = fscanf(MOX_87_nufission, "%f", nu_Sigma_f + 3 * P.n_energy_groups + i);
    ret = fscanf(FC_nufission,     "%f", nu_Sigma_f + 4 * P.n_energy_groups + i);

    // Fission XS
    ret = fscanf(UO2_fission,    "%f", Sigma_f + 0 * P.n_energy_groups + i);
    ret = fscanf(MOX_43_fission, "%f", Sigma_f + 1 * P.n_energy_groups + i);
    ret = fscanf(MOX_70_fission, "%f", Sigma_f + 2 * P.n_energy_groups + i);
    ret = fscanf(MOX_87_fission, "%f", Sigma_f + 3 * P.n_energy_groups + i);
    ret = fscanf(FC_fission,     "%f", Sigma_f + 4 * P.n_energy_groups + i);

    // Chi
    ret = fscanf(UO2_chi,    "%f", Chi + 0 * P.n_energy_groups + i);
    ret = fscanf(MOX_43_chi, "%f", Chi + 1 * P.n_energy_groups + i);
    ret = fscanf(MOX_70_chi, "%f", Chi + 2 * P.n_energy_groups + i);
    ret = fscanf(MOX_87_chi, "%f", Chi + 3 * P.n_energy_groups + i);
    ret = fscanf(FC_chi,     "%f", Chi + 4 * P.n_energy_groups + i);

    for( int j = 0; j < P.n_energy_groups; j++ )
    {
      // Scatter XS
      ret = fscanf(UO2_scatter,    "%f", Sigma_s + 0 * P.n_energy_groups * P.n_energy_groups + i * P.n_energy_groups + j);
      ret = fscanf(MOX_43_scatter, "%f", Sigma_s + 1 * P.n_energy_groups * P.n_energy_groups + i * P.n_energy_groups + j);
      ret = fscanf(MOX_70_scatter, "%f", Sigma_s + 2 * P.n_energy_groups * P.n_energy_groups + i * P.n_energy_groups + j);
      ret = fscanf(MOX_87_scatter, "%f", Sigma_s + 3 * P.n_energy_groups * P.n_energy_groups + i * P.n_energy_groups + j);
      ret = fscanf(FC_scatter,     "%f", Sigma_s + 4 * P.n_energy_groups * P.n_energy_groups + i * P.n_energy_groups + j);
      ret = fscanf(GT_scatter,     "%f", Sigma_s + 5 * P.n_energy_groups * P.n_energy_groups + i * P.n_energy_groups + j);
      ret = fscanf(Mod_scatter,    "%f", Sigma_s + 6 * P.n_energy_groups * P.n_energy_groups + i * P.n_energy_groups + j);
      ret = fscanf(CR_scatter,     "%f", Sigma_s + 7 * P.n_energy_groups * P.n_energy_groups + i * P.n_energy_groups + j);
    }
  }

  // Close all Files
  fclose(UO2_scatter      );
  fclose(UO2_transport    );
  fclose(UO2_nufission    );
  fclose(UO2_fission    );
  fclose(UO2_chi          );
  fclose(MOX_43_scatter   );
  fclose(MOX_43_transport );
  fclose(MOX_43_nufission );
  fclose(MOX_43_fission );
  fclose(MOX_43_chi       );
  fclose(MOX_70_scatter   );
  fclose(MOX_70_transport );
  fclose(MOX_70_nufission );
  fclose(MOX_70_fission );
  fclose(MOX_70_chi       );
  fclose(MOX_87_scatter   );
  fclose(MOX_87_transport );
  fclose(MOX_87_nufission );
  fclose(MOX_87_fission );
  fclose(MOX_87_chi       );
  fclose(FC_scatter       );
  fclose(FC_transport     );
  fclose(FC_nufission     );
  fclose(FC_fission     );
  fclose(FC_chi           );
  fclose(GT_scatter       );
  fclose(GT_transport     );
  fclose(Mod_scatter      );
  fclose(Mod_transport    );
  fclose(CR_scatter      );
  fclose(CR_transport    );

  char fname[512];
  sprintf(fname, "../data/C5G7_2D/material_ids_%d.txt", P.n_cells_per_dimension);
  printf("Searching for material data file \"%s\"...\n", fname); 
  FILE * material_file = fopen(fname, "r");
  if( material_file == NULL )
  {
    printf("Material data file not found for dimension %d. Generate new data file with ARRC,\nor use a dimension or multipiler with existing data file.\nCurrently supported multipliers \"-m <1, 2, 4, 8, 16, 32>\"\n", P.n_cells_per_dimension);
    exit(1);
  }
  else
    printf("Material data file found.\n");
  sz = P.n_cells * sizeof(int);
  int * material_id = (int *) malloc(sz);
  for( int c = 0; c < P.n_cells; c++ )
  {
    ret = fscanf(material_file, "%d", material_id + c);
  }

  fclose(material_file);

  ReadOnlyData ROD;
  ROD.Sigma_f = Sigma_f;
  ROD.Sigma_t = Sigma_t;
  ROD.Sigma_s = Sigma_s;
  ROD.nu_Sigma_f = nu_Sigma_f;
  ROD.Chi = Chi;
  ROD.material_id = material_id;

  if( ret == 0 )
  {
    printf("something went wrong with XS read in...\n");
    exit(1);
  }

  return ROD;
}

float eswap_float( float f )
{    
  char * ptr = (char *) &f;

  char tmp[4];
  tmp[0] = ptr[0];
  tmp[1] = ptr[1];
  tmp[2] = ptr[2];
  tmp[3] = ptr[3];

  ptr[0] = tmp[3];
  ptr[1] = tmp[2];
  ptr[2] = tmp[1];
  ptr[3] = tmp[0];

  return f;
}

int eswap_int( int f )
{    
  char * ptr = (char *) &f;
  
  char tmp[4];
  tmp[0] = ptr[0];
  tmp[1] = ptr[1];
  tmp[2] = ptr[2];
  tmp[3] = ptr[3];

  ptr[0] = tmp[3];
  ptr[1] = tmp[2];
  ptr[2] = tmp[1];
  ptr[3] = tmp[0];

  return f;
}

void plot_3D_vtk(Parameters P, float * scalar_flux_accumulator, int * material_id)
{
  center_print("PLOT GENERATION", 79);
  border_print();
  int N = P.n_cells_per_dimension;
  int z_N = 1;

  char fname[512];
  sprintf(fname, "minray_plots_%d.vtk", N);
  FILE * fp = fopen(fname, "w");

  int plot_thermal_flux = 1;
  int plot_fast_flux = 1;
  int plot_materials = 1;

  /*
  // For Corner Zoom
  G.box.min.x = 0.0 + 17*1.26 / 2.0 - 2*1.26;
  G.box.max.x = G.box.min.x + 4 * 1.26;
  G.box.min.y = 0.0 - 17*1.26 / 2.0 - 2*1.26;
  G.box.max.y = G.box.min.y + 4 * 1.26;
  */

  double x_delta = P.cell_width;
  double y_delta = P.cell_width;
  double z_delta = P.cell_width;

  printf("Plotting 2D Data X x Y = %d x %d to file %s...\n", N, N, fname);

  fprintf(fp,"# vtk DataFile Version 2.0\n");
  fprintf(fp, "Dataset File\n");
  fprintf(fp, "BINARY\n");
  fprintf(fp, "DATASET STRUCTURED_POINTS\n");
  fprintf(fp, "DIMENSIONS %d %d %d\n", N, N, z_N);
  fprintf(fp, "ORIGIN 0 0 0\n");
  fprintf(fp, "SPACING %lf %lf %lf\n", x_delta, y_delta, z_delta);
  fprintf(fp, "POINT_DATA %d\n", N*N*z_N);

  if( plot_thermal_flux )
  {
    fprintf(fp, "SCALARS thermal_flux float\n");
    fprintf(fp, "LOOKUP_TABLE default\n");

    int cell_id = 0;
    for( int y = 0; y < P.n_cells_per_dimension; y++)
    {
      for( int x = 0; x < P.n_cells_per_dimension; x++)
      {
        float thermal_flux = scalar_flux_accumulator[cell_id * P.n_energy_groups +P.n_energy_groups - 1] / P.n_active_iterations;
        thermal_flux = eswap_float(thermal_flux);
        fwrite(&thermal_flux, sizeof(float), 1, fp);
        cell_id++;
      }
    }
  }

  if( plot_fast_flux )
  {
    fprintf(fp, "SCALARS fast_flux float\n");
    fprintf(fp, "LOOKUP_TABLE default\n");

    int cell_id = 0;
    for( int y = 0; y < P.n_cells_per_dimension; y++)
    {
      for( int x = 0; x < P.n_cells_per_dimension; x++)
      {
        float fast_flux = scalar_flux_accumulator[cell_id * P.n_energy_groups] / P.n_active_iterations;
        fast_flux = eswap_float(fast_flux);
        fwrite(&fast_flux, sizeof(float), 1, fp);
        cell_id++;
      }
    }
  }

  if( plot_materials )
  {
    fprintf(fp, "SCALARS material_type int\n");
    fprintf(fp, "LOOKUP_TABLE default\n");
    int cell_id = 0;
    for( int y = 0; y < P.n_cells_per_dimension; y++)
    {
      for( int x = 0; x < P.n_cells_per_dimension; x++)
      {
        int material = material_id[cell_id++];
        material = eswap_int(material);
        fwrite(&material, sizeof(int), 1, fp);
      }
    }
  }

  fclose(fp);
  printf("Finished plotting!\n");
  border_print();
}

void print_ray_tracing_buffer(Parameters P, SimulationData SD)
{
  IntersectionData ID = SD.readWriteData.intersectionData;
  for( int r = 0; r < P.n_rays; r++ )
  {
    printf("Ray %d had %d intersections, and is now at location [%.2lf, %.2lf] with group 0 flux %.3le\n", r, ID.n_intersections[r], SD.readWriteData.rayData.location_x[r], SD.readWriteData.rayData.location_y[r], SD.readWriteData.rayData.angular_flux[r * P.n_energy_groups]);
    for( int i = 0; i < ID.n_intersections[r]; i++ )
    {
      int idx = r * P.max_intersections_per_ray + i;
      printf("\tIntersection %d:   cell_id: %d   distance: %.2le   vac reflect: %d\n", i, ID.cell_ids[idx], ID.distances[idx], ID.did_vacuum_reflects[idx]);
    }
  }
}

void output_thermal_fluxes(Parameters P, SimulationData SD)
{
  char * fname = "thermal_fluxes.dat";
  printf("Writing thermal flux data to file: \"%s\"...\n", fname);
  FILE * fp = fopen(fname, "w");
  int cell_id = 0;
  for( int y = 0; y < P.n_cells_per_dimension; y++)
  {
    for( int x = 0; x < P.n_cells_per_dimension; x++)
    {
      fprintf(fp, "%.3le ", SD.readWriteData.cellData.scalar_flux_accumulator[cell_id++] / P.n_active_iterations);
    }
    fprintf(fp, "\n");
  }
}

void print_ray(double x, double y, double x_dir, double y_dir, int cell_id)
{
  printf("Location[%.3lf, %.3lf] Direction[%.3lf, %.3lf] Cell ID %d\n", x, y, x_dir, y_dir, cell_id);
}

