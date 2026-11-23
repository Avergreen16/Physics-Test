
#include <axiom/include/ui.hpp>

void create_ui(uint cam) {
    static axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
    static axiom::render_system& render_system = axiom::ecs.get_system<axiom::render_system>();
    
    static ivec3 num_cubes = ivec3(4, 4, 4);
    static int num_links = 5;
    static uint object_target = axiom::NULL_ENTITY;
    static bool select_target = false;
    static uint number_base = 10;
    static uint camera = cam;

    ui_system.font_handler.process(render_system.resource_root + "/fonts/axiom-large.bdf", "test");
    //decode_font();

    /*
    axiom::signature s = axiom::update_signature<axiom::camera2d>();
    axiom::update_signature<axiom::transform2d>(s);
    axiom::ecs.collectors.push_back(Collector(s));
    */

    std::function<void(axiom::tab_widget*)> func_lua = [&](axiom::tab_widget* widget) {
        uint32_t w = 160;
        
        ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
        axiom::panel_widget::insert();

        ui_system.buffer(vec4(4.0f));
        ui_system.position(axiom::position_mode::TOP_LEFT);

        axiom::column_widget::insert();
        
        axiom::grid_widget::insert(3);
        
        ui_system.buffer(vec4(2.0f));

        ui_system.position(axiom::position_mode::CENTER_LEFT);
        //axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));
        axiom::text_widget::insert("Call Lua File", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));
        
        axiom::row_widget::insert();
        ui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        uint64_t n = axiom::input_box_widget<std::string>::insert(vec2(40, 16), "", 
            [](axiom::input_box_widget<std::string>& self) {

                /*
                if(self.update) {
                    self.value = glm::clamp(self.value, 1, 16);
                    num_cubes.x = self.value;
                } else {
                    self.value = num_cubes.x;
                }
                */
            }
        );
        //ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        axiom::button_widget::insert(vec2(16, 16), axiom::color_lua, "", 
            [&, n](axiom::button_widget& self) {
                self.icon = vec4(27.0f, 49.0f, 6.0f, 6.0f);
                self.icon_size = vec2(6.0f);
                
                if(self.pressed) {
                    auto* widget = (axiom::input_box_widget<std::string>*)ui_system.widgets[n].get();

                    std::string path = widget->value;

                    std::cout << "path is: " << path << "\n";

                    //axiom::ecs.lua_wrapper.run_file(path);
                }
            }
        );

        ui_system.input_step();
    };

    std::function<void(axiom::tab_widget*)> func_settings = [&](axiom::tab_widget* widget) {
        uint32_t w = 160;
        
        ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
        axiom::panel_widget::insert();

        ui_system.buffer(vec4(4.0f));
        ui_system.position(axiom::position_mode::TOP_LEFT);

        axiom::column_widget::insert();
        
        axiom::grid_widget::insert(3);
        
        ui_system.buffer(vec4(2.0f));
        
        // camera mode
        // number base
        // gravity
        // iteration count
        // substeps
        // dampening
        // DOF
        // num cubes
        // object scale
        // number of links
        
        // number base
        // gravity
        // iteration count
        // substeps
        // 
        // camera mode
        // dampening
        // DOF
        // 
        // num cubes
        // summon cubes
        // 
        // number of links
        // summon chain

        // number base

        ui_system.position(axiom::position_mode::CENTER_LEFT);
        axiom::text_widget::insert("Number Base", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        axiom::drop_widget::insert(vec2(160, 16), axiom::color_editor, 16, 16, 32, {"DECIMAL", "DOZENAL", "HEXADECIMAL", "a", "b", "c", "d"}, 0, 
            [](axiom::drop_widget& self) {
                if(self.selected == 0) number_base = 10;
                else if(self.selected == 1) number_base = 12;
                else if(self.selected == 2) number_base = 16;
            }
        );

        // gravity

        axiom::text_widget::insert("Gravity", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        //axiom::row_widget::insert();
        //ui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));
        
        axiom::slider_widget::insert(vec2(120, 16), 6.0f, axiom::color_physics, vec2(0.0f, 64.0f), 0.0f, 19.62f, "", [](axiom::slider_widget& self) {
            axiom::physics_system3d& ps = axiom::ecs.get_system<axiom::physics_system3d>();
            if(self.pressed) {
                ps.gravity_strength * self.current_value;
            } else {
                self.current_value = ps.gravity_strength;
            }
        });
        ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        /*
        axiom::input_box_widget<float>::insert(vec2(40, 16), 4.0f, 
            [](axiom::input_box_widget<float>& self) {
                axiom::physics_system3d& ps = axiom::ecs.get_system<axiom::physics_system3d>();
                if(self.update) ps.gravity_strength = self.value;
                else self.value = ps.gravity_strength;
            }
        );
        */
        
        //ui_system.input_step();

        // iterations

        axiom::text_widget::insert("Iteration Count", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        //axiom::row_widget::insert();
        //ui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        axiom::slider_widget::insert(vec2(120, 16), 6.0f, axiom::color_physics, vec2(1, 64), 1.0f, 4.0f, "", [](axiom::slider_widget& self) {
            axiom::physics_system3d& ps = axiom::ecs.get_system<axiom::physics_system3d>();
            if(self.pressed) {
                ps.iterations = self.current_value;
            } else {
                self.current_value = ps.iterations;
            }
        });
        ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        /*
        axiom::input_box_widget<float>::insert(vec2(40, 16), 4.0f, 
            [](axiom::input_box_widget<float>& self) {
                axiom::physics_system3d& ps = axiom::ecs.get_system<axiom::physics_system3d>();
                if(self.update) ps.iterations = self.value;
                else self.value = ps.iterations;
            }
        );
        */

        //ui_system.input_step();
        
        // substeps
        
        axiom::text_widget::insert("Substeps", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        
        //axiom::row_widget::insert();
        //ui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        axiom::slider_widget::insert(vec2(120, 16), 6.0f, axiom::color_physics, vec2(1, 64), 1.0f, 4.0f, "", [](axiom::slider_widget& self) {
            axiom::physics_system3d& ps = axiom::ecs.get_system<axiom::physics_system3d>();
            if(self.pressed) {
                float substeps = self.current_value;
                ps.substeps = substeps;
            } else {
                self.current_value = ps.substeps;
            }
        });
        ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        /*
        axiom::input_box_widget<float>::insert(vec2(40, 16), 4.0f, 
            [](axiom::input_box_widget<float>& self) {
                axiom::physics_system3d& ps = axiom::ecs.get_system<axiom::physics_system3d>();
                if(self.update) ps.substeps = self.value;
                else self.value = ps.substeps;
            }
        );
        */

        //ui_system.input_step();

        /*

        // spacer

        ui_system.input_step();
        axiom::spacer_widget::insert(vec2(0.0f, 8.0f), vec2(FLT_MAX, 8.0f), true);
        
        ui_system.buffer(vec4(4.0f));
        axiom::grid_widget::insert(3);

        // dampening
        
        ui_system.position(axiom::position_mode::CENTER_LEFT);
        axiom::text_widget::insert("Dampening", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        axiom::drop_widget::insert(vec2(160, 16), axiom::color_debug, 16, 16, 160, {"ON", "OFF"}, 0, 
            [](axiom::drop_widget& self) {
                static uint32_t prev = 0;
                if(prev != self.selected) {
                    axiom::physics_system3d& ps = axiom::ecs.get_system<axiom::physics_system3d>();

                    prev = self.selected;
                    if(self.selected == 0) ps.do_dampening = true;
                    else if(self.selected == 1) ps.do_dampening = false;
                }
            }
        );
        
        // DOF
        
        ui_system.position(axiom::position_mode::CENTER_LEFT);
        axiom::text_widget::insert("DOF", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        axiom::drop_widget::insert(vec2(160, 16), axiom::color_debug, 16, 16, 160, {"ON", "OFF"}, 0, 
            [](axiom::drop_widget& self) {
                static uint32_t prev = 0;
                if(prev != self.selected) {
                    axiom::physics_system3d& ps = axiom::ecs.get_system<axiom::physics_system3d>();

                    prev = self.selected;
                    if(self.selected == 0) ps.do_DOF = true;
                    else if(self.selected == 1) ps.do_DOF = false;
                }
            }
        );

        // spacer

        ui_system.input_step();
        axiom::spacer_widget::insert(vec2(0.0f, 8.0f), vec2(FLT_MAX, 8.0f), true);
        
        ui_system.buffer(vec4(4.0f));
        axiom::grid_widget::insert(3);

        // num cubes
        
        axiom::text_widget::insert("Number of Cubes", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        axiom::row_widget::insert();
        ui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        uint64_t tw = axiom::text_widget::insert("X", axiom::text_alignment::LEFT, false);
        axiom::input_box_widget<int>::insert(vec2(40, 16), 4.0f, 
            [](axiom::input_box_widget<int>& self) {

                if(self.update) {
                    self.value = glm::clamp(self.value, 1, 16);
                    num_cubes.x = self.value;
                } else {
                    self.value = num_cubes.x;
                }
            }
        );
        //ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        axiom::text_widget::insert("Y", axiom::text_alignment::LEFT, false);
        axiom::input_box_widget<int>::insert(vec2(40, 16), 4.0f, 
            [](axiom::input_box_widget<int>& self) {

                if(self.update) {
                    self.value = glm::clamp(self.value, 1, 16);
                    num_cubes.y = self.value;
                } else {
                    self.value = num_cubes.y;
                }
            }
        );
        //ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        axiom::text_widget::insert("Z", axiom::text_alignment::LEFT, false);
        axiom::input_box_widget<int>::insert(vec2(40, 16), 4.0f, 
            [](axiom::input_box_widget<int>& self) {

                if(self.update) {
                    self.value = glm::clamp(self.value, 1, 16);
                    num_cubes.z = self.value;
                } else {
                    self.value = num_cubes.z;
                }
            }
        );
        //ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));
        
        ui_system.input_step();

        // create crates

        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        axiom::row_widget::insert();
        ui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        axiom::button_widget::insert(vec2(150, 16), axiom::color_debug, "Summon Blocks", 
            [](axiom::button_widget& self) {
                //if(self.pressed) create_crates();
            }
        );
        //ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));
        
        ui_system.input_step();

        // spacer

        ui_system.input_step();
        axiom::spacer_widget::insert(vec2(0.0f, 8.0f), vec2(FLT_MAX, 8.0f), true);
        
        ui_system.buffer(vec4(4.0f));
        axiom::grid_widget::insert(3);

        // number of links
    
        axiom::text_widget::insert("Number of Links", axiom::text_alignment::LEFT, false);

        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));
        
        axiom::row_widget::insert();
        ui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(1.0f));

        axiom::slider_widget::insert(vec2(120, 16), 6.0f, axiom::color_debug, vec2(1, 16), 1, 5, "",
            [](axiom::slider_widget& self) {
                if(self.pressed) {
                    num_links = self.current_value;
                } else {
                    self.current_value = num_links;
                }
            }
        );

        //ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

        axiom::input_box_widget<float>::insert(vec2(40, 16), 4.0f, 
            [](axiom::input_box_widget<float>& self) {
                if(self.update) {
                    num_links = self.value;
                } else {
                    self.value = num_links;
                }
            }
        );

        ui_system.input_step();

        // summon chain
        
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));
        axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

        axiom::row_widget::insert();
        ui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(2.0f, 1.0f));

        axiom::button_widget::insert(vec2(40, 16), axiom::color_debug, "Summon Chain", 
            [](axiom::button_widget& self) {
                //if(self.pressed) create_chain();
            }
        );
        //ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));
        */
        
        ui_system.input_step();
    };

    /*
    
    // object scale

    axiom::text_widget::insert("Object Scale", axiom::text_alignment::LEFT, false);
    axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f));

    axiom::row_widget::insert();
    ui_system.set_attrib(vec2(w, 0.0f), vec2(w, FLT_MAX), vec2(1.0f));

    axiom::slider_widget::insert(vec2(120, 16), 6.0f, hsv_color(2
        if(self.pressed) {
            is.object_scale = self.current_value;
        } else {
            self.current_value = is.object_scale;
        }
    });
    //ui_system.set_attrib(vec2(0.0f, -1), vec2(FLT_MAX, -1), vec2(1.0f));

    axiom::input_box_widget<float>::insert(vec2(40, 16), 4.0f, 
        [](axiom::input_box_widget<float>& self) {

            if(self.update) {
                is.object_scale = self.value;
            } else {
                self.value = is.object_scale;
            }
        }
    );

    ui_system.input_step();
    */

    std::function<void(axiom::tab_widget*)> func_selection = [&](axiom::tab_widget* widget) {
        ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
        axiom::panel_widget::insert();
        
        ui_system.buffer(vec4(3.0f));
        ui_system.position(axiom::position_mode::TOP_LEFT);

        axiom::column_widget::insert(true);

        axiom::text_widget::insert("", axiom::text_alignment::LEFT, true,
            [](std::string str) -> std::string {

                if(object_target == axiom::NULL_ENTITY) return "no object selected";
                else {
                    axiom::transform3d& transform = axiom::ecs.get_component<axiom::transform3d>(object_target);
                    axiom::collider3d& collider = axiom::ecs.get_component<axiom::collider3d>(object_target);

                    std::string ret;

                    int base = number_base;

                    ret += "\\bPosition:\\r";
                    ret += "\n\\bX\\r = " + axiom::to_base(transform.position.x, base, 2);
                    ret += "\n\\bY\\r = " + axiom::to_base(transform.position.y, base, 2);
                    ret += "\n\\bZ\\r = " + axiom::to_base(transform.position.z, base, 2);
                    
                    ret += "\n";
                    ret += "\n\\bOrientation:\\r";
                    ret += "\n" + axiom::to_base(transform.orientation[0].x, base, 2) + " " + axiom::to_base(transform.orientation[1].x, base, 2) + " " + axiom::to_base(transform.orientation[2].x, base, 2);
                    ret += "\n" + axiom::to_base(transform.orientation[0].y, base, 2) + " " + axiom::to_base(transform.orientation[1].y, base, 2) + " " + axiom::to_base(transform.orientation[2].y, base, 2);
                    ret += "\n" + axiom::to_base(transform.orientation[0].z, base, 2) + " " + axiom::to_base(transform.orientation[1].z, base, 2) + " " + axiom::to_base(transform.orientation[2].z, base, 2);
                    ret += "\n\\bVector Lengths:\\r " + axiom::to_base(glm::length(transform.orientation[0]), base, 2) + " " + axiom::to_base(glm::length(transform.orientation[1]), base, 2) + " " + axiom::to_base(glm::length(transform.orientation[2]), base, 2);
                    ret += "\n\\bVector Dots:\\r " + axiom::to_base(dot(transform.orientation[0], transform.orientation[1]), base, 2) + " " + axiom::to_base(dot(transform.orientation[0], transform.orientation[2]), base, 2) + " " + axiom::to_base(dot(transform.orientation[1], transform.orientation[2]), base, 2);


                    ret += "\n";
                    ret += "\n\\bNumber of Subshapes:\\r " + axiom::to_base((int32_t)collider.shapes.size(), base);
                    ret += "\n\\bStatic:\\r " + std::string(collider.is_static ? "TRUE" : "FALSE");
                    ret += "\n\\bRotation Enabled:\\r " + std::string(collider.allow_rotation ? "TRUE" : "FALSE");
                    ret += "\n\\bGravity Enabled:\\r " + std::string(collider.allow_gravity ? "TRUE" : "FALSE");
                    ret += "\n\\bSleeping:\\r " + std::string(collider.sleeping ? "TRUE" : "FALSE");

                    ret += "\n";
                    ret += "\n\\bMass:\\r " + axiom::to_base(collider.mass, base, 2);
                    
                    ret += "\n";
                    ret += "\n\\bInertia Tensor:\\r";
                    ret += "\n" + axiom::to_base(collider.inertia_tensor[0].x, base, 6) + " " + axiom::to_base(collider.inertia_tensor[1].x, base, 6) + " " + axiom::to_base(collider.inertia_tensor[2].x, base, 6);
                    ret += "\n" + axiom::to_base(collider.inertia_tensor[0].y, base, 6) + " " + axiom::to_base(collider.inertia_tensor[1].y, base, 6) + " " + axiom::to_base(collider.inertia_tensor[2].y, base, 6);
                    ret += "\n" + axiom::to_base(collider.inertia_tensor[0].z, base, 6) + " " + axiom::to_base(collider.inertia_tensor[1].z, base, 6) + " " + axiom::to_base(collider.inertia_tensor[2].z, base, 6);
                    
                    ret += "\n";
                    ret += "\n\\bInverse Inertia Tensor:\\r";
                    ret += "\n" + axiom::to_base(collider.inverse_inertia_tensor[0].x, base, 6) + " " + axiom::to_base(collider.inverse_inertia_tensor[1].x, base, 6) + " " + axiom::to_base(collider.inverse_inertia_tensor[2].x, base, 6);
                    ret += "\n" + axiom::to_base(collider.inverse_inertia_tensor[0].y, base, 6) + " " + axiom::to_base(collider.inverse_inertia_tensor[1].y, base, 6) + " " + axiom::to_base(collider.inverse_inertia_tensor[2].y, base, 6);
                    ret += "\n" + axiom::to_base(collider.inverse_inertia_tensor[0].z, base, 6) + " " + axiom::to_base(collider.inverse_inertia_tensor[1].z, base, 6) + " " + axiom::to_base(collider.inverse_inertia_tensor[2].z, base, 6);
                    
                    ret += "\n";
                    ret += "\n\\bVelocity:\\r " + axiom::to_base(collider.velocity.x, base, 2) + " " + axiom::to_base(collider.velocity.y, base, 2) + " " + axiom::to_base(collider.velocity.z, base, 2);
                    
                    ret += "\n\\bAngular Momentum:\\r " + axiom::to_base(collider.angular_momentum.x, base, 2) + " " + axiom::to_base(collider.angular_momentum.y, base, 2) + " " + axiom::to_base(collider.angular_momentum.z, base, 2);

                    return ret;
                }
            }
        );
        
        ui_system.position(axiom::position_mode::TOP_CENTER);
        
        axiom::button_widget::insert(vec2(150, 16), axiom::color_editor, "", 
        [](axiom::button_widget& self) {
            if(self.pressed) select_target = !select_target;

            std::string label;
            if(select_target) {
                object_target = axiom::NULL_ENTITY;

                label = "Click Object";
                float m = fmod(axiom::get_time(), 2.0);
                if(m < 0.666) label += ".  ";
                else if(m < 1.333) label += ".. ";
                else label += "...";
            } else {
                label = "Click to Start Selection";
            }

            //if(label != self.text[0]->string) self.text_dirty = true;
            self.text[0]->string = label;
        });
    };
    
    /*
    std::function<void()> func_global = [&]() {
        ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
        axiom::panel_widget::insert();

        ui_system.buffer(vec4(4.0f));
        ui_system.position(axiom::position_mode::TOP_LEFT);
        axiom::row_widget::insert();
        axiom::text_widget::insert("", axiom::text_alignment::LEFT, true,
            [](std::string str) -> std::string {
                int base = number_base;
                
                static double time = 0.0f;
                static double freq = 1.0f;
                static uint32_t num_frames = 0;

                time += axiom::ecs.delta_time;
                ++num_frames;

                if(time > freq || str == "") {
                    double fps = double(num_frames) / time;
                    time = 0.0;
                    num_frames = 0;

                    std::string string = "\\bFPS: \\r" + axiom::to_base(float(fps), number_base, 3) + "\n\n";

                    std::vector<std::string> order;
                    std::unordered_map<std::string, double> times;
                    double total_time = 0.0f;

                    for(Profiler_Frame& frame : axiom::ecs.profiler.prev_frames) {
                        for(Profiler_Entry& entry : frame.steps) {
                            if(times.contains(entry.name)) {
                                times[entry.name] += entry.time;
                            } else {
                                order.push_back(entry.name);
                                times[entry.name] = entry.time;
                            }
                            total_time += entry.time;
                        }
                    }

                    for(std::string name : order) {
                        double time = times[name];
                        double percent = time / total_time * (base * base);
                        double avg = time / double(axiom::ecs.profiler.num_frames) * 1000.0f;

                        std::string entry_str = "\\b" + name + ": \\r" + axiom::to_base(avg, number_base, 2) + "ms (" + axiom::to_base(percent, number_base, 2) + "%)\n";
                        
                        string += entry_str;
                    }

                    return string;
                } else {
                    return str;
                }
            }
        );
    };

    std::function<void()> func_physics = [&]() {
        ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
        axiom::panel_widget::insert();

        ui_system.buffer(vec4(4.0f));
        ui_system.position(axiom::position_mode::TOP_LEFT);
        axiom::row_widget::insert();
        axiom::text_widget::insert("", axiom::text_alignment::LEFT, true,
            [](std::string str) -> std::string {
                int base = number_base;

                static double time = 0.0f;
                static double freq = 1.0f;
                static uint32_t num_frames = 0;

                time += axiom::ecs.delta_time;
                ++num_frames;

                if(time > freq || str == "") {
                    axiom::physics_system3d& physics_system = axiom::ecs.get_system<axiom::physics_system3d>();

                    double fps = double(num_frames) / time;
                    time = 0.0;
                    num_frames = 0;

                    std::string string;

                    std::vector<std::string> order;
                    std::unordered_map<std::string, double> times;
                    double total_time = 0.0f;

                    for(Profiler_Frame& frame : physics_system.profiler.prev_frames) {
                        for(Profiler_Entry& entry : frame.steps) {
                            if(times.contains(entry.name)) {
                                times[entry.name] += entry.time;
                            } else {
                                order.push_back(entry.name);
                                times[entry.name] = entry.time;
                            }
                            total_time += entry.time;
                        }
                    }

                    for(std::string name : order) {
                        double time = times[name];
                        double percent = time / total_time * (base * base);
                        double avg = time / double(axiom::ecs.profiler.num_frames) * 1000.0f;

                        std::string entry_str = "\\b" + name + ": \\r" + axiom::to_base(avg, number_base, 2) + "ms (" + axiom::to_base(percent, number_base, 2) + "%)\n";
                        
                        string += entry_str;
                    }

                    return string;
                } else {
                    return str;
                }
            }
        );
    };
    */

    // start

    ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 0.0f));

    auto mfunc = [timer = 0.0](axiom::menu_widget& widget) {

    };

    std::shared_ptr<axiom::menu_node> node(new axiom::menu_node{
        "",
        {
            axiom::menu_node("OPTION A"),
            axiom::menu_node("OPTION B", {
                axiom::menu_node("OPTION E"),
                axiom::menu_node("OPTION F"),
                axiom::menu_node("OPTION G"),
                axiom::menu_node("OPTION H", {
                    axiom::menu_node("OPTION E"),
                    axiom::menu_node("OPTION F"),
                    axiom::menu_node("OPTION G"),
                    axiom::menu_node("OPTION H"),
                    axiom::menu_node("OPTION I"),
                }),
                axiom::menu_node("OPTION I"),
            }),
            axiom::menu_node("OPTION C"),
            axiom::menu_node("OPTION D", {
                axiom::menu_node("OPTION E"),
                axiom::menu_node("OPTION F"),
                axiom::menu_node("OPTION G"),
                axiom::menu_node("OPTION H"),
                axiom::menu_node("OPTION I"),
            }),
        }
    });
    
    auto menu_func = [&mfunc, w = axiom::NULL_WIDGET, node](axiom::button_widget& widget) mutable {
        auto& ui_system = axiom::ecs.get_system<axiom::ui_system>();

        bool pressed_siblings = false;
        bool hovered_siblings = false;
        auto& parent = ui_system.widgets[widget.parent];
        for(uint64_t sibling : parent->children) {
            if(sibling != widget.self) {
                axiom::button_widget* button;
                if(button = dynamic_cast<axiom::button_widget*>(ui_system.widgets[sibling].get())) {
                    if(button->pressed) {
                        pressed_siblings = true;
                    }
                    
                    if(button->hovered) {
                        hovered_siblings = true;
                    }
                }
            }
        }

        bool click_off = false;
        if(widget.pressed) click_off = true;

        if(render_system.ctx->pressed_buttons.contains(axiom::input_code::MOUSE_LEFT) && w != axiom::NULL_WIDGET) {
            click_off = true;

            std::vector<uint64_t> cpath;
            uint64_t current = w;
            while(true) {
                cpath.push_back(current);

                auto& cw = axiom::ecs.get_system<axiom::ui_system>().widgets[current];
                if(cw->children.size()) current = cw->children[0];
                else break;
            }
            
            for(uint64_t c : cpath) {
                auto* cw = (axiom::menu_widget*)axiom::ecs.get_system<axiom::ui_system>().widgets[c].get();

                if(axiom::includes(render_system.ctx->cursor_pos, vec4(cw->position, cw->position + cw->size))) {
                    axiom::menu_node* rootn = cw->root;
                    for(uint32_t p : cw->path) {
                        rootn = &rootn->children[p];
                    }

                    if(rootn->children[cw->hovered].children.size()) {
                        click_off = false;
                        break;
                    }
                }
            }
        }

        bool is_null = w == axiom::NULL_WIDGET;
        if((widget.pressed || (pressed_siblings && widget.hovered)) && is_null) {
            axiom::ecs.get_system<axiom::ui_system>().click_capture = widget.self; // FLAG
            w = axiom::menu_widget::insert(widget.position + vec2(0.0f, -64.0f), 0.6f, axiom::color_editor * 0.9f, 160, 16, 64, node.get(), {});
        }

        if((hovered_siblings || click_off) && !is_null) {
            if(w != axiom::NULL_WIDGET) {
                std::vector<uint64_t> erase;
                uint64_t current = w;
                while(true) {
                    erase.push_back(current);

                    auto& cw = axiom::ecs.get_system<axiom::ui_system>().widgets[current];
                    if(cw->children.size()) current = cw->children[0];
                    else break;
                }

                for(uint64_t c : erase) {
                    axiom::ecs.get_system<axiom::ui_system>().delete_buffer.push_back(c);
                }

                widget.children.clear();
                w = axiom::NULL_WIDGET;
            }
        }

        if(w != axiom::NULL_WIDGET) {
            widget.pressed = true;
        }
    };

    axiom::screen_widget::insert("Axiom", axiom::color_magenta, render_system.ctx);

    ui_system.input_attach(1);

    ui_system.position(axiom::position_mode::CENTER_LEFT);
    axiom::row_widget::insert();
    axiom::button_widget::insert(vec2(30, 16), axiom::color_editor, "File", menu_func);

    axiom::button_widget::insert(vec2(30, 16), axiom::color_editor, "Edit", menu_func);

    axiom::button_widget::insert(vec2(60, 16), axiom::color_editor, "Selection", menu_func);

    axiom::button_widget::insert(vec2(30, 16), axiom::color_editor, "View", menu_func);

    ui_system.input_step(2);

    ui_system.input_attach(0);
    ui_system.position(axiom::position_mode::TOP_LEFT);

    axiom::split_widget::insert(axiom::layout_mode::COLUMN, {axiom::panel_constraint(1.0f / 16.0f, axiom::panel_mode::SIZE), axiom::panel_constraint(15.0f / 16.0f)});
    
    ui_system.buffer(vec4(4.0f));
    axiom::panel_widget::insert();
    axiom::column_widget::insert();
    /**/
    axiom::text_widget::insert("", axiom::text_alignment::LEFT, true,
        [](std::string str) -> std::string {
            axiom::transform3d& cam_tf = axiom::ecs.get_component<axiom::transform3d>(camera);

            std::string pos_x = "\\cE55";
            std::string pos_y = "\\c5E5";
            std::string pos_z = "\\c55E";
            std::string neg_x = "\\c5EE";
            std::string neg_y = "\\cE5E";
            std::string neg_z = "\\cEE5";
            std::string white = "\\cFFF";

            std::string a = (cam_tf.position.x >= 0) ? pos_x : neg_x;
            std::string b = (cam_tf.position.y >= 0) ? pos_y : neg_y;
            std::string c = (cam_tf.position.z >= 0) ? pos_z : neg_z;

            str = "\\bPOSITION: \\r\\h" + a + axiom::to_base(cam_tf.position.x, number_base, 2) + " " + b + axiom::to_base(cam_tf.position.y, number_base, 2) + " " + c + axiom::to_base(cam_tf.position.z, number_base, 2) + white;
            return str;
        }
    );
    axiom::text_widget::insert("", axiom::text_alignment::LEFT, true,
        [](std::string str) -> std::string {
            axiom::transform3d& cam_tf = axiom::ecs.get_component<axiom::transform3d>(camera);

            vec3 dir = cam_tf.orientation * vec3(0.0f, 0.0f, -1.0f);

            std::string pos_x = "\\cE55";
            std::string pos_y = "\\c5E5";
            std::string pos_z = "\\c55E";
            std::string neg_x = "\\c5EE";
            std::string neg_y = "\\cE5E";
            std::string neg_z = "\\cEE5";
            std::string white = "\\cFFF";

            std::string a = (abs(dir.x) > abs(dir.y) && abs(dir.x) > abs(dir.z)) ? ((dir.x > 0.0) ? pos_x : neg_x) : white;
            std::string b = (abs(dir.y) > abs(dir.x) && abs(dir.y) > abs(dir.z)) ? ((dir.y > 0.0) ? pos_y : neg_y) : white;
            std::string c = (abs(dir.z) > abs(dir.x) && abs(dir.z) > abs(dir.y)) ? ((dir.z > 0.0) ? pos_z : neg_z) : white;

            str = "\\bDIRECTION: \\r\\h" + a + axiom::to_base(dir.x, number_base, 2) + " " + b + axiom::to_base(dir.y, number_base, 2) + " " + c + axiom::to_base(dir.z, number_base, 2);
            return str;
        }
    );
    axiom::text_widget::insert("\\bFPS: ", axiom::text_alignment::LEFT, true,
        [](std::string str) -> std::string {
            static double time = 0.0f;
            static double freq = 1.0f;
            static uint32_t num_frames = 0;

            time += axiom::ecs.delta_time;
            ++num_frames;

            if(time > freq) {
                double fps = double(num_frames) / time;
                time = 0.0;
                num_frames = 0;

                return "\\bFPS: \\r" + axiom::to_base(float(fps), number_base, 3);
            } else {
                return str;
            }
        }
    );

    ui_system.input_step();

    ui_system.position(axiom::position_mode::TOP_RIGHT);
    ui_system.buffer(vec4(4.0f));
    axiom::row_widget::insert();
    /*
    axiom::button_widget::insert(vec2(30, 30), axiom::color_physics, "", 
    [](axiom::button_widget& self) {
        if(self.pressed) {
            if(is.camera_mode == CAMERA_FREECAM) {
                // set to player
                is.set_camera_mode(CAMERA_PLAYER);
            } else if(is.camera_mode == CAMERA_PLAYER) {
                // set to freecam
                is.set_camera_mode(CAMERA_FREECAM);
            }
        }

        if(is.camera_mode == CAMERA_PLAYER) self.icon = vec4(38.0f, 6.0f, 10.0f, 10.0f);
        else if(is.camera_mode == CAMERA_FREECAM) self.icon = vec4(28.0f, 6.0f, 10.0f, 10.0f);
        self.icon_size = vec2(10.0f);
    });
    */
    axiom::button_widget::insert(vec2(30, 30), axiom::color_physics, "", 
    [](axiom::button_widget& self) {
        axiom::physics_system3d& system = axiom::ecs.get_system<axiom::physics_system3d>();
        if(self.pressed) system.sim_active = !system.sim_active;

        if(system.sim_active) {
            self.icon = vec4(54.0f, 24.0f, 10.0f, 10.0f);
        } else {
            self.icon = vec4(54.0f, 14.0f, 10.0f, 10.0f);
        }
        self.icon_size = vec2(10.0f);
    });
    axiom::button_widget::insert(vec2(30, 30), axiom::color_physics, "", 
    [](axiom::button_widget& self) {
        if(self.pressed) {
            axiom::physics_system3d& system = axiom::ecs.get_system<axiom::physics_system3d>();
            if(!system.sim_active) {
                system.physics_loop();
            }
        }

        self.icon = vec4(44.0f, 54.0f, 10.0f, 10.0f);
        self.icon_size = vec2(10.0f);
    });
    ui_system.input_step();
    ui_system.input_step();

    axiom::split_widget::insert(axiom::layout_mode::ROW, {axiom::panel_constraint(0.5f, axiom::panel_mode::SIZE), axiom::panel_constraint(0.5f)});

    axiom::split_widget::insert(axiom::layout_mode::COLUMN, {axiom::panel_constraint(0.5f), axiom::panel_constraint(0.5f)});
    ui_system.buffer(vec4(0.0f));

    axiom::panel_widget::insert();
    axiom::tab_widget::insert(20.0f, 0.0f, {
        axiom::tab("Settings", 80.0f, axiom::color_editor, func_settings), 
        axiom::tab("Selection", 80.0f, axiom::color_physics, func_selection), 
        axiom::tab("Lua", 80.0f, axiom::color_lua, func_lua), 
    });
    ui_system.input_step();
    
    axiom::panel_widget::insert();
    /*
    axiom::tab_widget::insert(20.0f, 0.0f, {
        axiom::tab("Global", 80.0f, axiom::color_editor, func_global), 
        axiom::tab("Physics", 80.0f, axiom::color_physics, func_physics), 
    });
    */

    ui_system.input_step();
    ui_system.input_step();

    axiom::panel_widget::insert();
    ui_system.buffer(vec4(0.0f));
    //axiom::render_widget::insert();

    ui_system.input_step();
}