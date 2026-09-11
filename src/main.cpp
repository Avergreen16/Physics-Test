#include <include/core.hpp>
#include <include/render.hpp>
#include <include/ui.hpp>

struct ttf_point {
    vec2 point;
    bool curve = false;
};

struct ttf_bezier {
    uint a;
    uint b;
    uint c;
};

struct ttf_contour {
    std::vector<ttf_point> points;
    std::vector<ttf_bezier> beziers;
};

struct ttf_glyph {
    uint glyph_id;
    uint codepoint = 0xFFFFFFFF;
    uint glyph_address;

    ivec4 bounding_box;

    std::vector<ttf_contour> contours;
    std::vector<byte> bytecode_glyf;
};

struct ttf_font {
    std::map<uint, ttf_glyph> glyphs;
    ivec4 bounding_box;
    uint base_unit;

    std::vector<byte> bytecode_fpgm;
    std::vector<byte> bytecode_prep;
};

ttf_font font;
uint glyph_index = 0;
ivec2 glyph_size = ivec2(16, 16);

void process_ttf(std::string filepath) {
    axiom::binary_asset bin = axiom::binary_asset::load(filepath);
    uint cursor = 0;

    
    auto read_byte = [&bin, &cursor]() {
        byte ret = bin.data[cursor];
        ++cursor;
        return ret;    
    };
    
    auto read_short_be = [&bin, &cursor]() {
        byte ret_a = bin.data[cursor];
        byte ret_b = bin.data[cursor + 1];
        cursor += 2;

        uint16_t ret = (uint16_t(ret_a) << 8) | uint16_t(ret_b);
        int16_t r;
        memcpy(&r, &ret, 2);
        return r;    
    };

    auto read_ushort_be = [&bin, &cursor]() {
        byte ret_a = bin.data[cursor];
        byte ret_b = bin.data[cursor + 1];
        cursor += 2;

        return (uint16_t(ret_a) << 8) | uint16_t(ret_b);    
    };

    auto read_uint_be = [&bin, &cursor]() {
        byte ret_a = bin.data[cursor];
        byte ret_b = bin.data[cursor + 1];
        byte ret_c = bin.data[cursor + 2];
        byte ret_d = bin.data[cursor + 3];
        cursor += 4;

        return (uint32_t(ret_a) << 24) | (uint32_t(ret_b) << 16) | (uint32_t(ret_c) << 8) | uint32_t(ret_d);
    };
    
    auto read_ulong_be = [&bin, &cursor]() {
        byte ret_a = bin.data[cursor];
        byte ret_b = bin.data[cursor + 1];
        byte ret_c = bin.data[cursor + 2];
        byte ret_d = bin.data[cursor + 3];
        byte ret_e = bin.data[cursor + 4];
        byte ret_f = bin.data[cursor + 5];
        byte ret_g = bin.data[cursor + 6];
        byte ret_h = bin.data[cursor + 7];
        cursor += 8;

        return (uint64_t(ret_a) << 56) | (uint64_t(ret_b) << 48) | (uint64_t(ret_c) << 40) | (uint64_t(ret_d) << 32) |
            (uint64_t(ret_e) << 24) | (uint64_t(ret_f) << 16) | (uint64_t(ret_g) << 8) | uint64_t(ret_h);
    };

    //

    uint32_t version = read_uint_be();
    uint16_t num_tables = read_ushort_be();

    struct ttf_table {
        uint32_t checksum;
        uint32_t offset;
        uint32_t length;
    };

    std::unordered_map<std::string, ttf_table> tables;

    for(int i = 0; i < num_tables; ++i) {
        cursor = 12 + i * 16;

        std::string tag;
        
        tag += read_byte();
        tag += read_byte();
        tag += read_byte();
        tag += read_byte();

        uint32_t checksum = read_uint_be();
        uint32_t offset = read_uint_be();
        uint32_t length = read_uint_be();

        std::cout << "tag: " << tag << " | " << 
            "checksum: " << checksum << " | " <<
            "offset: " << offset << " | " << 
            "length: " << length << std::endl;

        ttf_table table{
            .checksum = checksum,
            .offset = offset,
            .length = length
        };

        tables.emplace(tag, table);
    }

    //

    int16_t loca_format;

    {
        ttf_table table = tables["head"];

        cursor = table.offset;

        uint16_t major_version = read_ushort_be();
        uint16_t minor_version = read_ushort_be();
        uint32_t revision = read_uint_be();

        uint32_t checksum = read_uint_be();
        uint32_t magic_number = read_uint_be();

        uint16_t flags = read_ushort_be();
        uint16_t units_per_em = read_ushort_be();

        font.base_unit = units_per_em;
        std::cout << font.base_unit << "\n";

        int64_t created_timestamp = read_ulong_be();
        int64_t modified_timestamp = read_ulong_be();

        int16_t xmin = read_short_be();
        int16_t ymin = read_short_be();
        int16_t xmax = read_short_be();
        int16_t ymax = read_short_be();

        font.bounding_box = {xmin, ymin, xmax, ymax};

        std::cout << "BOUNDING BOX " << font.bounding_box << "\n";
        
        uint16_t mac_style = read_ushort_be();

        uint16_t smallest_readable_size = read_ushort_be();
        std::cout << "SMALLEST SIZE " << smallest_readable_size << "\n";
        int16_t font_direction = read_short_be();
        loca_format = read_short_be(); // <- what we want
        int16_t glyph_format = read_short_be();
    }

    uint16_t num_glyphs;

    {
        ttf_table table = tables["maxp"];

        cursor = table.offset;

        uint32_t version = read_uint_be();

        if(version = 0x00005000) {
            num_glyphs = read_ushort_be();
        } else if(version = 0x00010000) {
            num_glyphs = read_ushort_be();

            uint16_t max_points = read_ushort_be();
            uint16_t max_contours = read_ushort_be();
            uint16_t max_composite_points = read_ushort_be();
            uint16_t max_composite_contours = read_ushort_be();
            uint16_t max_zones = read_ushort_be();
            uint16_t max_twilight_points = read_ushort_be();
            uint16_t max_storage = read_ushort_be();
            uint16_t max_function_defs = read_ushort_be();
            uint16_t max_instruction_devs = read_ushort_be();
            uint16_t max_stack_elements = read_ushort_be();
            uint16_t max_size_of_instructions = read_ushort_be();
            uint16_t max_component_elements = read_ushort_be();
            uint16_t max_component_depth = read_ushort_be();
        }
    }

    {
        ttf_table table = tables["cmap"];

        cursor = table.offset;

        uint16_t version = read_ushort_be(); // always 0
        uint16_t num_tables = read_ushort_be();

        uint prev_offset = cursor;
        for(int i = 0; i < num_tables; ++i) {
            cursor = prev_offset;
            uint16_t platform = read_ushort_be();
            uint16_t encoding = read_ushort_be();
            uint32_t offset = read_uint_be();

            prev_offset = cursor;

            if(platform == 0) {
                cursor = table.offset + offset;

                uint16_t format = read_ushort_be();
                uint16_t length = read_ushort_be();
                uint16_t language = read_ushort_be();
                uint16_t seg_count_x2 = read_ushort_be();
                uint16_t search_range = read_ushort_be();
                uint16_t entry_selector = read_ushort_be();
                uint16_t range_shift = read_ushort_be();

                //

                uint seg_count = seg_count_x2 / 2.0f;

                // end code is an array of seg_count of uint16_t
                
                std::vector<uint16_t> end_code;
                std::vector<uint16_t> start_code;
                std::vector<int16_t> id_delta;
                std::vector<uint16_t> id_range_offset;
                std::vector<uint32_t> addresses;

                end_code.reserve(seg_count);
                start_code.reserve(seg_count);
                id_delta.reserve(seg_count);
                id_range_offset.reserve(seg_count);

                for(int i = 0; i < seg_count; ++i) {
                    end_code.push_back(read_ushort_be());
                }

                read_ushort_be(); // pad
                
                for(int i = 0; i < seg_count; ++i) {
                    start_code.push_back(read_ushort_be());
                }
                for(int i = 0; i < seg_count; ++i) {
                    id_delta.push_back(read_short_be());
                }
                for(int i = 0; i < seg_count; ++i) {
                    addresses.push_back(cursor);
                    id_range_offset.push_back(read_ushort_be());
                }

                std::cout << seg_count << " SEGS\n";

                //

                uint start_cursor = cursor;

                for(int i = 0; i < seg_count; ++i) {
                    uint16_t start = start_code[i];
                    uint16_t end = end_code[i];

                    std::cout << start << " " << end << "\n";

                    for(int j = start; j <= end; ++j) {
                        uint glyph_id;
                        if(id_range_offset[i] == 0) {
                            glyph_id = j + id_delta[i];
                        } else {
                            uint address = addresses[i];
                            address += int(id_range_offset[i]) + (j - int(start)) * 2;
                            cursor = address;

                            glyph_id = read_ushort_be();

                            if(glyph_id != 0) glyph_id += id_delta[i];
                        }
                        glyph_id &= 0xFFFF;
                        
                        ttf_glyph glyph;
                        glyph.glyph_id = glyph_id;
                        glyph.codepoint = j;

                        font.glyphs.emplace(glyph_id, glyph);
                    }
                }
            }
        }
    }

    {
        ttf_table table = tables["loca"];

        cursor = table.offset;

        //std::cout << "FORMAT " << loca_format << "\n";

        if(loca_format == 0) {
            for(int i = 0; i < num_glyphs; ++i) {
                if(!font.glyphs.contains(i)) {
                    font.glyphs.emplace(i, ttf_glyph());
                }

                uint16_t offset = read_ushort_be();

                uint address = uint(offset) * 2;
                font.glyphs[i].glyph_address = address;
            }
        } else if(loca_format == 1) {
            for(int i = 0; i < num_glyphs; ++i) {
                if(!font.glyphs.contains(i)) {
                    font.glyphs.emplace(i, ttf_glyph());
                }

                uint32_t offset = read_uint_be();

                uint address = uint(offset);
                font.glyphs[i].glyph_address = address;
            }
        }
    }    

    {
        ttf_table table = tables["glyf"];

        for(auto& [k, glyph] : font.glyphs) {
            cursor = table.offset + glyph.glyph_address;

            // read header

            int16_t num_contours = read_short_be();
            int16_t xmin = read_short_be();
            int16_t ymin = read_short_be();
            int16_t xmax = read_short_be();
            int16_t ymax = read_short_be();

            glyph.bounding_box = {xmin, ymin, xmax, ymax};

            std::vector<ttf_point> points;

            if(num_contours > 0) {
                uint start_cursor = cursor;

                cursor += num_contours * 2;
                cursor -= 2;

                uint16_t num_vertices = read_short_be() + 1;

                uint16_t instruction_length = read_ushort_be();
                glyph.bytecode_glyf.reserve(instruction_length);
                for(int i = 0; i < instruction_length; ++i) {
                    glyph.bytecode_glyf.push_back(read_byte());
                }

                std::vector<byte> flags;
                flags.reserve(num_vertices);

                for(int i = 0; i < num_vertices; ++i) {
                    byte flag = read_byte();
                    flags.push_back(flag);

                    if((flag & 0x08) != 0x00) {
                        byte repeat = read_byte();
                        for(int i = 0; i < repeat; ++i) flags.push_back(flag);

                        i += int(repeat);
                    }
                }

                points.resize(num_vertices);

                vec2 prev = vec2(0.0f);

                for(int i = 0; i < num_vertices; ++i) {
                    byte flag = flags[i];

                    // if 0x02 is set, x coord is 1 byte long
                    // and sign is determined by the 0x10 bit

                    // else, 
                    // if the 0x10 bit is set, the x coordinate is the SAME as the previous one,
                    // and no element is added to the list (just copy it)
                    // ... if the 0x10 bit is NOT set (meaning both the 0x02 and 0x10 bits are NOT set), then the x coord is a signed 2-byte integer

                    // the number will be relative to the previous x coordinate
                    
                    if((flag & 0x02) != 0x00) { 
                        if((flag & 0x10) != 0x00) { // x coord is 1 byte long and positive
                            byte b = read_byte();
                            int16_t offset = b;

                            prev.x += offset;
                        } else { // x coord is 1 byte long and negative
                            byte b = read_byte();

                            int16_t offset = b;
                            offset = -offset;

                            prev.x += offset;
                        }
                    } else {
                        if((flag & 0x10) != 0x00) { // x coordinate is the same as previous, duplicate previous value
                            
                        } else { // x coordinate is a signed 2 byte integer
                            int16_t offset = read_short_be();
                            
                            prev.x += offset;
                        }
                    }
                    
                    points[i].point.x = prev.x;
                    if(flag & 0x01) points[i].curve = true;
                }

                //
                
                for(int i = 0; i < num_vertices; ++i) {
                    byte flag = flags[i];

                    // if 0x04 is set, y coord is 1 byte long
                    // and sign is determined by the 0x20 bit

                    // else, 
                    // if the 0x20 bit is set, the y coordinate is the SAME as the previous one,
                    // and no element is added to the list (just copy it)
                    // ... if the 0x20 bit is NOT set (meaning both the 0x04 and 0x20 bits are NOT set), then the y coord is a signed 2-byte integer

                    // the number will be relative to the previous y coordinate

                    // if 0x01 is set, the point is ON the curve, else it is OFF the curve
                    
                    if((flag & 0x04) != 0x00) { 
                        if((flag & 0x20) != 0x00) { // y coord is 1 byte long and positive
                            byte b = read_byte();
                            int16_t offset = b;

                            prev.y += offset;
                        } else { // y coord is 1 byte long and negative
                            byte b = read_byte();

                            int16_t offset = b;
                            offset = -offset;

                            prev.y += offset;
                        }
                    } else {
                        if((flag & 0x20) != 0x00) { // y coordinate is the same as previous, duplicate previous value
                            
                        } else { // y coordinate is a signed 2 byte integer
                            int16_t offset = read_short_be();
                            
                            prev.y += offset;
                        }
                    }
                    
                    points[i].point.y = prev.y;
                    if(flag & 0x01) points[i].curve = true;
                }

                cursor = start_cursor;

                uint16_t start_index = 0;
                for(int i = 0; i < num_contours; ++i) {
                    uint16_t end_index = read_ushort_be();

                    ttf_contour contour;
                    contour.points = std::vector<ttf_point>(points.begin() + start_index, points.begin() + (end_index + 1));

                    for(int i = 0; i < contour.points.size(); ++i) {
                        int ia = i;
                        int ib = (i + 1) % contour.points.size();

                        ttf_point& point_a = contour.points[ia];
                        ttf_point& point_b = contour.points[ib];

                        if(!point_a.curve && !point_b.curve) {
                            ttf_point new_point;
                            new_point.curve = true;
                            new_point.point = (point_a.point + point_b.point) * 0.5f;

                            contour.points.insert(contour.points.begin() + ib, new_point);
                            ++i;
                        }
                    }

                    for(int i = 0; i < contour.points.size(); ++i) {
                        int ia = i;
                        int ib = (i + 1) % contour.points.size();
                        int ic = (i + 2) % contour.points.size();
                        
                        ttf_point& point_a = contour.points[ia];
                        ttf_point& point_b = contour.points[ib];
                        ttf_point& point_c = contour.points[ic];

                        if(point_a.curve && !point_b.curve && point_c.curve) {
                            ttf_bezier bezier;
                            bezier.a = ia;
                            bezier.b = ib;
                            bezier.c = ic;

                            contour.beziers.push_back(bezier);
                        }
                    }

                    uint offset = 0;
                    for(auto& bezier : contour.beziers) {
                        uint pa = bezier.a + offset;
                        uint pb = bezier.b + offset;
                        uint pc = bezier.c + offset;
                        if(bezier.b < 2) pb = bezier.b;
                        if(bezier.c < 2) pc = bezier.c;

                        ttf_point& point_a = contour.points[pa];
                        ttf_point& point_b = contour.points[pb];
                        ttf_point& point_c = contour.points[pc];

                        std::vector<ttf_point> new_points;
                        int num_points = 8;

                        for(int i = 0; i < num_points; ++i) {
                            float frac = float(i + 1) / (num_points + 2);

                            vec2 pa = point_a.point * (1.0f - frac) + point_b.point * frac;
                            vec2 pb = point_b.point * (1.0f - frac) + point_c.point * frac;

                            vec2 point = pa * (1.0f - frac) + pb * frac;

                            new_points.push_back(ttf_point(point, true));
                        }

                        contour.points.erase(contour.points.begin() + pb);
                        contour.points.insert(contour.points.begin() + pb, new_points.begin(), new_points.end());
                        offset += num_points - 1;
                    }

                    glyph.contours.push_back(contour);

                    start_index = end_index + 1;
                }

                //std::cout << "num vertices: " << num_vertices << "\n";
            } else if(num_contours < 0) { // compound glyph
                while(true) {
                    uint16_t flags = read_ushort_be();
                    uint16_t glyph_index = read_ushort_be();

                    // flags:
                    // 0x0001 -> on: arguments are 16 bits, off: arguments are 8 bits
                    // 0x0002 -> on: arguments are signed coordinates, off: arguments are unsigned point indices 
                    // this means that point a on the composite glyph constructed so far, is aligned with point b on this component glyph

                    // 0x0004 -> if 0x0002 is set (so arguments are coordinates), then... on: the coordinate arguments are rounded to the nearest grid line,
                    // grid lines are used for rendering text at small sizes (so the SDF implementation will not use this)
                    // off: arguments are NOT rounded to the nearest grid line

                    // 0x0008 -> the component has a simple scale factor
                    // 0x0020 -> there is another component glyph after this one
                    // 0x0040 -> there are two scale factors, one for the x axis and another for the y axis
                    // 0x0080 -> there is a 2x2 matrix transformation factor that must be applied to the component glyph
                    // 0x0100 -> there are instructions following the end of the last glyph
                    // 0x0200 -> use the metrics (advance width, right/left side bearing) of this component glyph for the entire glyph
                    // 0x0400 -> the components of this compound glyph overlap (remember, each component glyph can itself be a composite glyph)... also this flag is not required to be set
                    // 0x0800 -> the component's offset is transformed by the component's transform (ignored if 0x0002 is not set)
                    // 0x1000 -> the component's offset is NOT transformed by the component's transform (ignored if 0x0002 is not set)
                
                    if((flags & 0x0001) && (flags & 0x0002)) { // arguments are signed 16 bit coords
                        int16_t arg_a = read_short_be();
                        int16_t arg_b = read_short_be();

                        //

                    } else if(!(flags & 0x0001) && (flags & 0x0002)) { // arguments are signed 8 bit coords
                        int8_t arg_a = read_byte();
                        int8_t arg_b = read_byte();

                        //

                    } else if((flags & 0x0001) && !(flags & 0x0002)) { // arguments are unsigned 16 bit indices
                        uint16_t arg_a = read_ushort_be();
                        uint16_t arg_b = read_ushort_be();

                        //

                    } else if(!(flags & 0x0001) && !(flags & 0x0002)) { // arguments are unsigned 8 bit indices
                        byte arg_a = read_byte();
                        byte arg_b = read_byte();

                        //

                    }

                    if(flags & 0x0008) { // simple scale factor
                        uint16_t scale_factor = read_ushort_be();
                    } else if(flags & 0x0040) { // x and y scale factors
                        uint16_t scale_factor_x = read_ushort_be();
                        uint16_t scale_factor_y = read_ushort_be();
                    } else if(flags & 0x0080) { // 2x2 transformation
                        uint16_t nxx = read_ushort_be();
                        uint16_t nxy = read_ushort_be();
                        uint16_t nyx = read_ushort_be();
                        uint16_t nyy = read_ushort_be();
                    }

                    if(!(flags & 0x0020)) { // there is NOT another component glyph after this one
                        if(flags & 0x0100) {
                            uint16_t num_instructions = read_ushort_be();
                            std::vector<byte> instructions;
                            for(uint i = 0; i < num_instructions; ++i) {
                                instructions.push_back(read_byte());
                            }
                        }

                        //step = cursor - offset;
                        break;
                    }
                }
            }
        }
    }

    std::cout << "DONE!!!";
}

/*
if(tag == "glyf") {
    uint step = 0;

    while(true) {
        
    }
}
    */


void render_billboards(uint camera, std::vector<vec3> origins, std::vector<vec4> textures, std::vector<vec4> colors, std::vector<vec2> sizes, ivec2 framebuffer_size, axiom::texture& texture) {
    static axiom::vertices vertices;
    if(!vertices.initialized) vertices.init();

    axiom::transform3d camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera);
    axiom::camera3d& camera_cam = axiom::ecs.get_component<axiom::camera3d>(camera);
    mat4 view = axiom::get_view(camera_cam, camera_transform);
    mat4 proj = axiom::get_proj(camera_cam);

    std::vector<axiom::texture_vertex3d> tvs;

    axiom::transform3d transform;
    transform.position = vec3(0.0f);
    transform.orientation = glm::identity<mat3>();

    mat4 model = axiom::get_model(transform, camera_transform);

    mat4 inv_proj = glm::inverse(proj);

    uint i = 0;
    for(vec3 vvv : origins) {
        vec4 pos = view * model * vec4(vvv, 1.0f);

        //if(pos.z < 0.0f) {
            pos = proj * pos;
            pos /= pos.w;

            vec2 size = sizes[i] / vec2(framebuffer_size);

            vec4 p = vec4(size, pos.z, 1.0f);
            p = inv_proj * p;
            p /= p.w;

            size = glm::abs(p.xy());

            //

            vec4 tex_range = textures[i];
            vec4 color = colors[i];

            //
            std::vector<axiom::texture_vertex3d> vs;
            vs.push_back(axiom::texture_vertex3d(vec3(-1.0f, -1.0f, 0.0f), vec2(0.0f, 0.0f), color, vec3(0.0f)));
            vs.push_back(axiom::texture_vertex3d(vec3(1.0f, -1.0f, 0.0f), vec2(1.0f, 0.0f), color, vec3(0.0f)));
            vs.push_back(axiom::texture_vertex3d(vec3(-1.0f, 1.0f, 0.0f), vec2(0.0f, 1.0f), color, vec3(0.0f)));
            vs.push_back(axiom::texture_vertex3d(vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 1.0f), color, vec3(0.0f)));

            vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

            for(auto& v : vs) {
                v.position = vvv + transpose(mat3(view)) * (v.position * vec3(size, 1.0f));

                v.texture = v.texture * tex_range.zw() + tex_range.xy();
            }

            tvs.insert(tvs.end(), vs.begin(), vs.end());
        //}
        ++i;
    }
    
    transform.orientation = glm::identity<mat3>();
    model = axiom::get_model(transform, camera_transform);

    vertices.vertex_buffer_data(tvs.data(), tvs.size(), sizeof(axiom::texture_vertex3d), GL_STREAM_DRAW);
    vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), 0);
    vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 3);
    vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 5);
    vertices.add_vertex_attribute(3, 3, GL_FLOAT, false, sizeof(axiom::texture_vertex3d), sizeof(float) * 9);

    axiom::shader& texture_shader = axiom::get_shader("texture3d");
    vec3 light_dir = vec3(0.0f, 0.0f, 0.0f);
    
    texture_shader.use();
    texture.bind(0);
    vertices.bind();

    axiom::push_uniform(0, &model);
    axiom::push_uniform(1, &view);
    axiom::push_uniform(2, &proj);
    //glUniform1f(3, msystem.light_contrast);

    vertices.draw_vertices_triangles();
}

void render_lines(uint camera, std::vector<vec3> points, std::vector<vec4> colors) {
    static axiom::vertices vertices;
    if(!vertices.initialized) vertices.init();

    axiom::transform3d camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera);
    axiom::camera3d& camera_cam = axiom::ecs.get_component<axiom::camera3d>(camera);
    mat4 view = axiom::get_view(camera_cam, camera_transform);
    mat4 proj = axiom::get_proj(camera_cam);

    std::vector<axiom::color_vertex3d> cvs;

    axiom::transform3d transform;
    transform.position = vec3(0.0f);
    transform.orientation = glm::identity<mat3>();

    mat4 model = axiom::get_model(transform, camera_transform);

    mat4 inv_proj = glm::inverse(proj);

    uint i = 0;
    for(int i = 0; i < points.size(); ++i) {
        axiom::color_vertex3d vertex;
        vertex.position = points[i];
        vertex.color = colors[i];

        cvs.push_back(vertex);
    }
    
    transform.orientation = glm::identity<mat3>();
    model = axiom::get_model(transform, camera_transform);

    vertices.vertex_buffer_data(cvs.data(), cvs.size(), sizeof(axiom::color_vertex3d), GL_STREAM_DRAW);
    vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), 0);
    vertices.add_vertex_attribute(1, 4, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 3);
    vertices.add_vertex_attribute(2, 3, GL_FLOAT, false, sizeof(axiom::color_vertex3d), sizeof(float) * 7);

    axiom::shader& shader = axiom::get_shader("color3d");
    vec3 light_dir = vec3(0.0f, 0.0f, 0.0f);
    
    shader.use();
    vertices.bind();

    axiom::push_uniform(0, &model);
    axiom::push_uniform(1, &view);
    axiom::push_uniform(2, &proj);
    //glUniform1f(3, msystem.light_contrast);

    vertices.draw_vertices_lines();
}

auto base_render = [](uint camera_entity) {
    axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
    axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);
};

void render_grid(uint camera_entity, axiom::framebuffer& framebuffer) {
    axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
    axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);

    auto& vertices = axiom::get_vertices();
    
    std::vector<vec2> vs = {
        vec2(-1.0f, -1.0f),
        vec2(1.0f, -1.0f),
        vec2(-1.0f, 1.0f),
        vec2(1.0f, 1.0f)
    };

    vs = {vs[0], vs[1], vs[3], vs[0], vs[3], vs[2]};

    vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec2), GL_STATIC_DRAW);
    vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec2), 0);
    
    axiom::transform3d grid_transform;
    grid_transform.position = vec3(0.0f);
    grid_transform.orientation = glm::identity<mat3>();
    
    mat4 model = axiom::get_model(grid_transform, camera_transform);
    mat4 view = axiom::get_view(camera, camera_transform);
    mat4 proj = axiom::get_proj(camera);

    axiom::get_shader("grid3d").use();

    framebuffer.textures[framebuffer.depth_texture].bind(0);
    vertices.bind();

    axiom::push_uniform(0, &model);
    axiom::push_uniform(1, &view);
    axiom::push_uniform(2, &proj);
    axiom::push_uniform(3, vec3(axiom::max_float));

    vertices.draw_vertices_triangles();
}

struct clip_space {
    vec4 range;
    float radius;
    uint parent = 0xFFFFFFFF;
};
static_assert(sizeof(clip_space) == 24);
static_assert(offsetof(clip_space, range) == 0);
static_assert(offsetof(clip_space, radius) == 16);
static_assert(offsetof(clip_space, parent) == 20);

int main(int argc, char** argv) {
    process_ttf("res/Capriola-Regular.ttf");

    axiom::window window(ivec2(256), ivec2(512), 0, "Axiom");

    axiom::render_init(&window);  
    axiom::ui_init(&window);

    // create and initialize camera
    
    uint camera_entity = axiom::ecs.insert_entity();

    axiom::camera3d cam;
    cam.fov = 90.0f;
    cam.near = 0.01f;
    cam.aspect = vec2(1.0f, 1.0f);
    axiom::ecs.insert_component(camera_entity, cam);

    axiom::transform3d transform;
    transform.position = vec3(0.0f, 0.0f, 8.0f);
    transform.orientation = glm::identity<mat3>();
    axiom::ecs.insert_component(camera_entity, transform);

    // create target callback and pass in camera

    auto create_glyph = [](uint index, int units, axiom::texture& texture) {

        auto& glyph = font.glyphs[index];

        auto intersect = [](vec2 a0, vec2 a1, vec2 p0, vec2& p1) {
            vec2 dir_p = vec2(1.0f, 0.0f);

            float a = (p0.y - a0.y) / (a1.y - a0.y);

            p1 = a0 + (a1 - a0) * a;

            return !(a0.y == a1.y) && a >= 0.0f && a < 1.0f; //!(a0.x == a1.x) && 
        };

        vec2 psize = vec2(glyph.bounding_box.zw() - glyph.bounding_box.xy());
        ivec2 size = glm::ceil(psize / float(font.base_unit) * float(units));
        psize = (vec2(size) / float(units)) * float(font.base_unit);

        std::vector<byte> colors(size.x * size.y * 4);
        for(int k = 0; k < size.x * size.y; ++k) {
            vec2 pt = vec2(k % size.x, k / size.x);
            //float w = glm::max(glyph.bounding_box.z - glyph.bounding_box.x, glyph.bounding_box.w - glyph.bounding_box.y) * 0.25f;

            float min_width = glm::min(size.x, size.y);

            uint supersample = 8;

            float frac = 0.0f;

            for(int ii = 0; ii < supersample * supersample; ++ii) {
                vec2 offset = vec2(ii % supersample + 0.5f, ii / supersample + 0.5f) / float(supersample);
                vec2 pt_o = pt + offset;

                pt_o = (pt_o / vec2(size)) * psize + vec2(glyph.bounding_box.xy());

                int count = 0;
                float min_dist = axiom::max_float;
                
                for(int i = 0; i < glyph.contours.size(); ++i) {
                    auto& contour = glyph.contours[i];

                    for(int j = 0; j < contour.points.size(); ++j) {
                        int a = j;
                        int b = (j + 1) % contour.points.size();

                        auto& point_a = contour.points[a];
                        auto& point_b = contour.points[b];

                        vec2 pt_s;

                        bool did_intersect = intersect(point_a.point, point_b.point, pt_o, pt_s);

                        if(pt_s.x < pt_o.x) did_intersect = false;

                        if(did_intersect) {
                            if(point_a.point.y < point_b.point.y) {
                                ++count;
                            } else {
                                --count;
                            }
                        }

                        //

                        /*
                        vec2 a0 = point_a.point;
                        vec2 a1 = point_b.point;

                        vec2 r = normalize(a1 - a0);
                        vec2 rx = vec2(r.y, -r.x);

                        vec2 rel = pt - a0;
                        rel -= rx * dot(rel, rx);

                        float l = dot(r, rel);
                        l = glm::clamp(l, 0.0f, length(a1 - a0));

                        vec2 point = a0 + r * l;

                        float dist = length((pt_o - point) / size);

                        min_dist = glm::min(dist, min_dist);
                        */
                    }
                }
                
                if(count != 0) {
                    frac += 1.0f;
                }
            }

            frac = (1.0f - frac / (supersample * supersample)) * 0xFF;

            colors[k * 4] = frac;
            colors[k * 4 + 1] = frac;
            colors[k * 4 + 2] = frac;
            colors[k * 4 + 3] = 0xFF;

            /*
            float f = min_dist * 255;
            if(count != 0) {
                colors[k * 4] = glm::clamp((int)glm::round(127.5f + f), 0x00, 0xFF);
                colors[k * 4 + 1] = 0x00;
                colors[k * 4 + 2] = 0x00;
                colors[k * 4 + 3] = 0xFF;
            } else {
                colors[k * 4] = glm::clamp((int)glm::round(127.5f - f), 0x00, 0xFF);
                colors[k * 4 + 1] = 0x00;
                colors[k * 4 + 2] = 0x00;
                colors[k * 4 + 3] = 0xFF;
            }
            */
        }

        axiom::texture_asset asset = axiom::texture_asset::load(colors, size, 4);

        texture.load(asset, axiom::texture_format::RGBA8, 0);
        
        return size;
    };
    
    auto target_callback = [&window, camera_entity](axiom::render_target& target) {
        target.framebuffer.bind();
        target.framebuffer.clear(vec4(0.0f, 0.0f, 0.0f, 1.0f));
        
        axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);
        camera.aspect = target.size;

        base_render(camera_entity);

        //
        
        /*
        std::vector<vec3> origins;
        std::vector<vec4> textures;
        std::vector<vec4> colors;
        std::vector<vec2> sizes;

        for(auto& pt : font.glyphs[0].points) {
            origins.push_back(vec3(pt, 5.0f));
            textures.push_back(vec4(48, 96, 5, 5)),
            colors.push_back(vec4(1.0f, 0.25f, 0.25f, 1.0f));
            sizes.push_back(vec2(5.0f, 5.0f));
        }

        render_billboards(camera_entity, origins, textures, colors, sizes, target.size, axiom::get_texture("ui"));
        */
        
        render_grid(camera_entity, target.framebuffer);
    };

    auto widget_callback = [camera_entity, &window](axiom::render_widget* widget) {
        static bool movement_capture = false;
        static float movement_speed = 1.0f;
        static bool cursor_hidden = false;

        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
        axiom::transform3d& camera_transform = axiom::ecs.get_component<axiom::transform3d>(camera_entity);
        axiom::camera3d& camera = axiom::ecs.get_component<axiom::camera3d>(camera_entity);
        
        if(ui_system.click_capture == widget->self) {
            if(movement_capture == false) {
                cursor_hidden = window.cursor_hidden;
                window.disable_cursor();

                movement_capture = true;
            }
        } else {
            if(movement_capture) {
                window.show_cursor();
                if(cursor_hidden) window.hide_cursor();

                movement_capture = false;
            }
        }

        if(ui_system.hover_capture == widget->self) {
            if(ui_system.window->scroll_delta != 0.0f) {
                movement_speed *= pow(2, ui_system.window->scroll_delta * 0.5f);
            }
        }

        if(ui_system.click_capture == widget->self) {
            if(movement_capture) {
                glm::vec3 raw_movement = {0, 0, 0};
                float rotate_value = 0.0f;

                vec3 rotate = vec3(0.0f);
                rotate.x = -ui_system.window->cursor_delta.x;
                rotate.y = -ui_system.window->cursor_delta.y;

                if(ui_system.window->input_map[axiom::input_code::KEY_Q]) {
                    rotate.z -= 1;
                }
                if(ui_system.window->input_map[axiom::input_code::KEY_E]) {
                    rotate.z += 1;
                }

                //

                if(ui_system.window->input_map[axiom::input_code::KEY_A]) {
                    raw_movement.x -= 1;
                }
                if(ui_system.window->input_map[axiom::input_code::KEY_D]) {
                    raw_movement.x += 1;
                } 
                if(ui_system.window->input_map[axiom::input_code::KEY_S]) {
                    raw_movement.z += 1;
                }
                if(ui_system.window->input_map[axiom::input_code::KEY_W]) {
                    raw_movement.z -= 1;
                }
                if(ui_system.window->input_map[axiom::input_code::KEY_SPACE]) {
                    raw_movement.y += 1;
                }
                if(ui_system.window->input_map[axiom::input_code::KEY_LEFT_SHIFT]) {
                    raw_movement.y -= 1;
                }

                //

                float len = length(raw_movement);
                if(len != 0.0f) raw_movement = glm::normalize(raw_movement);
                
                glm::vec3 translation_vec = camera_transform.orientation * raw_movement;
                
                vec3 dir = -camera_transform.orientation[2];
                vec3 u = camera_transform.orientation[1];
                glm::mat3 rotate_y_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 1024) * rotate.y), glm::normalize(glm::cross(u, dir)));
                glm::mat3 rotate_x_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 1024) * rotate.x), u);
                glm::mat3 rotate_z_mat = (mat3)glm::rotate(float(2 * axiom::pi * (1.0 / 128) * rotate.z * (axiom::ecs.delta_time * 60)), dir);

                camera_transform.orientation = rotate_z_mat * rotate_x_mat * rotate_y_mat * camera_transform.orientation;
                camera_transform.position += translation_vec * (float)axiom::ecs.delta_time * movement_speed;
            }
        }
    };

    //

    std::vector<axiom::texture_format> format;
    std::vector<axiom::texture_attachment> attachment;

    axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();

    {
        format = {axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
        attachment = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::COLOR1, axiom::texture_attachment::COLOR2, axiom::texture_attachment::DEPTH};
        static axiom::render_target render_target = axiom::render_target::create(
            target_callback,
            ivec2(512),
            format,
            attachment,
            {}
        );

        axiom::screen_widget::insert("AXIOM", axiom::color_magenta, &window);
        axiom::render_widget::insert(&render_target, 0, widget_callback);
    }

    //

    {
        ui_system.input_reset();
        axiom::window_widget::insert("AXIOM", ivec2(300, 100), ivec2(100), axiom::color_purple);
        axiom::panel_widget::insert();
        axiom::scroll_widget::insert(2.0f, false);

        ui_system.buffer(vec4(6));
        axiom::grid_widget::insert(3);

        ui_system.position(axiom::position_mode::CENTER_LEFT);

        axiom::text_widget::insert("Glyph Index", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(axiom::max_float, 0));
        axiom::slider_widget::insert(vec2(512.0f, 10.0f), 4, axiom::color_magenta, vec2(0, font.glyphs.size()), 1, 0, "", 
            [](axiom::slider_widget& widget) {
                widget.text[0]->string = axiom::to_base(int64_t(widget.current_value), 10);

                glyph_index = widget.current_value;
            }
        );
        
        axiom::text_widget::insert("texture res X", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(axiom::max_float, 0));
        axiom::slider_widget::insert(vec2(128.0f, 10.0f), 4, axiom::color_magenta, vec2(4, 40), 1, 16, "", 
            [](axiom::slider_widget& widget) {
                widget.text[0]->string = axiom::to_base(int64_t(widget.current_value), 10);

                glyph_size.x = widget.current_value;
            }
        );
        
        axiom::text_widget::insert("texture res Y", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(axiom::max_float, 0));
        axiom::slider_widget::insert(vec2(128.0f, 10.0f), 4, axiom::color_magenta, vec2(4, 40), 1, 16, "", 
            [](axiom::slider_widget& widget) {
                widget.text[0]->string = axiom::to_base(int64_t(widget.current_value), 10);

                glyph_size.y = widget.current_value;
            }
        );
    }

    //

    {
        auto target_callback = [&create_glyph, &window, camera_entity](axiom::render_target& target) {
            static axiom::texture texture;
            ivec2 gsize = create_glyph(glyph_index, glyph_size.y, texture);

            target.framebuffer.bind();
            target.framebuffer.clear(vec4(1.0f, 1.0f, 1.0f, 1.0f));
            
            std::vector<vec4> vs = {
                vec4(-1.0f, -1.0f, 0.0f, 0.0f),
                vec4(1.0f, -1.0f, 1.0f, 0.0f),
                vec4(1.0f, 1.0f, 1.0f, 1.0f),
                vec4(-1.0f, -1.0f, 0.0f, 0.0f),
                vec4(1.0f, 1.0f, 1.0f, 1.0f),
                vec4(-1.0f, 1.0f, 0.0f, 1.0f),
            };

            vec2 size = gsize;
            vec2 offset = vec2((ivec2(size)) % 2);
            for(vec4& v : vs) {
                v.x = v.x * size.x + offset.x;
                v.y = v.y * size.y + offset.y;

                std::cout << v.x << "\n";

                v.x /= target.size.x;
                v.y /= target.size.y;
            }

            auto& vertices = axiom::get_vertices();

            vertices.vertex_buffer_data(vs.data(), vs.size(), sizeof(vec4), GL_STREAM_DRAW);
            vertices.add_vertex_attribute(0, 2, GL_FLOAT, false, sizeof(vec4), 0);
            vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(vec4), sizeof(float) * 2);

            texture.bind(0);

            axiom::get_shader("glyph").use();
            vertices.draw_vertices_triangles();
        };

        std::vector<axiom::texture_format> format = {axiom::texture_format::RGBA8};
        std::vector<axiom::texture_attachment> attachment = {axiom::texture_attachment::COLOR0};
        static axiom::render_target target = axiom::render_target::create(
            target_callback,
            ivec2(512),
            format,
            attachment,
            {}
        );

        ui_system.input_reset();
        axiom::window_widget::insert("AXIOM", ivec2(200, 200), ivec2(400, 100), axiom::color_purple);
        axiom::render_widget::insert(&target, 0);
    }

    //
    
    auto main_target_callback = [](axiom::render_target& target) {
        axiom::ui_system& ui_system = axiom::ecs.get_system<axiom::ui_system>();
        auto& vertices = axiom::get_vertices();

        static axiom::storage_buffer storage_buffer;
        if(!storage_buffer.initialized) storage_buffer.init();
        std::vector<clip_space> clip_spaces = {
            clip_space(vec4(0.0f, 0.0f, target.size), 10.0f, 0xFFFFFFFF),
        };
        storage_buffer.buffer_data(clip_spaces.data(), clip_spaces.size() * sizeof(clip_space), GL_STREAM_DRAW);

        //

        vertices.vertex_buffer_data(ui_system.vertices.data(), ui_system.vertices.size(), sizeof(axiom::ui_vertex), GL_STREAM_DRAW);

        vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::ui_vertex), 0);
        vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 3);
        vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 5);
        vertices.add_vertex_attribute(3, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 9);
        vertices.add_vertex_attribute(4, 1, GL_UNSIGNED_INT, false, sizeof(axiom::ui_vertex), sizeof(float) * 13);

        //

        mat3 view_matrix = glm::translate(glm::identity<mat3>(), vec2(-1.0f, -1.0f)) * glm::scale(glm::identity<mat3>(), vec2(2.0f / ui_system.window->size.x, 2.0f / ui_system.window->size.y));
        mat3 trans_matrix = glm::identity<mat3>();

        axiom::get_shader("ui").use();

        axiom::get_texture("font_axiom_default").bind(0);
        axiom::get_texture("ui").bind(1);
        for(int i = 0; i < ui_system.target_textures.size(); ++i) {
            ui_system.target_textures[i]->bind(i + 2);
        }

        storage_buffer.bind(0);

        axiom::push_uniform(0, &view_matrix);
        axiom::push_uniform(1, &trans_matrix);

        vertices.draw_vertices_triangles();
    };
    
    format = {axiom::texture_format::RGBA8, axiom::texture_format::DEPTHF};
    attachment = {axiom::texture_attachment::COLOR0, axiom::texture_attachment::DEPTH};
    axiom::render_target main_target = axiom::render_target::create(
        main_target_callback,
        ivec2(512),
        format,
        attachment,
        {}
    );
    window.attach(&main_target);

    while(!window.should_close) {
        window.poll_events();

        main_target.call();

        axiom::ecs.frame();
    }
}