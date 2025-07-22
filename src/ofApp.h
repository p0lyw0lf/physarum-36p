#pragma once

#include "ofMain.h"
#include "points_basematrix.h"

namespace SimulationSettings
{
    constexpr int WIDTH = 1280;
    constexpr int HEIGHT = 736;
    constexpr size_t NUMBER_OF_PARTICLES = 512 * 512 * 22;
    constexpr int PARTICLE_PARAMETERS_COUNT = 4;
    constexpr float DECAY_FACTOR = 0.75f;
    constexpr float DEPOSIT_FACTOR = 0.003f;
    constexpr int FRAME_RATE = 60;
    constexpr int NUMBER_OF_COLOR_MODES = 10;
    constexpr int WORK_GROUP_SIZE = 32;
    constexpr int PARTICLE_WORK_GROUPS = 128;
};

struct PointSettings
{
    float defaultScalingFactor;
    float SensorDistance0;
    float SD_exponent;
    float SD_amplitude;
    float SensorAngle0;
    float SA_exponent;
    float SA_amplitude;
    float RotationAngle0;
    float RA_exponent;
    float RA_amplitude;
    float MoveDistance0;
    float MD_exponent;
    float MD_amplitude;
    float SensorBias1;
    float SensorBias2;
};

class ofApp : public ofBaseApp
{

public:
    void setup();
    void update();
    void draw();

    std::vector<int> selectedPoints = {0, 5, 2, 15, 3, 4, 6, 1, 7, 8, 9, 10, 11, 12, 14, 16, 13, 17, 18, 19, 20, 21}; // list of chosen points from the matrix
    int numberOfPoints = selectedPoints.size();
    int pointCursorIndex = 0;
    void loadParameters();

    ofFbo trailReadBuffer, trailWriteBuffer, fboDisplay;
    ofShader setterShader, moveShader, depositShader, diffusionShader;

    std::vector<uint32_t> counter;
    ofBufferObject counterBuffer;
    std::vector<PointSettings> simulationParameters;
    ofBufferObject simulationParametersBuffer;
    struct Particle
    {
        glm::vec4 data;
    };
    std::vector<uint16_t> particles;
    ofBufferObject particlesBuffer;

    void keyPressed(int key);
};
