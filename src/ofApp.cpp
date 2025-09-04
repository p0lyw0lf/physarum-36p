#include "ofApp.h"

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
}

void ofApp::loadParameters() {
  int matrixRow = selectedPoints[pointCursorIndex];

  auto getParam = [&](int column) {
    return ParametersMatrix[matrixRow][column];
  };

  PointSettings loadedParams;

  loadedParams.defaultScalingFactor = getParam(PARAMS_DIMENSION - 1);
  loadedParams.SensorDistance0 = getParam(0);
  loadedParams.SD_exponent = getParam(1);
  loadedParams.SD_amplitude = getParam(2);
  loadedParams.SensorAngle0 = getParam(3);
  loadedParams.SA_exponent = getParam(4);
  loadedParams.SA_amplitude = getParam(5);
  loadedParams.RotationAngle0 = getParam(6);
  loadedParams.RA_exponent = getParam(7);
  loadedParams.RA_amplitude = getParam(8);
  loadedParams.MoveDistance0 = getParam(9);
  loadedParams.MD_exponent = getParam(10);
  loadedParams.MD_amplitude = getParam(11);
  loadedParams.SensorBias1 = getParam(12);
  loadedParams.SensorBias2 = getParam(13);

  simulationParameters[0] = loadedParams;

  simulationParametersBuffer.updateData(simulationParameters);
}

void ofApp::update() {
  loadParameters();

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
  ofPushMatrix();
  ofScale(1.0 * ofGetWidth() / fboDisplay.getWidth(),
          1.0 * ofGetHeight() / fboDisplay.getHeight());
  fboDisplay.draw(0, 0);
  ofPopMatrix();
}

void ofApp::keyPressed(int key) {
  switch (key) {
  case OF_KEY_RIGHT:
    pointCursorIndex = (pointCursorIndex + 1 + numberOfPoints) % numberOfPoints;
    break;
  case OF_KEY_LEFT:
    pointCursorIndex = (pointCursorIndex - 1 + numberOfPoints) % numberOfPoints;
    break;
  case OF_KEY_UP:
    pointCursorIndex = (pointCursorIndex + 1 + numberOfPoints) % numberOfPoints;
    break;
  case OF_KEY_DOWN:
    pointCursorIndex = (pointCursorIndex - 1 + numberOfPoints) % numberOfPoints;
    break;
  }

  loadParameters();
}
