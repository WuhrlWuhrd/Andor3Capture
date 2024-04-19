#include "atfunc.cpp"
#include "queue.cpp"
#include <ctime>
#include <fstream>
#include <thread>

using namespace std;

#define Thread thread

class A3C {

private:
    AT_H handle;
    String outputPath;
    int frameLimit;
    int imageSize;
    Thread acquireThread;
    Thread processThread;
    Thread writeThread;
    FIFOQueue<unsigned char *> processQueue;
    FIFOQueue<unsigned char *> writeQueue;
    ostream* out = &cout;

    bool running = false;

public:

    A3C(AT_H handle) {

        this->handle = handle;
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

        running = true;

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

    }

    void stop() {

        // Change flag to stop, and wait for all threads to shutdown
        running = false;

        *out << "Waiting for acquisition thread to terminate... ";
        acquireThread.join();
        *out << "Done." << endl;

        processQueue.push(nullptr);

        *out << "Waiting for processing thread to terminate... ";
        processThread.join();
        *out << "Done." << endl;

        writeQueue.push(nullptr);

        *out << "Waiting for writing thread to terminate... ";
        writeThread.join();
        *out << "Done." << endl;
    }

    int acquire() {

        long start = time(0);
        double temperature;
        int frameRate = 2 * (int) getFloat(handle, "FrameRate");

        AT_64 atSize;
        unsigned char *pBuffer;
        int size = 0;

        AT_GetInt(handle, L"ImageSizeBytes", &atSize);

        imageSize = (int)atSize;

        setEnum(handle, "CycleMode", "Continuous");
        AT_Command(handle, L"AcquisitionStart");

        for (int count = 1; running; count++) {

            // Create new buffer, queue it and await data
            unsigned char *buffer = new unsigned char[imageSize];

            int qCode = AT_QueueBuffer(handle, buffer, imageSize);
            int wCode = AT_WaitBuffer(handle, &pBuffer, &size, 10000);

            if (qCode != AT_SUCCESS || wCode != AT_SUCCESS) {
                
                cerr << "ERROR CODES: " << qCode << ", " << wCode << endl;

                AT_Command(handle, L"AcquisitionStop");
                AT_Flush(handle);
                AT_Command(handle, L"AcquisitionStart");

                continue;

            }

            // Push the buffer into the processing queue
            processQueue.push(pBuffer);

            if ((count % frameRate) == 0) {
                temperature = getFloat(handle, "SensorTemperature");
                *out << "\r\e[K" << std::flush;
                *out << "FPS = " << count / (time(0) - start) << " Hz" << ", T = " << temperature << "*C" << ", PQ = " << processQueue.size() << ", WQ = " << writeQueue.size();
            }

            if (frameLimit > 0 && count >= frameLimit) {
                running = false;
            }
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

        while (running) {

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

        remove(outputPath.c_str());

        ofstream output = ofstream(outputPath, ios::binary | ios::out);

        *out << "FPS = ... Hz, T = ...*C, PQ = ..., WQ = ...";

        AT_64 imageHeight;
        AT_64 imageWidth;

        AT_GetInt(handle, L"AOI Height", &imageHeight);
        AT_GetInt(handle, L"AOI Width", &imageWidth);

        int size = imageHeight * imageWidth;

        for (int count = 1; running; count++) {

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
    
};
