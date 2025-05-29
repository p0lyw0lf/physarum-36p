#include "ofMain.h"
#include "ofApp.h"

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
// by Etienne Jacob / bleuje

// Crediting:
// Work derived 36 Points (www.sagejenson.com/36points/) by Sage Jenson (mxsage) but with a different implementation.
// This project is using counters on pixels and this is different.
// It's using the set of parameters from 36 Points, some points work well, some don't, I had to tune stuff and I kept what worked.


//========================================================================
int main( ){
    // Uses compute shaders which are only supported since
    // openGL 4.3
    ofGLWindowSettings settings;
    settings.setGLVersion(4,3);
    settings.setSize(1280,736);
    //settings.windowMode = OF_FULLSCREEN;
    ofCreateWindow(settings);

    ofRunApp(new ofApp());
}
