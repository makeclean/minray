#include "parameters.h"

typedef struct{
  double distance_to_surface;
  double surface_normal_x;
  double surface_normal_y;
} TraceResult;

typedef struct{
  int cell_id;
  int cartesian_cell_idx_x;
  int cartesian_cell_idx_y;
  int boundary_condition;
} CellLookup;

CellLookup find_cell_id(Parameters P, double x, double y);
TraceResult cartesian_ray_trace(double x, double y, double cell_width, int x_idx, int y_idx, double x_dir, double y_dir);

__kernel void ray_trace_kernel(ARGUMENTS)
{
  ulong ray_id = get_global_id(0);
  if( ray_id >= P.n_cells)
    return;
  double distance_travelled = 0.0;
  int intersection_id = 0;

  double x =     location_x[ ray_id];
  double y =     location_y[ ray_id];
  double x_dir = direction_x[ray_id];
  double y_dir = direction_y[ray_id];
  int ray_cell_id =  cell_id[    ray_id];
  int x_idx = ray_cell_id % P.n_cells_per_dimension;
  int y_idx = ray_cell_id / P.n_cells_per_dimension;
    
  int just_hit_vacuum = 0;
  int is_terminal = 0;

  // We run this loop until either:
  // 1) The maximum number of intersections has been reached (not typical -- would indicate an error)
  // 2) The ray has reached its set distance (typical operation)
  for( intersection_id = 0; (intersection_id < P.max_intersections_per_ray) && (distance_travelled < P.distance_per_ray); intersection_id++ )
  {
    // Perform ray trace through a Cartesian geometry
    TraceResult trace = cartesian_ray_trace(x, y, P.cell_width, x_idx, y_idx, x_dir, y_dir);

    // Check to see if ray has reached its maximum distance. Truncate if needed
    if(distance_travelled + trace.distance_to_surface >= P.distance_per_ray)
    {
      trace.distance_to_surface = (P.distance_per_ray - distance_travelled) + BUMP;
      is_terminal = 1;
    }

    // Record intersection information for use by flux attenuation kernel
    ulong global_intersection_id = ray_id * P.max_intersections_per_ray + intersection_id;
    distances[          global_intersection_id] = trace.distance_to_surface;
    cell_ids[           global_intersection_id] = ray_cell_id;
    did_vacuum_reflects[global_intersection_id] = just_hit_vacuum;
    hit_count[                     ray_cell_id] = 1;
    just_hit_vacuum = 0;

    // Move ray forward to intersection surface
    x += x_dir * trace.distance_to_surface;
    y += y_dir * trace.distance_to_surface;

    // Create a test point inside the next cell
    double x_across_surface = x + trace.surface_normal_x * BUMP;
    double y_across_surface = y + trace.surface_normal_y * BUMP;

    // Look up the "neighbor" cell id of the test point.
    // This function also gives us some info on if we hit a boundary, and what type it was.
    CellLookup lookup = find_cell_id(P, x_across_surface, y_across_surface);

    // A sanity check
    //assert(lookup.cell_id != cell_id || is_terminal);

    // If we hit an outer boundary, reflect the ray
    if( lookup.boundary_condition != NONE && !is_terminal )
    {
      trace.surface_normal_x *= -1.0;
      trace.surface_normal_y *= -1.0;
      if( trace.surface_normal_x )
        x_dir *= -1.0;
      else
        y_dir *= -1.0;
    }

    // Note if we hit a vacuum boundary
    if( lookup.boundary_condition == VACUUM )
      just_hit_vacuum = 1;

    // If we didn't hit a boundary, the ray is moved into the next cell
    if( lookup.boundary_condition == NONE )
    {
      ray_cell_id = lookup.cell_id;
      x_idx =   lookup.cartesian_cell_idx_x;
      y_idx =   lookup.cartesian_cell_idx_y;
    }

    // Move ray off of surface
    x += trace.surface_normal_x * BUMP;
    y += trace.surface_normal_y * BUMP;

    // Add this intersection's distance to the total for the ray
    distance_travelled += trace.distance_to_surface;

    // Some sanity checks (can be disabled if desired)
    //assert(cell_id >= 0 && cell_id < P.n_cells);
    //assert(x > 0.0 && y > 0.0 && x < P.length_per_dimension && y < P.length_per_dimension);
  }

  // Some sanity checks (can be disabled if desired)
  //if(intersection_id >= P.max_intersections_per_ray)
  //{
  //  printf("WARNING: Increase max number of intersections per ray\n");
  //  print_ray(x, y, x_dir, y_dir, cell_id);
  //}

  // Bank the ray's status for use in the next iteration
  location_x[ ray_id] = x;
  location_y[ ray_id] = y;
  direction_x[ray_id] = x_dir;
  direction_y[ray_id] = y_dir;
  cell_id[ray_id] = ray_cell_id;

  // Bank number of intersections that this ray had this iteration
  n_intersections[ray_id] = intersection_id;
}

CellLookup find_cell_id(Parameters P, double x, double y)
{
  int cartesian_cell_idx_x = floor(x * P.inverse_cell_width);
  int cartesian_cell_idx_y = floor(y * P.inverse_cell_width);

  int boundary_x = floor(x * P.inverse_length_per_dimension) + 1;
  int boundary_y = floor(y * P.inverse_length_per_dimension) + 1;

  int boundary_condition = P.boundary_conditions[boundary_x][boundary_y];

  int cell_id = cartesian_cell_idx_y * P.n_cells_per_dimension + cartesian_cell_idx_x; 

  CellLookup lookup;
  lookup.cell_id = cell_id;
  lookup.cartesian_cell_idx_x = cartesian_cell_idx_x;
  lookup.cartesian_cell_idx_y = cartesian_cell_idx_y;
  lookup.boundary_condition = boundary_condition;
  return lookup;
}

TraceResult cartesian_ray_trace(double x, double y, double cell_width, int x_idx, int y_idx, double x_dir, double y_dir)
{
  x -= x_idx * cell_width;
  y -= y_idx * cell_width;

  double min_dist = 1e9;
  double surface_normal_x = 0.0;
  double surface_normal_y = 0.0;

  // Test out all 4 surfaces
  double x_pos_dist = (cell_width - x) / x_dir;
  double y_pos_dist = (cell_width - y) / y_dir;
  double x_neg_dist = -x / x_dir;
  double y_neg_dist = -y / y_dir;

  // Determine closest one
  if( x_pos_dist < min_dist && x_pos_dist > 0 )
  {
    min_dist = x_pos_dist;
    surface_normal_x = 1.0;
    surface_normal_y = 0.0;
  }
  if( y_pos_dist < min_dist && y_pos_dist > 0 )
  {
    min_dist = y_pos_dist;
    surface_normal_x = 0.0;
    surface_normal_y = 1.0;
  }
  if( x_neg_dist < min_dist && x_neg_dist > 0)
  {
    min_dist = x_neg_dist;
    surface_normal_x = -1.0;
    surface_normal_y =  0.0;
  }
  if( y_neg_dist < min_dist && y_neg_dist > 0)
  {
    min_dist = y_neg_dist;
    surface_normal_x =  0.0;
    surface_normal_y = -1.0;
  }

  TraceResult trace;
  trace.distance_to_surface = min_dist;
  trace.surface_normal_x = surface_normal_x;
  trace.surface_normal_y = surface_normal_y;

  return trace;
}
