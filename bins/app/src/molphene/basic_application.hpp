#ifndef MOLPHENE_APP_APPLICATION_VIEW_HPP
#define MOLPHENE_APP_APPLICATION_VIEW_HPP

#include <set>
#include <sstream>
#include <string>
#include <utility>

#include <molecule/chemdoodle_json_parser.hpp>
#include <molecule/molecule.hpp>

#include <molphene/algorithm.hpp>
#include <molphene/gl_renderer.hpp>
#include <molphene/scene.hpp>

#include <molphene/ballstick_representation.hpp>
#include <molphene/camera.hpp>
#include <molphene/cylinder_mesh_builder.hpp>
#include <molphene/cylinder_vertex_buffers_batch.hpp>
#include <molphene/cylinder_vertex_buffers_instanced.hpp>
#include <molphene/drawable.hpp>
#include <molphene/instance_copy_builder.hpp>
#include <molphene/molecule_display.hpp>
#include <molphene/molecule_to_shape.hpp>
#include <molphene/spacefill_representation.hpp>
#include <molphene/sphere_mesh_builder.hpp>
#include <molphene/sphere_vertex_buffers_batch.hpp>
#include <molphene/sphere_vertex_buffers_instanced.hpp>

#include <molphene/io/click_state.hpp>

#include "buffers_builder.hpp"

namespace molphene {

template<typename TApp>
class basic_application {
public:
  using camera_type = camera<void>;

  using spacefill_representation =
   basic_spacefill_representation<sphere_vertex_buffers_batch>;

  using spacefill_representation_instanced =
   basic_spacefill_representation<sphere_vertex_buffers_instanced>;

  using ballstick_representation =
   basic_ballstick_representation<sphere_vertex_buffers_batch,
                                  cylinder_vertex_buffers_batch>;

  using ballstick_representation_instanced =
   basic_ballstick_representation<sphere_vertex_buffers_instanced,
                                  cylinder_vertex_buffers_instanced>;

  using representations_container = std::list<drawable>;

  void setup()
  {
    static_cast<TApp*>(this)->init_context();

    representations_.emplace_back(spacefill_representation{});
    representations_.emplace_back(ballstick_representation{});
    representations_.emplace_back(spacefill_representation_instanced{});
    representations_.emplace_back(ballstick_representation_instanced{});

    scene.setup_graphics();
    renderer.init();

    const auto [width, height] = static_cast<TApp*>(this)->framebuffer_size();

    renderer.change_dimension(width, height);
    camera.aspect_ratio(width, height);
    camera.update_view_matrix();

    scene.reset_mesh(molecule);

    // representation_ = molecule_display::ball_and_stick;
    // representation_ = molecule_display::spacefill_instance;
    // representation_ = molecule_display::spacefill;
    representation_ = molecule_display::ball_and_stick_instance;
    reset_representation(molecule);
  }

  void open_pdb_data(std::string pdbdata)
  {
    auto pdbstm = std::stringstream{pdbdata};
    molecule = chemdoodle_json_parser{}.parse(pdbstm);

    scene.reset_mesh(molecule);
    reset_representation(molecule);
    camera.top = scene.bounding_sphere().radius() + 2;
    camera.update_view_matrix();
  }

  void render_frame()
  {
    renderer.render(scene, camera, representations_);
  }

  void canvas_size_change_callback(int width, int height)
  {
    renderer.change_dimension(width, height);
    camera.aspect_ratio(width, height);
    camera.update_view_matrix();
  }

  void change_representation(int representation_type)
  {
    switch(representation_type) {
    case static_cast<int>(molecule_display::spacefill): {
      representation(molecule_display::spacefill, molecule);
    } break;
    case static_cast<int>(molecule_display::ball_and_stick): {
      representation(molecule_display::ball_and_stick, molecule);
    } break;
    case static_cast<int>(molecule_display::spacefill_instance): {
      representation(molecule_display::spacefill_instance, molecule);
    } break;
    case static_cast<int>(molecule_display::ball_and_stick_instance): {
      representation(molecule_display::ball_and_stick_instance, molecule);
    } break;
    }
  }

  void representation(molecule_display value, const molecule& mol)
  {
    if(representation_ == value) {
      return;
    }

    representation_ = value;
    reset_representation(mol);
  }

  auto build_spacefill_representation(const molecule& mol) const
   -> spacefill_representation
  {
    namespace range = boost::range;

    auto spacefill = spacefill_representation{};

    auto atoms = detail::make_reserved_vector<const atom*>(mol.atoms().size());
    range::transform(
     mol.atoms(), std::back_inserter(atoms), [](auto& atom) noexcept {
       return &atom;
     });

    auto sphere_mesh_attrs =
     detail::make_reserved_vector<sphere_mesh_attribute>(atoms.size());

    atoms_to_sphere_attrs(atoms,
                          std::back_inserter(sphere_mesh_attrs),
                          {spacefill.radius_type, spacefill.radius_size, 1.});

    spacefill.atom_sphere_buffers.buffer_positions =
     build_sphere_mesh_positions(sph_mesh_builder, sphere_mesh_attrs);

    spacefill.atom_sphere_buffers.buffer_normals =
     build_sphere_mesh_normals(sph_mesh_builder, sphere_mesh_attrs);

    spacefill.atom_sphere_buffers.buffer_texcoords =
     build_sphere_mesh_texcoords(sph_mesh_builder, sphere_mesh_attrs);

    spacefill.atom_sphere_buffers.color_texture =
     build_shape_color_texture(sphere_mesh_attrs);

    return spacefill;
  }

  auto build_ballstick_representation(const molecule& mol)
   -> ballstick_representation
  {
    namespace range = boost::range;

    auto ballnstick = ballstick_representation{};

    auto bonds = detail::make_reserved_vector<const bond*>(mol.bonds().size());
    range::transform(
     mol.bonds(), std::back_inserter(bonds), [](auto& bond) noexcept {
       return std::addressof(bond);
     });

    using pair_atoms_t = std::pair<const atom*, const atom*>;
    auto bond_atoms = detail::make_reserved_vector<pair_atoms_t>(bonds.size());
    range::transform(
     bonds,
     std::back_inserter(bond_atoms),
     [& atoms = mol.atoms()](auto bond) noexcept {
       return std::make_pair(&atoms.at(bond->atom1()),
                             &atoms.at(bond->atom2()));
     });

    auto atoms = detail::make_reserved_vector<const atom*>(mol.atoms().size());
    range::transform(
     mol.atoms(), std::back_inserter(atoms), [](auto& atom) noexcept {
       return &atom;
     });

    auto atoms_in_bond = std::set<const atom*>{};
    boost::for_each(
     bond_atoms, [&](auto atom_pair) noexcept {
       atoms_in_bond.insert({atom_pair.first, atom_pair.second});
     });

    {
      auto sphere_mesh_attrs =
       detail::make_reserved_vector<sphere_mesh_attribute>(
        atoms_in_bond.size());

      atoms_to_sphere_attrs(
       atoms_in_bond,
       std::back_inserter(sphere_mesh_attrs),
       {ballnstick.atom_radius_type, ballnstick.atom_radius_size, 0.5});

      ballnstick.atom_sphere_buffers.buffer_positions =
       build_sphere_mesh_positions(sph_mesh_builder, sphere_mesh_attrs);

      ballnstick.atom_sphere_buffers.buffer_normals =
       build_sphere_mesh_normals(sph_mesh_builder, sphere_mesh_attrs);

      ballnstick.atom_sphere_buffers.buffer_texcoords =
       build_sphere_mesh_texcoords(sph_mesh_builder, sphere_mesh_attrs);

      ballnstick.atom_sphere_buffers.color_texture =
       build_shape_color_texture(sphere_mesh_attrs);
    }

    auto cylinder_mesh_attrs =
     detail::make_reserved_vector<cylinder_mesh_attribute>(bond_atoms.size());

    bonds_to_cylinder_attrs(bond_atoms,
                            std::back_insert_iterator(cylinder_mesh_attrs),
                            {true, ballnstick.radius_size});

    ballnstick.bond1_cylinder_buffers.buffer_positions =
     build_cylinder_mesh_positions(cyl_mesh_builder, cylinder_mesh_attrs);

    ballnstick.bond1_cylinder_buffers.buffer_normals =
     build_cylinder_mesh_normals(cyl_mesh_builder, cylinder_mesh_attrs);

    ballnstick.bond1_cylinder_buffers.buffer_texcoords =
     build_cylinder_mesh_texcoords(cyl_mesh_builder, cylinder_mesh_attrs);

    ballnstick.bond1_cylinder_buffers.color_texture =
     build_shape_color_texture(cylinder_mesh_attrs);

    cylinder_mesh_attrs.clear();

    bonds_to_cylinder_attrs(bond_atoms,
                            std::back_insert_iterator(cylinder_mesh_attrs),
                            {false, ballnstick.radius_size});

    ballnstick.bond2_cylinder_buffers.buffer_positions =
     build_cylinder_mesh_positions(cyl_mesh_builder, cylinder_mesh_attrs);

    ballnstick.bond2_cylinder_buffers.buffer_normals =
     build_cylinder_mesh_normals(cyl_mesh_builder, cylinder_mesh_attrs);

    ballnstick.bond2_cylinder_buffers.buffer_texcoords =
     build_cylinder_mesh_texcoords(cyl_mesh_builder, cylinder_mesh_attrs);

    ballnstick.bond2_cylinder_buffers.color_texture =
     build_shape_color_texture(cylinder_mesh_attrs);

    return ballnstick;
  }

  auto build_spacefill_representation_instanced(const molecule& mol) const
   -> spacefill_representation_instanced
  {
    namespace range = boost::range;

    auto spacefill = spacefill_representation_instanced{};

    auto atoms = detail::make_reserved_vector<const atom*>(mol.atoms().size());
    range::transform(
     mol.atoms(), std::back_inserter(atoms), [](auto& atom) noexcept {
       return &atom;
     });

    auto sphere_mesh_attrs =
     detail::make_reserved_vector<sphere_mesh_attribute>(atoms.size());

    atoms_to_sphere_attrs(atoms,
                          std::back_inserter(sphere_mesh_attrs),
                          {spacefill.radius_type, spacefill.radius_size, 1.});

    const auto sphere_attr = std::array<sphere_mesh_attribute, 1>{};

    spacefill.atom_sphere_buffers.buffer_positions =
     build_sphere_mesh_positions(sph_mesh_builder, sphere_attr);

    spacefill.atom_sphere_buffers.buffer_normals =
     build_sphere_mesh_normals(sph_mesh_builder, sphere_attr);

    spacefill.atom_sphere_buffers.buffer_texcoords =
     build_sphere_mesh_texcoord_instances(copy_builder, sphere_mesh_attrs);

    spacefill.atom_sphere_buffers.buffer_transforms =
     build_sphere_mesh_transform_instances(copy_builder, sphere_mesh_attrs);

    spacefill.atom_sphere_buffers.color_texture =
     build_shape_color_texture(sphere_mesh_attrs);

    return spacefill;
  }

  auto build_ballstick_instance_representation(const molecule& mol)
   -> ballstick_representation_instanced
  {
    namespace range = boost::range;

    auto ballnstick = ballstick_representation_instanced{};

    auto bonds = detail::make_reserved_vector<const bond*>(mol.bonds().size());
    range::transform(
     mol.bonds(), std::back_inserter(bonds), [](auto& bond) noexcept {
       return std::addressof(bond);
     });

    using pair_atoms_t = std::pair<const atom*, const atom*>;
    auto bond_atoms = detail::make_reserved_vector<pair_atoms_t>(bonds.size());
    range::transform(
     bonds,
     std::back_inserter(bond_atoms),
     [& atoms = mol.atoms()](auto bond) noexcept {
       return std::make_pair(&atoms.at(bond->atom1()),
                             &atoms.at(bond->atom2()));
     });

    auto atoms = detail::make_reserved_vector<const atom*>(mol.atoms().size());
    range::transform(
     mol.atoms(), std::back_inserter(atoms), [](auto& atom) noexcept {
       return &atom;
     });

    auto atoms_in_bond = std::set<const atom*>{};
    boost::for_each(
     bond_atoms, [&](auto atom_pair) noexcept {
       atoms_in_bond.insert({atom_pair.first, atom_pair.second});
     });

    {
      auto sphere_mesh_attrs =
       detail::make_reserved_vector<sphere_mesh_attribute>(
        atoms_in_bond.size());

      atoms_to_sphere_attrs(
       atoms_in_bond,
       std::back_inserter(sphere_mesh_attrs),
       {ballnstick.atom_radius_type, ballnstick.atom_radius_size, 0.5});

      const auto sphere_attr = std::array<sphere_mesh_attribute, 1>{};

      ballnstick.atom_sphere_buffers.buffer_positions =
       build_sphere_mesh_positions(sph_mesh_builder, sphere_attr);

      ballnstick.atom_sphere_buffers.buffer_normals =
       build_sphere_mesh_normals(sph_mesh_builder, sphere_attr);

      ballnstick.atom_sphere_buffers.buffer_texcoords =
       build_sphere_mesh_texcoord_instances(copy_builder, sphere_mesh_attrs);

      ballnstick.atom_sphere_buffers.buffer_transforms =
       build_sphere_mesh_transform_instances(copy_builder, sphere_mesh_attrs);

      ballnstick.atom_sphere_buffers.color_texture =
       build_shape_color_texture(sphere_mesh_attrs);
    }

    auto cylinder_mesh_attrs =
     detail::make_reserved_vector<cylinder_mesh_attribute>(bond_atoms.size());

    const auto cylinder_attr = std::array<cylinder_mesh_attribute, 1>{};

    bonds_to_cylinder_attrs(bond_atoms,
                            std::back_insert_iterator(cylinder_mesh_attrs),
                            {true, ballnstick.radius_size});

    ballnstick.bond1_cylinder_buffers.buffer_positions =
     build_cylinder_mesh_positions(cyl_mesh_builder, cylinder_attr);

    ballnstick.bond1_cylinder_buffers.buffer_normals =
     build_cylinder_mesh_normals(cyl_mesh_builder, cylinder_attr);

    ballnstick.bond1_cylinder_buffers.buffer_texcoords =
     build_cylinder_mesh_texcoord_instances(copy_builder, cylinder_mesh_attrs);

    ballnstick.bond1_cylinder_buffers.buffer_transforms =
     build_cylinder_mesh_transform_instances(copy_builder, cylinder_mesh_attrs);

    ballnstick.bond1_cylinder_buffers.color_texture =
     build_shape_color_texture(cylinder_mesh_attrs);

    cylinder_mesh_attrs.clear();

    bonds_to_cylinder_attrs(bond_atoms,
                            std::back_insert_iterator(cylinder_mesh_attrs),
                            {false, ballnstick.radius_size});

    ballnstick.bond2_cylinder_buffers.buffer_positions =
     build_cylinder_mesh_positions(cyl_mesh_builder, cylinder_attr);

    ballnstick.bond2_cylinder_buffers.buffer_normals =
     build_cylinder_mesh_normals(cyl_mesh_builder, cylinder_attr);

    ballnstick.bond2_cylinder_buffers.buffer_texcoords =
     build_cylinder_mesh_texcoord_instances(copy_builder, cylinder_mesh_attrs);

    ballnstick.bond2_cylinder_buffers.buffer_transforms =
     build_cylinder_mesh_transform_instances(copy_builder, cylinder_mesh_attrs);

    ballnstick.bond2_cylinder_buffers.color_texture =
     build_shape_color_texture(cylinder_mesh_attrs);

    return ballnstick;
  }

  void reset_representation(const molecule& mol) noexcept
  {
    representations_.clear();
    switch(representation_) {
    case molecule_display::spacefill: {
      representations_.emplace_back(build_spacefill_representation(mol));
    } break;
    case molecule_display::ball_and_stick: {
      representations_.emplace_back(build_ballstick_representation(mol));
    } break;
    case molecule_display::spacefill_instance: {
      representations_.emplace_back(
       build_spacefill_representation_instanced(mol));
    } break;
    case molecule_display::ball_and_stick_instance: {
      representations_.emplace_back(
       build_ballstick_instance_representation(mol));
    } break;
    }
  }

  void key_press_event(unsigned char charcode, int mods)
  {
    switch(charcode) {
    case 27:
      static_cast<TApp*>(this)->close_app();
      break;
    case 80:
    case 112:
      camera.projection_mode(true);
      break;
    case 79:
    case 111:
      camera.projection_mode(false);
      break;
    case 72:
    case 104:
      representation(molecule_display::ball_and_stick_instance, molecule);
      break;
    case 74:
    case 106:
      representation(molecule_display::spacefill_instance, molecule);
      break;
    case 75:
    case 107:
      representation(molecule_display::spacefill, molecule);
      break;
    case 76:
    case 108:
      representation(molecule_display::ball_and_stick, molecule);
      break;
    }
  }

  void mouse_press_event(int button, int mods, int pos_x, int pos_y)
  {
    click_state.is_down = true;
    click_state.last_x = pos_x;
    click_state.last_y = pos_y;
  }

  void mouse_release_event(int button, int mods, int pos_x, int pos_y)
  {
    click_state.is_down = false;
    click_state.last_x = pos_x;
    click_state.last_y = pos_y;
  }

  void mouse_move_event(int pos_x, int pos_y)
  {
    if(click_state.is_down) {
      const auto delta_x = static_cast<double>(click_state.last_x) - pos_x;
      const auto delta_y = static_cast<double>(click_state.last_y) - pos_y;

      click_state.last_x = pos_x;
      click_state.last_y = pos_y;

      scene.rotate({M_PI * delta_y / 180, M_PI * delta_x / 180, 0});
    }
  }

  void mouse_scroll_event(int offset_x, int offset_y)
  {
    offset_y > 0 ? camera.zoom_in() : camera.zoom_out();
  }

  void framebuffer_size_change_event(int width, int height)
  {
    renderer.change_dimension(width, height);
    camera.aspect_ratio(width, height);
    camera.update_view_matrix();
  }

protected:
  static constexpr auto sph_mesh_builder = sphere_mesh_builder<10, 20>{};

  static constexpr auto cyl_mesh_builder = cylinder_mesh_builder<20>{};
  
  static constexpr auto copy_builder = instance_copy_builder{};

  io::click_state click_state{false, 0, 0};

  scene scene{};

  gl_renderer renderer;

  camera_type camera;

  molecule molecule;

  representations_container representations_;

  molecule_display representation_{molecule_display::spacefill};
};

} // namespace molphene

#endif
