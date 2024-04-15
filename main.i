%module Andor3Capture
%{
#include "main.cpp"
%}

// Very simple C++ example for linked list

class A3C {

    public:
        A3C(AT_H handle);

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