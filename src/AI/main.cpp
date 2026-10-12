#include <include/addie.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

vec4 red_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
vec4 blue_color = vec4(0.0f, 0.0f, 1.0f, 1.0f);

std::vector<std::vector<float>> training_inputs;
std::vector<std::vector<float>> training_outputs;

float function(vec2 point) {
    return ((length(point) - 0.6f + sin((point.x + point.y) * 6.0f) * 0.2f) * 6.0f < 0.0f) ? 1.0f : 0.0f;
}

int main() {
    addie::random32 rand(0x5F);
    addie::network ai({2, 40, 40, 60, 1}, 0xF15);

    uint num_steps = 65536;
    
    {
        float total_error = 0.0f;
        float accum = 0.0f;

        ivec2 grid = ivec2(256);
        for(int x = 0; x < grid.x; ++x) {
            for(int y = 0; y < grid.y; ++y) {  
                vec2 point = (vec2(x, y) + 0.5f) / vec2(grid) * 2.0f - 1.0f;

                std::vector<float> input = {point.x, point.y};

                float target = function(point);
                float output = ai.sample(input)[0];

                float error = abs(target - output);

                total_error += error;
                accum += 1.0f;
            }
        }

        std::cout << "START ERROR: " << total_error / accum << "\n";
    }

    //
    
    training_inputs.clear();
    training_outputs.clear();

    ivec2 grid = ivec2(24, 24);
    for(int x = 0; x < grid.x; ++x) {
        for(int y = 0; y < grid.y; ++y) { 
            vec2 point = vec2(x, y) / vec2(grid) * 2.0f - 1.0f;
            point += vec2(0.5f, 0.5f) / vec2(grid);//vec2(rand(), rand()) / vec2(grid);

            training_outputs.push_back({function(point)});
            training_inputs.push_back({point.x, point.y});
        }
    }
    
    float frac = 0.0f;
    for(int k = 0; k < num_steps; ++k) {
        ai.learn(training_inputs, training_outputs, 0.02f);

        float new_frac = float(k + 1) / num_steps;

        if(floor(new_frac * 100) != floor(frac * 100)) {
            frac = new_frac;
            //std::cout << "PROGRESS: " << new_frac * 100 << "\n";
            std::cout << "ERROR: " << ai.compute_error(training_inputs, training_outputs) << "\n";
        }
    }

    ai.print_gradient(training_inputs, training_outputs);

    //

    float total_error = 0.0f;
    float accum = 0.0f;

    ivec2 size = ivec2(256, 256);
    vec4 range = vec4(-1.0f, -1.0f, 1.0f, 1.0f);

    std::vector<std::vector<float>> outputs(size.x * size.y);
    for(int x = 0; x < size.x; ++x) {
        for(int y = 0; y < size.y; ++y) {
            vec2 pos = (vec2(x, y) + 0.5f) / vec2(size);
            pos = range.xy() + pos * (range.zw() - range.xy());

            std::vector<float> inputs = {pos.x, pos.y};
            float target = function(pos);

            std::vector<float> o = ai.sample(inputs);

            outputs[y * size.x + x] = {o[0]};
            
            float error = abs(target - o[0]);
            total_error += error;
            accum += 1.0f;
        }
    }
    
    std::cout << "FINISH ERROR: " << total_error / accum << "\n";

    //

    std::vector<uint8_t> bytes;

    for(int y = 0; y < size.y; ++y) {
        for(int x = 0; x < size.x; ++x) {
            int i = y * size.x + x;

            float output = outputs[i][0];

            vec4 color = vec4(addie::hsv_color(output, 1.0f, 1.0f), 1.0f);

            bytes.push_back(color.x * 0xFF);
            bytes.push_back(color.y * 0xFF);
            bytes.push_back(color.z * 0xFF);
            bytes.push_back(color.w * 0xFF);
        }
    }

    stbi_flip_vertically_on_write(1);
    stbi_write_png("output/addie.png", size.x, size.y, 4, bytes.data(), size.x * 4);
}