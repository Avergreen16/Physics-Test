#include <include/addie.hpp>
#include <numeric>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

vec4 red_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
vec4 blue_color = vec4(0.0f, 0.0f, 1.0f, 1.0f);

std::vector<std::vector<float>> training_inputs;
std::vector<std::vector<float>> training_outputs;

std::vector<float> function(vec2 point) {
    float a = 0.3f - length(point - vec2(-0.5f, 0.0f));
    float b = 0.3f - length(point - vec2(0.5f, 0.0f));
    float c = 0.3f - length(point - vec2(0.0f, -0.5f));
    float d = 0.3f - length(point - vec2(0.0f, 0.5f));

    if(a > 0.0f) return {0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    if(b > 0.0f) return {0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    if(c > 0.0f) return {0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    if(d > 0.0f) return {0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    return {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
}

int main() {
    addie::random32 rand(0xF01D);
    addie::network ai({2, 100, 100, 200, 5}, 0xF01D);
    std::vector<uint8_t> bytes;

    ivec2 image_size;
    auto images = addie::decode_idx3("external/addie/res/train-images.idx3-ubyte", image_size);
    auto labels = addie::decode_idx1("external/addie/res/train-labels.idx1-ubyte");

    ivec2 size = ivec2(512, 512);
    vec4 range = vec4(-1.0f, -1.0f, 1.0f, 1.0f);
    for(int y = 0; y < size.y; ++y) {
        for(int x = 0; x < size.x; ++x) {
            vec2 pos = (vec2(x, y) + 0.5f) / vec2(size);
            pos = range.xy() + pos * (range.zw() - range.xy());

            std::vector<float> output = function(pos);
            vec4 color = output[0] * vec4(1.0f, 0.0f, 0.0f, 1.0f) + output[1] * vec4(1.0f, 1.0f, 0.0f, 1.0f) + output[2] * vec4(0.0f, 1.0f, 0.0f, 1.0f) + output[3] * vec4(0.0f, 1.0f, 1.0f, 1.0f) + output[4] * vec4(0.0f, 0.0f, 1.0f, 1.0f);

            bytes.push_back(color.x * 0xFF);
            bytes.push_back(color.y * 0xFF);
            bytes.push_back(color.z * 0xFF);
            bytes.push_back(color.w * 0xFF);
        }
    }

    stbi_flip_vertically_on_write(1);
    stbi_write_png("output/addie_target.png", size.x, size.y, 4, bytes.data(), size.x * 4);

    //

    uint num_steps = 100000;
    training_inputs.clear();
    training_outputs.clear();

    ivec2 grid = ivec2(128, 128);
    for(int x = 0; x < grid.x; ++x) {
        for(int y = 0; y < grid.y; ++y) { 
            vec2 point = vec2(x, y) / vec2(grid) * (range.zw() - range.xy()) + range.xy();
            point += vec2(0.5f, 0.5f) / vec2(grid);//vec2(rand(), rand()) / vec2(grid);

            training_outputs.push_back({function(point)});
            training_inputs.push_back({point.x, point.y});
        }
    }
    
    vec2 error = ai.compute_error(training_inputs, training_outputs);
    std::cout << "ERROR: " << error.x << " LOSS: " << error.y << "\n";

    std::vector<uint> permutation_base(grid.x * grid.y);
    std::iota(permutation_base.begin(), permutation_base.end(), 0);

    double start_time = addie::get_time();

    uint batch_size = 64;
    
    float frac = 0.0f;
    for(int k = 0; k < num_steps; ++k) {
        std::vector<std::vector<float>> batch_inputs(glm::min(uint(permutation_base.size()), batch_size));
        std::vector<std::vector<float>> batch_outputs(glm::min(uint(permutation_base.size()), batch_size));
        std::vector<uint> permutation = permutation_base;

        for(int i = 0; i < glm::min(uint(permutation_base.size() - 1), batch_size); ++i) {
            uint j = i + rand.next() % (int(permutation_base.size() - 1) - i);

            std::swap(permutation[i], permutation[j]);
        }
        

        for(int i = 0; i < glm::min(uint(permutation_base.size()), batch_size); ++i) {
            uint j = permutation[i];

            batch_inputs[i] = training_inputs[j]; 
            batch_outputs[i] = training_outputs[j];
        }

        ai.learn(batch_inputs, batch_outputs, 0.01f);

        //

        float new_frac = float(k + 1) / num_steps;


        if(floor(new_frac * 100) != floor(frac * 100)) {
            frac = new_frac;

            double current_time = addie::get_time();
            double elapsed = current_time - start_time;
            double time_left = elapsed * ((1.0f - frac) / frac);

            std::cout << "\nPROGRESS: " << int(new_frac * 100) << std::dec << "%\n";
            //std::cout << "\nPROGRESS: " << std::uppercase << std::hex << int(new_frac * 4096) << std::nouppercase << std::dec << "%\n";
            std::cout << "elapsed time: " << elapsed << " time left: " << time_left << "\n";

            vec2 error = ai.compute_error(training_inputs, training_outputs);
            std::cout << "ERROR: " << error.x << " LOSS: " << error.y << "\n";
        }
    }

    float total_error = 0.0f;
    float accum = 0.0f;

    std::vector<std::vector<float>> outputs(size.x * size.y);
    for(int x = 0; x < size.x; ++x) {
        for(int y = 0; y < size.y; ++y) {
            vec2 pos = (vec2(x, y) + 0.5f) / vec2(size);
            pos = range.xy() + pos * (range.zw() - range.xy());

            std::vector<float> inputs = {pos.x, pos.y};
            std::vector<float> target = function(pos);

            std::vector<float> o = ai.sample(inputs);

            outputs[y * size.x + x] = {o};
            
            for(int i = 0; i < target.size(); ++i) total_error += abs(target[i] - o[i]);
            accum += target.size();
        }
    }
    
    std::cout << "FINISH ERROR: " << total_error / accum << "\n";

    //

    bytes.clear();

    for(int y = 0; y < size.y; ++y) {
        for(int x = 0; x < size.x; ++x) {
            int i = y * size.x + x;

            auto& output = outputs[i];
            vec4 color = output[0] * vec4(1.0f, 0.0f, 0.0f, 1.0f) + output[1] * vec4(1.0f, 1.0f, 0.0f, 1.0f) + output[2] * vec4(0.0f, 1.0f, 0.0f, 1.0f) + output[3] * vec4(0.0f, 1.0f, 1.0f, 1.0f) + output[4] * vec4(0.0f, 0.0f, 1.0f, 1.0f);

            bytes.push_back(color.x * 0xFF);
            bytes.push_back(color.y * 0xFF);
            bytes.push_back(color.z * 0xFF);
            bytes.push_back(color.w * 0xFF);
        }
    }

    stbi_flip_vertically_on_write(1);
    stbi_write_png("output/addie.png", size.x, size.y, 4, bytes.data(), size.x * 4);
}