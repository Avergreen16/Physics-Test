#include <include/addie.hpp>
#include <numeric>
#include <csignal>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

vec4 red_color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
vec4 blue_color = vec4(0.0f, 0.0f, 1.0f, 1.0f);

std::vector<Eigen::MatrixXf> training_inputs;
std::vector<Eigen::VectorXf> training_outputs;
std::vector<Eigen::MatrixXf> test_inputs;
std::vector<Eigen::VectorXf> test_outputs;

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

    uint num_steps = 1000;
    uint batch_size = 64;

    float rate = 0.001f;
    float B1 = 0.9f;
    float B2 = 0.999f;
    
    if(argc > 1) num_steps = std::stoi(argv[1]);
    if(argc > 2) batch_size = std::stoi(argv[2]);
    if(argc > 3) B1 = std::stof(argv[3]);
    if(argc > 4) B2 = std::stof(argv[4]);
    if(argc > 5) rate = std::stof(argv[5]);

    //

    addie::random32 rand(0x8F);
    
    addie::network ai({10 * 10 * 32, 240, 160, 10}, 0xEE1);
    ai.pooling_layers.resize(2);
    ai.pooling_layers[0].initialize(ivec3(24, 24, 16), 2);
    ai.conv_layers.resize(2);
    ai.conv_layers[0].initialize(ivec3(28, 28, 1), ivec3(5, 5, 16), rand.next());
    ai.conv_layers[1].initialize(ivec3(12, 12, 16), ivec3(3, 3, 32), rand.next());

    ai.tags = {
        {addie::tag_layer::CONV, 0},
        {addie::tag_layer::POOLING, 0},
        {addie::tag_layer::CONV, 1},
        {addie::tag_layer::DENSE, 0},
        {addie::tag_layer::DENSE, 1},
        {addie::tag_layer::DENSE, 2},
    };

    /*
    time elapsed: 3.56456 time left: 354.323 ERROR: 0.258275
    time elapsed: 6.92852 time left: 340.192 ERROR: 0.185375
    time elapsed: 10.2641 time left: 332.33 ERROR: 0.127527
    time elapsed: 13.4179 time left: 322.365 ERROR: 0.105965
    time elapsed: 16.8836 time left: 321.059 ERROR: 0.0948143
    time elapsed: 20.1686 time left: 316.199 ERROR: 0.0810233
    time elapsed: 23.3538 time left: 310.462 ERROR: 0.0769935
    time elapsed: 26.4009 time left: 303.776 ERROR: 0.0699215
    time elapsed: 29.3375 time left: 296.78 ERROR: 0.0646131
    time elapsed: 32.2576 time left: 290.448 ERROR: 0.0574388
    time elapsed: 35.3563 time left: 286.181 ERROR: 0.0523461
    time elapsed: 38.6035 time left: 283.199 ERROR: 0.0530268
    time elapsed: 43.4748 time left: 291.05 ERROR: 0.0576706
    time elapsed: 47.5965 time left: 292.476 ERROR: 0.0487386
    time elapsed: 51.0689 time left: 289.481 ERROR: 0.0462369
    time elapsed: 54.4433 time left: 285.912 ERROR: 0.0558418
    time elapsed: 57.9385 time left: 282.957 ERROR: 0.0426778
    time elapsed: 61.165 time left: 278.716 ERROR: 0.0454139
    time elapsed: 64.345 time left: 274.384 ERROR: 0.0469409
    time elapsed: 67.57 time left: 270.348 ERROR: 0.043213
    time elapsed: 71.3413 time left: 268.444 ERROR: 0.0390803
    time elapsed: 74.8215 time left: 265.338 ERROR: 0.0405475
    time elapsed: 78.0504 time left: 261.358 ERROR: 0.037744
    time elapsed: 81.4198 time left: 257.886 ERROR: 0.0360511
    time elapsed: 0.115194 time left: 11.4505 ERROR: 0.249758
    */

    ai.rate = rate;
    ai.B1 = B1;
    ai.B2 = B2;

    if(argc > 6) ai.read_params(argv[6]);

    //

    std::vector<uint8_t> bytes;

    //

    ivec2 image_size;
    auto images = addie::decode_idx3("external/addie/res/train-images.idx3-ubyte", image_size);
    auto labels = addie::decode_idx1("external/addie/res/train-labels.idx1-ubyte");
    for(int i = 0; i < images.size(); ++i) {
        std::vector<float> image(images[i].size());
        for(int j = 0; j < images[i].size(); ++j) {
            image[j] = float(images[i](j)) / 0xFF;
        }

        uint8_t label = labels[i]; 
        Eigen::VectorXf outputs(10);
        outputs.fill(0.0f);

        outputs[label] = 1.0f;

        training_outputs.push_back(outputs);
    }
    training_inputs = images;

    images = addie::decode_idx3("external/addie/res/test-images.idx3-ubyte", image_size);
    labels = addie::decode_idx1("external/addie/res/test-labels.idx1-ubyte");
    for(int i = 0; i < images.size(); ++i) {
        std::vector<float> image(images[i].size());
        for(int j = 0; j < images[i].size(); ++j) {
            image[j] = float(images[i](j)) / 0xFF;
        }


        uint8_t label = labels[i]; 
        Eigen::VectorXf outputs(10);
        outputs.fill(0.0f);

        outputs[label] = 1.0f;

        test_outputs.push_back(outputs);
    }
    test_inputs = images;

    std::vector<uint> permutation_base(training_inputs.size());
    std::iota(permutation_base.begin(), permutation_base.end(), 0);
    std::vector<uint> permutation_base2(permutation_base.begin(), permutation_base.begin() + test_inputs.size());

    //

    double start_time = addie::get_time();

    uint index = 0xFFFFFFFF - batch_size;
    
    
    //
    
    Eigen::MatrixXf tinputs(784, 1024);
    Eigen::MatrixXf toutputs(10, 1024);
    
    std::vector<uint> permutation = permute(permutation_base2, rand, 1024);
    for(int i = 0; i < 1024; ++i) {
        uint j = permutation[i];

        Eigen::Map<Eigen::VectorXf> vector(
            test_inputs[j].data(), test_inputs[j].size()
        );

        tinputs.col(i) = vector.eval(); 
        toutputs.col(i) = test_outputs[j];
    }
    
    auto error = ai.compute_error(tinputs, toutputs);
    std::cout << error.sum() / float(1024) << "\n";
    
    float frac = 0.0f;
    for(int k = 0; k < num_steps; ++k) {
        if(interrupt) break;

        Eigen::MatrixXf batch_inputs(784, batch_size);
        Eigen::MatrixXf batch_outputs(10, batch_size);

        //

        if(index + batch_size >= training_inputs.size()) {
            permutation_base = permute(permutation_base, rand, permutation_base.size() - 1);
            index = 0;
        }

        for(int i = 0; i < batch_size; ++i) {
            uint j = permutation_base[index];

            Eigen::Map<Eigen::VectorXf> vector(
                training_inputs[j].data(), training_inputs[j].size()
            );

            batch_inputs.col(i) = vector.eval();
            batch_outputs.col(i) = training_outputs[j];

            ++index;
        }

        ai.learn(batch_inputs, batch_outputs);

        std::stringstream ss;
        float new_frac = float(k + 1) / num_steps;

        if(floor(new_frac * 100) != floor(frac * 100)) {
            auto error = ai.compute_error(tinputs, toutputs);

            double current_time = addie::get_time();
            double elapsed = current_time - start_time;
            double time_left = elapsed * ((1.0f - frac) / frac);

            ss << "time elapsed: " << elapsed << " time left: " << time_left << " "; 
            ss << "ERROR: " << error.sum() / float(1024);
            
            std::cout << ss.str() << std::endl; 
        }

        frac = new_frac;
    }
    
    ai.write_params("output/addie_params" + std::to_string(addie::get_timestamp()) + ".bin");
}