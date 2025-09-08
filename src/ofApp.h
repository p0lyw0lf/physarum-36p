#pragma once

#include "ofMain.h"
#include "points_basematrix.h"
#include <atomic>
#include <cstdint>

namespace SimulationSettings {
constexpr int HEADER_HEIGHT = 70;
// TODO: see if I can change WIDTH/HEIGHT/NUMBER_OF_PARTICLES to exactly match
// resolution currently seems like no, I should just leave them as-is because
// they're already pretty finely-tuned for the exact parameters & look nice
// enough anyways
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
}; // namespace SimulationSettings

template <typename T> struct PointSettings {
  T defaultScalingFactor = 0;
  T SensorDistance0 = 0;
  T SD_exponent = 0;
  T SD_amplitude = 0;
  T SensorAngle0 = 0;
  T SA_exponent = 0;
  T SA_amplitude = 0;
  T RotationAngle0 = 0;
  T RA_exponent = 0;
  T RA_amplitude = 0;
  T MoveDistance0 = 0;
  T MD_exponent = 0;
  T MD_amplitude = 0;
  T SensorBias1 = 0;
  T SensorBias2 = 0;
};

class ofApp : public ofBaseApp {
public:
  void setup();
  void update();
  void draw();

  void keyPressed(ofKeyEventArgs &key);

private:
  void shaderUpdate();

  std::vector<int> selectedPoints = {
      0,  5,  2,  15, 3,  4,  6,  1,  7,  8, 9, 10,
      11, 12, 14, 16, 13, 17, 18, 19, 20, 21}; // list of chosen points from the
                                               // matrix
  int numberOfPoints = selectedPoints.size();
  int pointCursorIndex = 0; // indexes into selectedPoints
  std::vector<PointSettings<float>> simulationParameters;
  ofBufferObject simulationParametersBuffer;

  // The offset from the current base point when we load a setting
  PointSettings<float> offset;
  // When changing each setting, how much to change it by.
  // These values index into the INCREMENTS array
  PointSettings<int> increment;
  // Loads the parameters from a changed pointCursorIndex, resetting offset and
  // increment when it does so.
  void loadParameters();
  // Loads the paramenters into the simulationParametersBuffer given just a
  // changed offset array.
  void updateParameters();

  // Given a field to modify, check if that key is pressed and perform the
  // resulting modification accordingly.
  // keys[0] == increment up
  // keys[1] == increment down
  // keys[2] == toggle increment
  void checkIncrementKey(int codepoint, float PointSettings<float>::*offset_m,
                         int PointSettings<int>::*increment_m,
                         const int (&keys)[3]);

  // Displays the value of a given setting using ofDrawBitmapText
  void drawSetting(const char *name, float PointSettings<float>::*offset_m,
                   int PointSettings<int>::*increment_m, float x, float y);

  ofFbo trailReadBuffer, trailWriteBuffer, fboDisplay;
  ofShader setterShader, moveShader, depositShader, diffusionShader;

  std::vector<uint32_t> counter;
  ofBufferObject counterBuffer;
  struct Particle {
    glm::vec4 data;
  };
  std::vector<uint16_t> particles;
  ofBufferObject particlesBuffer;

  // Boolean, either stores 1 or 0. Accessed from all threads.
  std::atomic<uint8_t> playing;
  std::atomic<uint8_t> musicPlaying;
  // Accessed only from the ::update thread.
  bool lastMusicPlaying = false;
  int lastPositionMS = 0;

  ofSoundPlayer music;
};
