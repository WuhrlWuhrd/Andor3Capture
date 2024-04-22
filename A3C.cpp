#include "atfunc.cpp"
#include "queue.cpp"
#include <ctime>
#include <fstream>
#include <thread>
#include <vector>

using namespace std;

#define Thread thread
#define Vector vector

class A3C {

private:

    AT_H handle;
    String outputPath;
    int frameLimit;
    int imageSize;
    Thread acquireThread;
    Thread processThread;
    Thread writeThread;
    Thread monitorThread;
    FIFOQueue<unsigned char *> processQueue;
    FIFOQueue<unsigned char *> writeQueue;
    Vector<String> errors;
    ostream *out = &cout;
    bool running    = false;
    bool monitoring = false;

    long acquireCount = 0;
    long processCount = 0;
    long writeCount   = 0;

public:

    A3C(AT_H handle) {

        this->handle     = handle;
        this->outputPath = "output.h5";
        this->frameLimit = -1;

        int result = AT_Flush(handle);

        if (result != AT_SUCCESS) {
            throw result;
        }
        
    }

    void setVerbose(bool flag) {
        this->out = flag ? &cout : new ostream(0);
    }

    void setFrameLimit(int limit) {
        this->frameLimit = limit;
    }

    void setOutputPath(string path) {
        this->outputPath = path;
    }

    int getFrameLimit() {
        return frameLimit;
    }

    string getOutputPath() {
        return outputPath;
    }

    void start() {

        running    = true;
        monitoring = true;

        // Clear and reset everything
        processQueue.clear();
        writeQueue.clear();

        // Start everything going on separate threads

        *out << "Starting writing thread... ";
        writeThread = Thread(&A3C::write, this);
        *out << "Done." << endl;

        *out << "Starting processing thread... ";
        processThread = Thread(&A3C::process, this);
        *out << "Done." << endl;

        *out << "Starting acquisition thread... ";
        acquireThread = Thread(&A3C::acquire, this);
        *out << "Done." << endl;

        *out << "Starting monitoring thread... ";
        monitorThread = Thread(&A3C::monitor, this);
        *out << "Done." << endl;
        
    }

    void stop() {

        // Change flag to stop, and wait for all threads to shutdown
        running = false;
        acquireThread.join();

        processQueue.push(nullptr);
        processThread.join();

        writeQueue.push(nullptr);
        writeThread.join();

        monitoring = false;
        monitorThread.join();

    }

    int acquire() {

        long   start = time(0);
        double temperature;
        float  frameRate  = getFloat(handle, "FrameRate");
        float  frameTime  = 1.0 / frameRate;
        int    frameCount = (int) frameRate;
        long   timeOut    = (long) (1000.0 * 2.0 * frameTime);

        AT_64 atSize;
        unsigned char *pBuffer;
        int size = 0;

        AT_GetInt(handle, L"ImageSizeBytes", &atSize);

        imageSize = (int)atSize;

        setEnum(handle, "CycleMode", "Continuous");
        AT_Command(handle, L"AcquisitionStart");

        for (acquireCount = 0; running; acquireCount++) {

            // Create new buffer, queue it and await data
            unsigned char *buffer = new unsigned char[imageSize];

            int qCode = AT_QueueBuffer(handle, buffer, imageSize);
            int wCode = AT_WaitBuffer(handle, &pBuffer, &size, timeOut > 500 ? timeOut : 500);

            if (qCode != AT_SUCCESS || wCode != AT_SUCCESS) {
                
                cerr << endl << endl << "DROPPED FRAME: RESTARTING ACQUISITION" << endl;

                errors.push_back("Error (" + to_string(qCode) + ", " + to_string(wCode) + "), restarting acquisition.");

                AT_Command(handle, L"AcquisitionStop");
                AT_Flush(handle);
                AT_Command(handle, L"AcquisitionStart");

                continue;

            }

            // Push the buffer into the processing queue
            processQueue.push(pBuffer);

        }

        AT_Command(handle, L"AcquisitionStop");
        AT_Flush(handle);

        return 0;
    }

    int process() {

        AT_64 imageHeight;
        AT_64 imageWidth;
        AT_64 imageStride;

        AT_GetInt(handle, L"AOI Height", &imageHeight);
        AT_GetInt(handle, L"AOI Width", &imageWidth);
        AT_GetInt(handle, L"AOI Stride", &imageStride);

        int size = imageHeight * imageWidth;

        for (processCount = 0; running || processQueue.hasWaiting(); processCount++) {

            unsigned char *buffer = processQueue.pop();

            if (buffer == nullptr) {
                continue;
            }
            
            unsigned char *converted = new unsigned char[size];

            AT_ConvertBuffer(buffer, converted, imageWidth, imageHeight, imageStride, L"Mono12", L"Mono12");

            writeQueue.push(converted);

            delete[] buffer;
        }

        return 0;

    }

    int write() {

        long start = time(0);
        double temperature;
        bool stopped = false;
        int queued = 0;

        remove(outputPath.c_str());

        ofstream output = ofstream(outputPath, ios::binary | ios::out);

        *out << "FPS = ... Hz, T = ...*C, PQ = ..., WQ = ...";

        AT_64 imageHeight;
        AT_64 imageWidth;

        AT_GetInt(handle, L"AOI Height", &imageHeight);
        AT_GetInt(handle, L"AOI Width", &imageWidth);

        int size = imageHeight * imageWidth;

        for (writeCount = 0; running || writeQueue.hasWaiting(); writeCount++) {

            unsigned char *buffer = writeQueue.pop();

            if (buffer == nullptr) {
                continue;
            }

            for (int i = 0; i < size; i++) {
                output << buffer[i];
            }

            delete[] buffer;

        }

        *out << endl;

        output.close();

        return 0;
    }

    int monitor() {

        long start = time(0);

        this_thread::sleep_for(chrono::seconds(1));

        *out << "...";

        while (monitoring) {

            if (running) {

                long   duration = time(0) - start;
                double aRate    = acquireCount / duration;
                double pRate    = processCount / duration;
                double wRate    = writeCount / duration;
                double temp     = getFloat(handle, "SensorTemperature");
                int    pQueue   = processQueue.size();
                int    wQueue   = writeQueue.size();

                *out << "\r\e[K"
                    << "A = " << aRate << " Hz, P = " << pRate << " Hz, "
                    << ", W = " << wRate << " Hz, PQ = " << pQueue << ", WQ = " << wQueue;

            } else {

                *out << "\r\e[K"
                     << "Stopping threads: Left to Process = " << processQueue.size() << ", Left to Write = " << writeQueue.size();

            }

            if (!errors.empty()) {

                *out << endl;

                for (int i = 0; i < errors.size(); i++) {
                    *out << errors[i] << endl;
                }

                errors.clear();

            }

            this_thread::sleep_for(chrono::seconds(1));

        }

        *out << "\r\e[K" << "Shutdown complete." << endl;
        *out << "Total frames written to disk: " << writeCount << endl;

        return 0;
        
    }
    
};
