#include "ballstick_instancing_representation.hpp"

#include "gl/draw_instanced_arrays.hpp"
#include "gl_vertex_attribs_guard.hpp"
#include "shader_attrib_location.hpp"

namespace molphene {

auto ballstick_instancing_representation::atom_radius(
 typename atom::atom_element element) const noexcept -> double
{
  switch(atom_radius_type) {
  case atom_radius_kind::van_der_waals:
    return element.rvdw * radius_size;
  case atom_radius_kind::covalent:
    return element.rcov * radius_size;
  default:
    return radius_size;
  }
}

auto ballstick_instancing_representation::atom_color(const atom& atom) const
 noexcept -> rgba8
{
  return color_manager.get_element_color(atom.element().symbol);
}

void ballstick_instancing_representation::render(
 const color_light_shader& shader) const noexcept
{
  const auto verts_guard =
   gl_vertex_attribs_guard<shader_attrib_location::vertex,
                           shader_attrib_location::normal,
                           shader_attrib_location::texcoordcolor,
                           shader_attrib_location::transformation,
                           shader_attrib_location::transformation_1,
                           shader_attrib_location::transformation_2,
                           shader_attrib_location::transformation_3>{};

  {
    assert(all_has_same_props(*bond1_cylinder_buffer_positions,
                              *bond1_cylinder_buffer_normals));

    assert(all_has_same_props(*bond1_cylinder_buffer_transforms,
                              *bond1_cylinder_buffer_texcoords));

    shader.color_texture_image(bond1_cylinder_color_texture->texture_image());

    const auto size = bond1_cylinder_buffer_transforms->size();
    const auto remain_instances =
     bond1_cylinder_buffer_transforms->remain_instances();
    const auto instances_per_block =
     bond1_cylinder_buffer_transforms->instances_per_block();
    const auto verts_per_instance =
     bond1_cylinder_buffer_positions->verts_per_instance();

    bond1_cylinder_buffer_positions->bind_attrib_pointer_index(0);
    bond1_cylinder_buffer_normals->bind_attrib_pointer_index(0);

    for(auto i = GLsizei{0}; i < size; ++i) {
      const auto total_instances =
       GLsizei{i == (size - 1) ? remain_instances : instances_per_block};

      bond1_cylinder_buffer_texcoords->bind_attrib_pointer_index(i);
      bond1_cylinder_buffer_transforms->bind_attrib_pointer_index(i);

      gl::draw_arrays_instanced(
       GL_TRIANGLE_STRIP, 0, verts_per_instance, total_instances);
    }
  }

  {
    assert(all_has_same_props(*bond2_cylinder_buffer_positions,
                              *bond2_cylinder_buffer_normals));
    assert(all_has_same_props(*bond2_cylinder_buffer_transforms,
                              *bond2_cylinder_buffer_texcoords));

    shader.color_texture_image(bond2_cylinder_color_texture->texture_image());

    const auto size = bond2_cylinder_buffer_transforms->size();
    const auto remain_instances =
     bond2_cylinder_buffer_transforms->remain_instances();
    const auto instances_per_block =
     bond2_cylinder_buffer_transforms->instances_per_block();
    const auto verts_per_instance =
     bond2_cylinder_buffer_positions->verts_per_instance();

    bond2_cylinder_buffer_positions->bind_attrib_pointer_index(0);
    bond2_cylinder_buffer_normals->bind_attrib_pointer_index(0);

    for(auto i = GLsizei{0}; i < size; ++i) {
      const auto total_instances =
       GLsizei{i == (size - 1) ? remain_instances : instances_per_block};

      bond2_cylinder_buffer_texcoords->bind_attrib_pointer_index(i);
      bond2_cylinder_buffer_transforms->bind_attrib_pointer_index(i);

      gl::draw_arrays_instanced(
       GL_TRIANGLE_STRIP, 0, verts_per_instance, total_instances);
    }
  }

  atom_sphere_buffers.draw(shader);
}

} // namespace molphene
