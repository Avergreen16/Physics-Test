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
    float momentum_decay = 0.85;
    float learning_rate = 0.01f;
    
    if(argc > 1) momentum_decay = std::stof(argv[1]);
    if(argc > 2) learning_rate = std::stof(argv[2]);
    if(argc > 3) num_steps = std::stoi(argv[3]);

    std::cout << momentum_decay << " " << learning_rate << "\n";

    //

    addie::random32 rand(0xE1);
    /*
    addie::network ai({7 * 7 * 24, 128, 64, 32, 10}, 0xE1);
    ai.players.resize(2);
    ai.players[0].initialize(ivec3(28, 28, 16), 2);
    ai.players[1].initialize(ivec3(14, 14, 24), 2);
    ai.clayers.resize(2);
    ai.clayers[0].initialize(ivec3(28, 28, 1), ivec3(28, 28, 16), ivec2(5), rand.next());
    ai.clayers[1].initialize(ivec3(14, 14, 16), ivec3(14, 14, 24), ivec2(3), rand.next());

    ai.tags = {
        {addie::tag_layer::CONV, 0},
        {addie::tag_layer::POOLING, 0},
        {addie::tag_layer::CONV, 1},
        {addie::tag_layer::POOLING, 1},
        {addie::tag_layer::DENSE, 0},
        {addie::tag_layer::DENSE, 1},
        {addie::tag_layer::DENSE, 2},
        {addie::tag_layer::DENSE, 3},
    };
    */
    
    
    addie::network ai({784, 128, 64, 32, 10}, 0xE1);
    ai.tags = {
        {addie::tag_layer::DENSE, 0},
        {addie::tag_layer::DENSE, 1},
        {addie::tag_layer::DENSE, 2},
        {addie::tag_layer::DENSE, 3},
    };

    //if(argc > 4) ai.read_params(argv[4]);

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

    //std::cout << "ERROR: " << ai.compute_error(test_inputs, test_outputs).x << std::endl;
    
    //auto f = ai.compute_error_hot(test_inputs, test_outputs);
    //std::cout << f.size() << "\n";
    
    uint batch_size = 96;

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

        //
        /*
        0.0174929
        0.0186087
        0.0149799
        0.0121312
        0.0122109
        0.0167653
        */

        std::stringstream ss;
        float new_frac = float(k + 1) / num_steps;

        /*
        if(floor(new_frac * 1000) != floor(frac * 1000)) {
        }
        */

        if(floor(new_frac * 100) != floor(frac * 100)) {
            double current_time = addie::get_time();
            double elapsed = current_time - start_time;
            double time_left = elapsed * ((1.0f - frac) / frac);
            ss << "time elapsed: " << elapsed << " time left: " << time_left << " "; 

            std::cout << "x";

            auto error = ai.compute_error(tinputs, toutputs);
            
            ss << "ERROR: " << error.sum() / float(1024);
            
            std::cout << ss.str() << std::endl; 
        }

        frac = new_frac;
    }
    
    //

    std::cout << "FINISH!\n";
    //ai.write_params("output/addie_params" + std::to_string(addie::get_timestamp()) + ".bin");
}