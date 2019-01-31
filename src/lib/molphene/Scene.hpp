#ifndef MOLPHENE_SCENE_HPP
#define MOLPHENE_SCENE_HPP

#include "stdafx.hpp"

#include "BoundingSphere.hpp"
#include "Camera.hpp"
#include "ColorLightBuffer.hpp"
#include "DirectionalLight.hpp"
#include "Fog.hpp"
#include "Material.hpp"
#include "PointLight.hpp"
#include "SpacefillRepresentation.hpp"
#include "SphereMeshBuilder.hpp"
#include "SpotLight.hpp"
#include "Viewport.hpp"
#include "m3d.hpp"
#include "mol/Atom.hpp"
#include "mol/Bond.hpp"
#include "mol/Molecule.hpp"

namespace molphene {

class Scene {
public:
  struct ConfigType {
    using float_type = double;
  };

  struct SphereMeshAttr {
    Rgba8 color;
    std::size_t index;
    Vec2<double> texcoord;
    Sphere<double> sphere;
  };

  using float_type = typename type_configs<ConfigType>::float_type;
  using size_type = typename type_configs<ConfigType>::size_type;

  using Vec3f = Vec3<float_type>;
  using Vec2f = Vec2<float_type>;
  using Mat4f = Mat4<float_type>;
  using Mat3f = Mat3<float_type>;

  using BoundingSphere = BoundingSphere<float_type>;

  using Camera = Camera<ConfigType>;

  using Fog = Fog<Rgba8, ConfigType>;

  using Material = Material<Rgba8, ConfigType>;

  using LightSource = DirectionalLight<Rgba8, ConfigType>;

  using PointLight = PointLight<Rgba8, ConfigType>;

  using SpotLight = SpotLight<Rgba8, ConfigType>;

  using SphereMeshBuilder = SphereMeshBuilder<ConfigType>;

  using Viewport = Viewport<size_type>;

  auto setup_graphics() noexcept -> bool;

  void reset_mesh() noexcept;

  void change_dimension(size_type width, size_type height) noexcept;

  void rotate(Vec3f rot) noexcept;

  void open_chemdoodle_json_stream(std::istream& is);

  auto parse_chemdoodle_json(const std::string& json) -> Molecule;

  auto model_matrix() const noexcept -> Mat4f;

  auto light_source() const noexcept -> LightSource;

  auto material() const noexcept -> Material;

  auto fog() const noexcept -> Fog;

  auto viewport() const noexcept -> Viewport;

  auto mesh_buffers() const noexcept -> const ColorLightBuffer*;

  auto bounding_sphere() const noexcept -> BoundingSphere;

  auto build_sphere_mesh(const std::vector<SphereMeshAttr>& atoms)
   -> std::unique_ptr<ColorLightBuffer>;

  template<typename Represent, typename InRange, typename OutIter>
  void transform_sphere_attrs(const Represent& representation,
                              const InRange& atoms,
                              OutIter output)
  {
    const auto tex_size =
     static_cast<std::size_t>(std::ceil(std::sqrt(atoms.size())));
    auto aindex = std::size_t{0};
    for(auto atomptr : atoms) {
      const auto& atom = *atomptr;
      const auto element = atom.element();
      const auto apos = atom.position();
      const auto arad = representation.atom_radius(element);
      const auto acol = representation.atom_color(atom);

      const auto atex = Vec2f{float_type(aindex % tex_size),
                              std::floor(float_type(aindex) / tex_size)} /
                        tex_size;

      auto sphere_mesh_attr = SphereMeshAttr{};
      sphere_mesh_attr.sphere = {arad, apos};
      sphere_mesh_attr.index = aindex++;
      sphere_mesh_attr.texcoord = atex;
      sphere_mesh_attr.color = acol;

      *output++ = sphere_mesh_attr;
    }
  }

private:
  SpacefillRepresentation spacefill_representation_;

  LightSource light_source_;

  PointLight point_light_source_;

  SpotLight spot_light_source_;

  Fog fog_;

  Material material_;

  Viewport viewport_;

  Mat4f model_matrix_{1};

  BoundingSphere bounding_sphere_;

  Molecule molecule_;
};

} // namespace molphene

#endif
