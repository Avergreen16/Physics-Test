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
    addie::network ai({14 * 14 * 16, 100, 10}, 0x3F4);
    ai.players.resize(1);
    ai.players[0].initialize(ivec3(28, 28, 16), 2);
    ai.clayers.resize(3);
    ai.clayers[0].initialize(ivec3(28, 28, 1), ivec3(28, 28, 8), ivec2(5), rand.next());
    ai.clayers[1].initialize(ivec3(28, 28, 8), ivec3(28, 28, 16), ivec2(3), rand.next());
    ai.clayers[2].initialize(ivec3(28, 28, 16), ivec3(28, 28, 16), ivec2(3), rand.next());

    if(argc > 2) ai.read_params(argv[2]);

    std::vector<uint8_t> bytes;

    //

    ivec2 image_size;
    auto images = addie::decode_idx3("external/addie/res/train-images.idx3-ubyte", image_size);
    auto labels = addie::decode_idx1("external/addie/res/train-labels.idx1-ubyte");
    for(int i = 0; i < images.size(); ++i) {
        std::vector<float> image(images[i].size());
        for(int j = 0; j < images[i].size(); ++j) {
            image[j] = float(images[i][j]) / 0xFF;
        }

        uint8_t label = labels[i]; 
        std::vector<float> outputs(10, 0.0f);
        outputs[label] = 1.0f;

        training_inputs.push_back(image);
        training_outputs.push_back(outputs);
    }

    images = addie::decode_idx3("external/addie/res/test-images.idx3-ubyte", image_size);
    labels = addie::decode_idx1("external/addie/res/test-labels.idx1-ubyte");
    for(int i = 0; i < images.size(); ++i) {
        std::vector<float> image(images[i].size());
        for(int j = 0; j < images[i].size(); ++j) {
            image[j] = float(images[i][j]) / 0xFF;
        }


        uint8_t label = labels[i]; 
        std::vector<float> outputs(10, 0.0f);
        outputs[label] = 1.0f;

        test_inputs.push_back(image);
        test_outputs.push_back(outputs);
    }

    //std::cout << "ERROR: " << ai.compute_error(test_inputs, test_outputs).x << std::endl;
    
    uint batch_size = 32;

    std::vector<uint> permutation_base(training_inputs.size());
    std::iota(permutation_base.begin(), permutation_base.end(), 0);
    std::vector<uint> permutation_base2(permutation_base.begin(), permutation_base.begin() + test_inputs.size());

    //

    double start_time = addie::get_time();
    
    float frac = 0.0f;
    for(int k = 0; k < num_steps; ++k) {
        if(interrupt) break;

        std::vector<std::vector<float>> batch_inputs(batch_size);
        std::vector<std::vector<float>> batch_outputs(batch_size);

        //

        std::vector<uint> permutation = permute(permutation_base, rand, batch_size);
        for(int i = 0; i < batch_size; ++i) {
            uint j = permutation[i];

            batch_inputs[i] = training_inputs[j]; 
            batch_outputs[i] = training_outputs[j];
        }

        ai.learn(batch_inputs, batch_outputs, 0.02f);

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
            std::vector<std::vector<float>> batch_inputs(2000);
            std::vector<std::vector<float>> batch_outputs(2000);
            
            std::vector<uint> permutation = permute(permutation_base2, rand, 2000);
            for(int i = 0; i < 2000; ++i) {
                uint j = permutation[i];

                batch_inputs[i] = test_inputs[j]; 
                batch_outputs[i] = test_outputs[j];
            }
            
            ss << "ERROR: " << ai.compute_error(batch_inputs, batch_outputs).x;
        }

        frac = new_frac;

        if(ss.str().size()) std::cout << ss.str() << std::endl;
    }
    
    //

    ai.write_params("output/addie_params" + std::to_string(addie::get_timestamp()) + ".bin");
}