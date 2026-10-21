#include <include/addie.hpp>
#include <numeric>
#include <csignal>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

vec4 red_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
vec4 blue_color = vec4(0.0f, 0.0f, 1.0f, 1.0f);

std::vector<std::vector<float>> training_inputs;
std::vector<std::vector<float>> training_outputs;
std::vector<std::vector<float>> test_inputs;
std::vector<std::vector<float>> test_outputs;

std::vector<float> function(vec2 point) {
    std::vector<float> outputs(10, 0.0f);
    float angle = std::atan2(point.y, point.x);
    int f = glm::floor(glm::mod(angle / (3.1415927f * 2.0f) * 10.0f, 10.0f));
    outputs[f] = 1.0f;
    
    return outputs;
}

bool interrupt = false;
void handle_interrupt(int) {
    std::cout << "caught SIGINT, outputting current results\n";
    interrupt = true;
}

std::vector<uint> permute(std::vector<uint> base, addie::random32& rand, uint batch_size) {
    for(int i = 0; i < batch_size; ++i) {
        uint j = i + rand.next() % (int(base.size() - 1) - i);

        std::swap(base[i], base[j]);
    }

    return std::vector<uint>(base.begin(), base.begin() + batch_size);
}

int main(int argc, char** argv) {
    std::signal(SIGINT, handle_interrupt);

    uint num_steps;
    if(argc > 1) num_steps = std::stoi(argv[1]);
    else num_steps = 1000;

    //

    addie::random32 rand(0x3F4);
    addie::network ai({2, 400, 400, 400, 10}, 0x3F4);
    if(argc > 2) ai.read_params(argv[2]);

    std::vector<std::vector<float>> training_inputs;
    std::vector<std::vector<float>> training_outputs;

    for(int y = 0; y < 16; ++y) {
        for(int x = 0; x < 16; ++x) {
            vec2 pos = vec2(x + 0.5f, y + 0.5f) / vec2(16.0f) * 2.0f - 1.0f;

            std::vector<float> input = {pos.x, pos.y};
            std::vector<float> output = function(pos);

            training_inputs.push_back(input);
            training_outputs.push_back(output);
        }
    } 

    uint batch_size = 256;

    std::vector<uint> permutation_base(training_inputs.size());
    std::iota(permutation_base.begin(), permutation_base.end(), 0);

    //

    double start_time = addie::get_time();
    
    float frac = 0.0f;
    for(int k = 0; k < num_steps; ++k) {
        if(interrupt) break;

        std::vector<std::vector<float>> batch_inputs(batch_size);
        std::vector<std::vector<float>> batch_outputs(batch_size);

        //

        std::vector<uint> permutation = permutation_base;//permute(permutation_base, rand, batch_size);
        for(int i = 0; i < batch_size; ++i) {
            uint j = permutation[i];

            batch_inputs[i] = training_inputs[j]; 
            batch_outputs[i] = training_outputs[j];
        }

        ai.learn(batch_inputs, batch_outputs, 0.85f, 0.005f);

        //

        std::stringstream ss;
        float new_frac = float(k + 1) / num_steps;


        if(floor(new_frac * 1000) != floor(frac * 1000)) {
            double current_time = addie::get_time();
            double elapsed = current_time - start_time;
            double time_left = elapsed * ((1.0f - frac) / frac);
            ss << "time elapsed: " << elapsed << " time left: " << time_left << " "; 
        }

        if(floor(new_frac * 100) != floor(frac * 100)) {
            //std::vector<std::vector<float>> batch_inputs(2000);
            //std::vector<std::vector<float>> batch_outputs(2000);
            
            /*
            std::vector<uint> permutation = permute(permutation_base, rand, 2000);
            for(int i = 0; i < 2000; ++i) {
                uint j = permutation[i];

                batch_inputs[i] = training_inputs[j]; 
                batch_outputs[i] = training_outputs[j];
            }
            */
            
            ss << "ERROR: " << ai.compute_error(batch_inputs, batch_outputs).x;
        }

        frac = new_frac;

        if(ss.str().size()) std::cout << ss.str() << std::endl;
    }
    
    //

    ai.write_params("output/addie_params" + std::to_string(addie::get_timestamp()) + ".bin");
}