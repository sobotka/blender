/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "BLI_map.hh"
#include "BLI_math_matrix.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"

#include "BKE_mesh.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "node_geometry_util.hh"

static bNodeSocketTemplate geo_node_mesh_primitive_uv_sphere_in[] = {
    {SOCK_INT, N_("Segments"), 32, 0.0f, 0.0f, 0.0f, 3, 1024},
    {SOCK_INT, N_("Rings"), 16, 0.0f, 0.0f, 0.0f, 3, 1024},
    {SOCK_FLOAT, N_("Radius"), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, FLT_MAX, PROP_DISTANCE},
    {SOCK_VECTOR, N_("Location"), 0.0f, 0.0f, 0.0f, 1.0f, -FLT_MAX, FLT_MAX, PROP_TRANSLATION},
    {SOCK_VECTOR, N_("Rotation"), 0.0f, 0.0f, 0.0f, 1.0f, -FLT_MAX, FLT_MAX, PROP_EULER},
    {-1, ""},
};

static bNodeSocketTemplate geo_node_mesh_primitive_uv_sphere_out[] = {
    {SOCK_GEOMETRY, N_("Geometry")},
    {-1, ""},
};

namespace blender::nodes {

static int vert_total(const int segments, const int rings)
{
  return segments * (rings - 1) + 2;
}

static int edge_total(const int segments, const int rings)
{
  return segments * (rings * 2 - 1);
}

static int corner_total(const int segments, const int rings)
{
  const int quad_corners = 4 * segments * (rings - 2);
  const int tri_corners = 3 * segments * 2;
  return quad_corners + tri_corners;
}

static int face_total(const int segments, const int rings)
{
  const int quads = segments * (rings - 2);
  const int triangles = segments * 2;
  return quads + triangles;
}

static void calculate_vertex_coords_and_normals(MutableSpan<MVert> verts,
                                                const float radius,
                                                const int segments,
                                                const int rings)
{
  const float delta_theta = M_PI / rings;
  const float delta_phi = (2 * M_PI) / segments;

  copy_v3_v3(verts[0].co, float3(0.0f, 0.0f, radius));
  normal_float_to_short_v3(verts[0].no, float3(0.0f, 0.0f, 1.0f));

  int vert_index = 1;
  float theta = delta_theta;
  for (const int UNUSED(ring) : IndexRange(rings - 1)) {
    float phi = 0.0f;
    const float z = cosf(theta);
    for (const int UNUSED(segment) : IndexRange(segments)) {
      const float x = sinf(theta) * cosf(phi);
      const float y = sinf(theta) * sinf(phi);
      copy_v3_v3(verts[vert_index].co, float3(x, y, z) * radius);
      normal_float_to_short_v3(verts[vert_index].no, float3(x, y, z));
      phi += delta_phi;
      vert_index++;
    }
    theta += delta_theta;
  }

  copy_v3_v3(verts.last().co, float3(0.0f, 0.0f, -radius));
  normal_float_to_short_v3(verts.last().no, float3(0.0f, 0.0f, -1.0f));
}

static void calculate_edge_indices(MutableSpan<MEdge> edges, const int segments, const int rings)
{
  int edge_index = 0;

  /* Add the edges connecting the top vertex to the first ring. */
  const int first_vert_ring_index_start = 1;
  for (const int segment : IndexRange(segments)) {
    MEdge &edge = edges[edge_index++];
    edge.v1 = 0;
    edge.v2 = first_vert_ring_index_start + segment;
  }

  int ring_vert_index_start = 1;
  for (const int ring : IndexRange(rings - 1)) {
    const int next_ring_vert_index_start = ring_vert_index_start + segments;

    /* Add the edges running along each ring. */
    for (const int segment : IndexRange(segments)) {
      MEdge &edge_in_ring = edges[edge_index++];
      edge_in_ring.v1 = ring_vert_index_start + segment;
      edge_in_ring.v2 = ring_vert_index_start + ((segment + 1) % segments);
    }

    /* Add the edges connecting to the next ring. */
    if (ring < rings - 2) {
      for (const int segment : IndexRange(segments)) {
        MEdge &edge_to_next_ring = edges[edge_index++];
        edge_to_next_ring.v1 = ring_vert_index_start + segment;
        edge_to_next_ring.v2 = next_ring_vert_index_start + segment;
      }
    }
    ring_vert_index_start += segments;
  }

  /* Add the edges connecting the last ring to the bottom vertex. */
  const int last_vert_index = vert_total(segments, rings) - 1;
  const int last_vert_ring_start = last_vert_index - segments;
  for (const int segment : IndexRange(segments)) {
    MEdge &edge = edges[edge_index++];
    edge.v1 = last_vert_index;
    edge.v2 = last_vert_ring_start + segment;
  }
}

static void calculate_faces(MutableSpan<MLoop> loops,
                            MutableSpan<MPoly> polys,
                            const int segments,
                            const int rings)
{
  int loop_index = 0;
  int poly_index = 0;

  /* Add the triangles conntected to the top vertex. */
  const int first_vert_ring_index_start = 1;
  for (const int segment : IndexRange(segments)) {
    MPoly &poly = polys[poly_index++];
    poly.loopstart = loop_index;
    poly.totloop = 3;

    MLoop &loop_a = loops[loop_index++];
    loop_a.v = 0;
    loop_a.e = segment;

    MLoop &loop_b = loops[loop_index++];
    loop_b.v = first_vert_ring_index_start + segment;
    loop_b.e = segments + segment;

    MLoop &loop_c = loops[loop_index++];
    loop_c.v = first_vert_ring_index_start + (segment + 1) % segments;
    loop_c.e = (segment + 1) % segments;
  }

  int ring_vert_index_start = 1;
  int ring_edge_index_start = segments;
  for (const int UNUSED(ring) : IndexRange(rings - 2)) {
    const int next_ring_vert_index_start = ring_vert_index_start + segments;
    const int next_ring_edge_index_start = ring_edge_index_start + segments * 2;
    const int ring_vertical_edge_index_start = ring_edge_index_start + segments;

    for (const int segment : IndexRange(segments)) {
      MPoly &poly = polys[poly_index++];
      poly.loopstart = loop_index;
      poly.totloop = 4;

      MLoop &loop_a = loops[loop_index++];
      loop_a.v = ring_vert_index_start + segment;
      loop_a.e = ring_vertical_edge_index_start + segment;

      MLoop &loop_b = loops[loop_index++];
      loop_b.v = next_ring_vert_index_start + segment;
      loop_b.e = next_ring_edge_index_start + segment;

      MLoop &loop_c = loops[loop_index++];
      loop_c.v = next_ring_vert_index_start + ((segment + 1) % segments);
      loop_c.e = ring_vertical_edge_index_start + ((segment + 1) % segments);

      MLoop &loop_d = loops[loop_index++];
      loop_d.v = ring_vert_index_start + ((segment + 1) % segments);
      loop_d.e = ring_edge_index_start + segment;
    }
    ring_vert_index_start += segments;
    ring_edge_index_start += segments * 2;
  }

  /* Add the triangles connected to the bottom vertex. */
  const int last_edge_ring_start = segments * (rings - 2) * 2 + segments;
  const int bottom_edge_fan_start = last_edge_ring_start + segments;
  const int last_vert_index = vert_total(segments, rings) - 1;
  const int last_vert_ring_start = last_vert_index - segments;
  for (const int segment : IndexRange(segments)) {
    MPoly &poly = polys[poly_index++];
    poly.loopstart = loop_index;
    poly.totloop = 3;

    MLoop &loop_a = loops[loop_index++];
    loop_a.v = last_vert_index;
    loop_a.e = bottom_edge_fan_start + ((segment + 1) % segments);

    MLoop &loop_b = loops[loop_index++];
    loop_b.v = last_vert_ring_start + (segment + 1) % segments;
    loop_b.e = last_edge_ring_start + segment;

    MLoop &loop_c = loops[loop_index++];
    loop_c.v = last_vert_ring_start + segment;
    loop_c.e = bottom_edge_fan_start + segment;
  }
}

static Mesh *create_uv_sphere_mesh(const float3 location,
                                   const float3 rotation,
                                   const float radius,
                                   const int segments,
                                   const int rings)
{
  float4x4 transform;
  loc_eul_size_to_mat4(transform.values, location, rotation, float3(1.0f));

  Mesh *mesh = BKE_mesh_new_nomain(vert_total(segments, rings),
                                   edge_total(segments, rings),
                                   0,
                                   corner_total(segments, rings),
                                   face_total(segments, rings));
  MutableSpan<MVert> verts = MutableSpan<MVert>(mesh->mvert, mesh->totvert);
  MutableSpan<MEdge> edges = MutableSpan<MEdge>(mesh->medge, mesh->totedge);
  MutableSpan<MLoop> loops = MutableSpan<MLoop>(mesh->mloop, mesh->totloop);
  MutableSpan<MPoly> polys = MutableSpan<MPoly>(mesh->mpoly, mesh->totpoly);

  calculate_vertex_coords_and_normals(verts, radius, segments, rings);

  calculate_edge_indices(edges, segments, rings);

  calculate_faces(loops, polys, segments, rings);

  BLI_assert(BKE_mesh_is_valid(mesh));

  return mesh;
}

static void geo_node_mesh_primitive_uv_sphere_exec(GeoNodeExecParams params)
{
  GeometrySet geometry_set;

  const int segments_num = params.extract_input<int>("Segments");
  const int rings_num = params.extract_input<int>("Rings");
  if (segments_num < 3 || rings_num < 3) {
    params.set_output("Geometry", geometry_set);
    return;
  }

  const float radius = params.extract_input<float>("Radius");
  const float3 location = params.extract_input<float3>("Location");
  const float3 rotation = params.extract_input<float3>("Rotation");

  geometry_set.replace_mesh(
      create_uv_sphere_mesh(location, rotation, radius, segments_num, rings_num));

  params.set_output("Geometry", geometry_set);
}

}  // namespace blender::nodes

void register_node_type_geo_mesh_primitive_uv_sphere()
{
  static bNodeType ntype;

  geo_node_type_base(
      &ntype, GEO_NODE_MESH_PRIMITIVE_UV_SPHERE, "UV Sphere", NODE_CLASS_GEOMETRY, 0);
  node_type_socket_templates(
      &ntype, geo_node_mesh_primitive_uv_sphere_in, geo_node_mesh_primitive_uv_sphere_out);
  ntype.geometry_node_execute = blender::nodes::geo_node_mesh_primitive_uv_sphere_exec;
  nodeRegisterType(&ntype);
}
