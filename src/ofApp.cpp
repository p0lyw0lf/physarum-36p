#include "ofApp.h"
#include "increments.h"
#include "ofEvents.h"
#include "ofGraphics.h"
#include "ofSoundPlayer.h"
#include <sstream>

void ofApp::setup() {
  ofSetFrameRate(SimulationSettings::FRAME_RATE);

  ofEnableAntiAliasing();

  counter.resize(SimulationSettings::WIDTH * SimulationSettings::HEIGHT);
  counterBuffer.allocate(counter, GL_DYNAMIC_DRAW);

  trailReadBuffer.allocate(SimulationSettings::WIDTH,
                           SimulationSettings::HEIGHT, GL_R16F);
  trailWriteBuffer.allocate(SimulationSettings::WIDTH,
                            SimulationSettings::HEIGHT, GL_R16F);
  fboDisplay.allocate(SimulationSettings::WIDTH, SimulationSettings::HEIGHT,
                      GL_RGBA8);

  setterShader.setupShaderFromFile(GL_COMPUTE_SHADER,
                                   "shaders/computeshader_setter.glsl");
  setterShader.linkProgram();

  depositShader.setupShaderFromFile(GL_COMPUTE_SHADER,
                                    "shaders/computeshader_deposit.glsl");
  depositShader.linkProgram();

  moveShader.setupShaderFromFile(GL_COMPUTE_SHADER,
                                 "shaders/computeshader_move.glsl");
  moveShader.linkProgram();

  diffusionShader.setupShaderFromFile(GL_COMPUTE_SHADER,
                                      "shaders/computeshader_diffusion.glsl");
  diffusionShader.linkProgram();

  particles.resize(SimulationSettings::NUMBER_OF_PARTICLES *
                   SimulationSettings::PARTICLE_PARAMETERS_COUNT);

  auto floatAsUint16 = [](float x) -> uint16_t {
    return static_cast<uint16_t>(
        std::round(std::clamp(x, 0.0f, 1.0f) * 65535.0f));
  };

  // initialize particles
  for (size_t i = 0; i < SimulationSettings::NUMBER_OF_PARTICLES; i++) {
    particles[SimulationSettings::PARTICLE_PARAMETERS_COUNT * i + 0] =
        floatAsUint16(ofRandom(0, SimulationSettings::WIDTH) /
                      SimulationSettings::WIDTH);
    particles[SimulationSettings::PARTICLE_PARAMETERS_COUNT * i + 1] =
        floatAsUint16(ofRandom(0, SimulationSettings::HEIGHT) /
                      SimulationSettings::HEIGHT);
    particles[SimulationSettings::PARTICLE_PARAMETERS_COUNT * i + 2] =
        floatAsUint16(ofRandom(1));
    particles[SimulationSettings::PARTICLE_PARAMETERS_COUNT * i + 3] =
        floatAsUint16(ofRandom(1));
  }
  particlesBuffer.allocate(particles, GL_DYNAMIC_DRAW);

  simulationParameters.resize(
      1); // in other projects I'm using many Points at the same time
  simulationParametersBuffer.allocate(simulationParameters, GL_DYNAMIC_DRAW);
  simulationParametersBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 5);

  trailReadBuffer.getTexture().bindAsImage(0, GL_READ_ONLY);
  trailWriteBuffer.getTexture().bindAsImage(1, GL_WRITE_ONLY);
  particlesBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 2);
  counterBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
  fboDisplay.getTexture().bindAsImage(4, GL_WRITE_ONLY);

  loadParameters();

  music.load("project.mp3");
}

void ofApp::loadParameters() {
  offset = PointSettings<float>{0};
  increment = PointSettings<int>{0};

  updateParameters();
}

void ofApp::updateParameters() {
  int matrixRow = selectedPoints[pointCursorIndex];

  auto getParam = [&](int column) {
    return ParametersMatrix[matrixRow][column];
  };

  PointSettings<float> loadedParams;

  loadedParams.defaultScalingFactor =
      getParam(PARAMS_DIMENSION - 1) + offset.defaultScalingFactor;
  loadedParams.SensorDistance0 = getParam(0) + offset.SensorDistance0;
  loadedParams.SD_exponent = getParam(1) + offset.SD_exponent;
  loadedParams.SD_amplitude = getParam(2) + offset.SD_amplitude;
  loadedParams.SensorAngle0 = getParam(3) + offset.SensorAngle0;
  loadedParams.SA_exponent = getParam(4) + offset.SA_exponent;
  loadedParams.SA_amplitude = getParam(5) + offset.SA_amplitude;
  loadedParams.RotationAngle0 = getParam(6) + offset.RotationAngle0;
  loadedParams.RA_exponent = getParam(7) + offset.RA_exponent;
  loadedParams.RA_amplitude = getParam(8) + offset.RA_amplitude;
  loadedParams.MoveDistance0 = getParam(9) + offset.MoveDistance0;
  loadedParams.MD_exponent = getParam(10) + offset.MD_exponent;
  loadedParams.MD_amplitude = getParam(11) + offset.MD_amplitude;
  loadedParams.SensorBias1 = getParam(12) + offset.SensorBias1;
  loadedParams.SensorBias2 = getParam(13) + offset.SensorBias2;

  simulationParameters[0] = loadedParams;

  simulationParametersBuffer.updateData(simulationParameters);
}

void ofApp::update() {
  ofSoundUpdate();

  bool currentPlaying = playing.load();
  if (currentPlaying) {
    shaderUpdate();
  }

  bool currentMusicPlaying = musicPlaying.load();
  bool shouldPlayMusic = currentPlaying && currentMusicPlaying;
  if (shouldPlayMusic) {
    if (!lastMusicPlaying) {
      music.setPositionMS(lastPositionMS);
      music.play();
    }
  } else if (lastMusicPlaying) {
    lastPositionMS = music.getPositionMS();
    music.stop();
  }

  lastMusicPlaying = shouldPlayMusic;
}

void ofApp::shaderUpdate() {
  setterShader.begin();
  setterShader.setUniform1i("width", trailReadBuffer.getWidth());
  setterShader.setUniform1i("height", trailReadBuffer.getHeight());
  setterShader.setUniform1i("value", 0);
  setterShader.dispatchCompute(
      SimulationSettings::WIDTH / SimulationSettings::WORK_GROUP_SIZE,
      SimulationSettings::HEIGHT / SimulationSettings::WORK_GROUP_SIZE, 1);
  setterShader.end();

  trailReadBuffer.getTexture().bindAsImage(0, GL_READ_ONLY);
  trailWriteBuffer.getTexture().bindAsImage(1, GL_WRITE_ONLY);

  moveShader.begin();
  moveShader.setUniform1i("width", trailReadBuffer.getWidth());
  moveShader.setUniform1i("height", trailReadBuffer.getHeight());

  moveShader.dispatchCompute(
      particles.size() / (SimulationSettings::PARTICLE_WORK_GROUPS *
                          SimulationSettings::PARTICLE_PARAMETERS_COUNT),
      1, 1);
  moveShader.end();

  depositShader.begin();
  depositShader.setUniform1i("width", trailReadBuffer.getWidth());
  depositShader.setUniform1i("height", trailReadBuffer.getHeight());
  depositShader.setUniform1f("depositFactor",
                             SimulationSettings::DEPOSIT_FACTOR);
  depositShader.dispatchCompute(
      SimulationSettings::WIDTH / SimulationSettings::WORK_GROUP_SIZE,
      SimulationSettings::HEIGHT / SimulationSettings::WORK_GROUP_SIZE, 1);
  depositShader.end();

  trailReadBuffer.getTexture().bindAsImage(1, GL_WRITE_ONLY);
  trailWriteBuffer.getTexture().bindAsImage(0, GL_READ_ONLY);

  diffusionShader.begin();
  diffusionShader.setUniform1i("width", trailReadBuffer.getWidth());
  diffusionShader.setUniform1i("height", trailReadBuffer.getHeight());
  diffusionShader.setUniform1f("decayFactor", SimulationSettings::DECAY_FACTOR);
  diffusionShader.dispatchCompute(
      trailReadBuffer.getWidth() / SimulationSettings::WORK_GROUP_SIZE,
      trailReadBuffer.getHeight() / SimulationSettings::WORK_GROUP_SIZE, 1);
  diffusionShader.end();

  std::stringstream strm;
  strm << "fps: " << ofGetFrameRate();
  ofSetWindowTitle(strm.str());
}

void ofApp::draw() {
  // Draw main simulation
  ofPushMatrix();
  float xscale = 1.0 * (ofGetWidth() - SimulationSettings::SIDEBAR_WIDTH) /
                 fboDisplay.getWidth();
  float yscale = 1.0 * (ofGetHeight() - SimulationSettings::HEADER_HEIGHT) /
                 fboDisplay.getHeight();
  ofScale(xscale, yscale);
  fboDisplay.draw(SimulationSettings::SIDEBAR_WIDTH / xscale,
                  SimulationSettings::HEADER_HEIGHT / yscale);
  ofPopMatrix();

  ofPushStyle();
  ofFill();
  ofSetColor(0, 0, 0);
  ofSetLineWidth(0);
  // Draw sidebar
  ofDrawRectangle(0, 0, SimulationSettings::SIDEBAR_WIDTH, ofGetHeight());
  // Draw header
  ofDrawRectangle(0, 0, ofGetWidth(), SimulationSettings::HEADER_HEIGHT);
  ofPopStyle();

  ofPushStyle();
  ofFill();
  ofSetColor(255, 255, 255);
  ofSetLineWidth(0);
  // Draw play/pause button on top of sidebar
  float midpoint = SimulationSettings::SIDEBAR_WIDTH / 2.0;
  if (playing && musicPlaying) {
    ofDrawTriangle(20, 20, 20, SimulationSettings::SIDEBAR_WIDTH - 20,
                   SimulationSettings::SIDEBAR_WIDTH - 20, midpoint);
  } else {
    ofDrawRectangle(20, 20, midpoint - 30,
                    SimulationSettings::SIDEBAR_WIDTH - 40);
    ofDrawRectangle(midpoint + 10, 20, midpoint - 30,
                    SimulationSettings::SIDEBAR_WIDTH - 40);
  }

  // Display values for each parameter
#define DRAW(name, field, x, y)                                                \
  drawSetting(name, &PointSettings<float>::field, &PointSettings<int>::field,  \
              x, y);

  int text_start = SimulationSettings::SIDEBAR_WIDTH;
#define DRAW3(name0, field0, name1, field1, name2, field2)                     \
  DRAW(name0, field0, text_start, 10);                                         \
  DRAW(name1, field1, text_start, 30);                                         \
  DRAW(name2, field2, text_start, 50);                                         \
  text_start += 140;

  DRAW3("SD0", SensorDistance0, "SDA", SD_amplitude, "SDE", SD_exponent);
  DRAW3("SA0", SensorAngle0, "SAA", SA_amplitude, "SAE", SA_exponent);
  DRAW3("RA0", RotationAngle0, "RAA", RA_amplitude, "RAE", RA_exponent);
  DRAW3("MD0", MoveDistance0, "MDA", MD_amplitude, "MDE", MD_exponent);
  DRAW3("SB1", SensorBias1, "SB2", SensorBias2, "SF", defaultScalingFactor);

  ofPopStyle();
}

void ofApp::drawSetting(const char *name, float PointSettings<float>::*offset_m,
                        int PointSettings<int>::*increment_m, float x,
                        float y) {
  stringstream buf;
  buf << name << ": " << simulationParameters[0].*offset_m << "("
      << INCREMENTS[increment.*increment_m] << ")";
  ofDrawBitmapString(buf.str(), x, y);
}

void ofApp::keyPressed(ofKeyEventArgs &key) {
  switch (key.key) {
  case OF_KEY_RIGHT:
    pointCursorIndex = (pointCursorIndex + 1 + numberOfPoints) % numberOfPoints;
    goto full_load;
  case OF_KEY_LEFT:
    pointCursorIndex = (pointCursorIndex - 1 + numberOfPoints) % numberOfPoints;
    goto full_load;
  case OF_KEY_UP:
    pointCursorIndex = (pointCursorIndex + 1 + numberOfPoints) % numberOfPoints;
    goto full_load;
  case OF_KEY_DOWN:
    pointCursorIndex = (pointCursorIndex - 1 + numberOfPoints) % numberOfPoints;
    goto full_load;
  case OF_KEY_SPACE:
    playing ^= 1;
    goto full_load;
  }

#define CHECK(up, down, change, field)                                         \
  checkIncrementKey(key.codepoint, &PointSettings<float>::field,               \
                    &PointSettings<int>::field, {up, down, change});

  CHECK('q', 'a', 'z', SensorDistance0);
  CHECK('w', 's', 'x', SD_amplitude);
  CHECK('e', 'd', 'c', SD_exponent);
  CHECK('r', 'f', 'v', SensorAngle0);
  CHECK('t', 'g', 'b', SA_amplitude);
  CHECK('y', 'h', 'n', SA_exponent);
  CHECK('u', 'j', 'm', RotationAngle0);
  CHECK('i', 'k', ',', RA_amplitude);
  CHECK('o', 'l', '.', RA_exponent);
  CHECK('Q', 'A', 'Z', MoveDistance0);
  CHECK('W', 'S', 'X', MD_amplitude);
  CHECK('E', 'D', 'C', MD_exponent);
  CHECK('R', 'F', 'V', SensorBias1);
  CHECK('T', 'G', 'B', SensorBias2);
  CHECK('Y', 'H', 'N', defaultScalingFactor);

  if (key.codepoint == 'M') {
    musicPlaying ^= 1;
  }

  updateParameters();
  return;

full_load:
  loadParameters();
}

void ofApp::checkIncrementKey(int codepoint,
                              float PointSettings<float>::*offset_m,
                              int PointSettings<int>::*increment_m,
                              const int (&keys)[3]) {
  auto offset_p = &(offset.*offset_m);
  auto increment_p = &(increment.*increment_m);
  if (codepoint == keys[0]) {
    *offset_p += INCREMENTS[*increment_p];
  } else if (codepoint == keys[1]) {
    *offset_p -= INCREMENTS[*increment_p];
  } else if (codepoint == keys[2]) {
    *increment_p = (*increment_p + 1) % NUM_INCREMENTS;
  }
}
