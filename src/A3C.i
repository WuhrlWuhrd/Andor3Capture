%module Andor3Capture
%include std_string.i
%include exception.i   
%inline %{
using namespace std;
%}
%{
#include "A3C.cpp"
%}    
%exception { 
    try {
        $action
    } catch (std::string &e) {
        SWIG_exception(SWIG_RuntimeError, e.c_str());
    } catch (int &e) {
        SWIG_exception(SWIG_RuntimeError, ("Andor3 Error Code: " + std::to_string(e)).c_str());
    } catch (...) {
        SWIG_exception(SWIG_RuntimeError, "unknown exception");
    }
}

class A3C {

public:

    A3C(long handle);

    void setVerbose(bool flag);

    void setFrameLimit(int limit);

    void setOutputPath(string path);

    int getFrameLimit();

    string getOutputPath();

    void start();

    void stop();

};
