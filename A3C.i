%module Andor3Capture
%{
#include "A3C.cpp"
%}

class A3C {

public:
    A3C(long handle);
    void setFrameLimit(int limit);
    void setOutputPath(string path);
    int getFrameLimit();
    string getOutputPath();
    void start();
    void stop();
    int acquire();
    int process();
    int write();

};