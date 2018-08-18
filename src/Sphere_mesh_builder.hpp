#ifndef MOLPHENE_SPHERE_MESH_BUILDER_HPP
#define MOLPHENE_SPHERE_MESH_BUILDER_HPP

#include "stdafx.hpp"

#include "m3d.hpp"
#include "shape/Uv_sphere.hpp"

namespace molphene {

class Sphere_mesh_builder {
public:
  using Div_type = size_t;

public:
  Sphere_mesh_builder(size_t max_bytes, Div_type lat_div, Div_type long_div);

  void
  build(size_t idx);

  size_t
  get_vertices_size();

  size_t
  size_bytes();

  size_t
  size();

  const std::vector<Vec3f>&
  positions() const noexcept;

  const std::vector<Vec3f>&
  normals() const noexcept;

  const std::vector<Vec2f>&
  texcoords() const noexcept;

  template<typename... Ts>
  Vec2f
  texcoord(Ts&&... args) noexcept
  {
    return tex_ = Vec2f{std::forward<Ts>(args)...};
  }

  Uv_sphere<float>
  sphere(Uv_sphere<float> sphere);

private:
  Div_type lat_div_;

  Div_type long_div_;

  std::vector<Vec3f> normals_;

  std::vector<Vec3f> positions_;

  std::vector<Vec2f> texcoords_;

  size_t size_;

  Vec2f tex_{0};

  Uv_sphere<float> sphere_;
};
} // namespace molphene

#endif
