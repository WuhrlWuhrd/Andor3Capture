%module PyZyla
%inline %{
%}
%{
#pragma once
#include "A3C.cpp"
%}    

class A3C {

public:

    A3C(const A3C& other);

    A3C(long handle);

    void setVerbose(bool flag);

    void setFrameLimit(int limit);

    void setOutputPath(std::string path);

    int getFrameLimit();

    std::string getOutputPath();

    void start();

    void stop();

};
