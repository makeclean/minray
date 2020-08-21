#include "minray.h"

void transport_sweep(Parameters P, SimulationData SD)
{
  // Ray Trace Kernel
  for( int ray = 0; ray < P.n_rays; ray++ )
    ray_trace_kernel(P, SD, SD.readWriteData.rayData, ray);

  // Flux Attenuate Kernel
  for( int ray = 0; ray < P.n_rays; ray++ )
    for( int energy_group = 0; energy_group < P.n_energy_groups; energy_group++ )
      flux_attenuation_kernel(P, SD, ray, energy_group);
}

void print_ray_tracing_buffer(Parameters P, SimulationData SD)
{
  IntersectionData ID = SD.readWriteData.intersectionData;
  for( int r = 0; r < P.n_rays; r++ )
  {
    printf("Ray %d had %d intersections\n", r, ID.n_intersections[r]);
    for( int i = 0; i < ID.n_intersections[r]; i++ )
    {
      int idx = r * P.max_intersections_per_ray + i;
      printf("\tIntersection %d:   cell_id: %d   distance: %.2le   vac reflect: %d\n", i, ID.cell_ids[idx], ID.distances[idx], ID.did_vacuum_reflects[idx]);
    }
  }
}

void update_isotropic_sources(Parameters P, SimulationData SD, double k_eff)
{
  for( int cell = 0; cell < P.n_cells; cell++ )
  {
    for( int energy_group = 0; energy_group < P.n_energy_groups; energy_group++ )
    {
      update_isotropic_sources_kernel(P, SD, cell, energy_group, 1.0/k_eff);
    }
  }
}

void run_simulation(Parameters P, SimulationData SD)
{
  // k is the multiplication factor (or eigenvalue) we are trying to solve for.
  // The eigenvector is the scalar flux vector
  double k_eff = 1.0;

  for( int iter = 0; iter < P.n_iterations; iter++ )
  {
    // Update Source
    update_isotropic_sources(P, SD, k_eff);

    // Set flux tally to zero

    // Transport Sweep
    transport_sweep(P, SD);
    //print_ray_tracing_buffer(P, SD);

    // Normalize Scalar Flux

    // Add Source to Flux

    // Compute K-eff

    break;
  }
}

int main(int argc, char * argv[])
{
  Parameters P = read_CLI(argc, argv);
  SimulationData SD = initialize_simulation(P);
  initialize_rays(P, SD);
  initialize_fluxes(P, SD);

  run_simulation(P, SD);


  return 0;
}
