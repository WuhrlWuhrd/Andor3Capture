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

        cout << "Starting writing thread... ";
        writeThread = Thread(&A3C::write, this);
        cout << "Done." << endl;

        cout << "Starting processing thread... ";
        processThread = Thread(&A3C::process, this);
        cout << "Done." << endl;

        cout << "Starting acquisition thread... ";
        acquireThread = Thread(&A3C::acquire, this);
        cout << "Done." << endl;
    }

    void stop() {

        // Change flag to stop, and wait for all threads to shutdown
        running = false;

        cout << "Waiting for acquisition thread to terminate... ";
        acquireThread.join();
        cout << "Done." << endl;

        cout << "Waiting for processing thread to terminate... ";
        processThread.join();
        cout << "Done." << endl;

        cout << "Waiting for writing thread to terminate... ";
        writeThread.join();
        cout << "Done." << endl;
    }

    int acquire() {

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

        ofstream output = ofstream(outputPath, ios::binary | ios::out);

        cout << "FPS = ... Hz T = ...*C";

        AT_64 imageHeight;
        AT_64 imageWidth;

        AT_GetInt(handle, L"AOI Height", &imageHeight);
        AT_GetInt(handle, L"AOI Width", &imageWidth);

        int frameRate = 2 * (int) getFloat(handle, "FrameRate");

        int size = imageHeight * imageWidth;

        for (int count = 1; running; count++) {

            unsigned char *buffer = writeQueue.pop();

            for (int i = 0; i < size; i++) {
                output << buffer[i];
            }

            delete[] buffer;

            if ((count % frameRate) == 0) {
                temperature = getFloat(handle, "SensorTemperature");
                cout << "\r\e[K" << std::flush;
                cout << "FPS = " << count / (time(0) - start) << " Hz" << ", T = " << temperature << "*C, " << "Write queue length = " << writeQueue.size();
            }

        }

        cout << endl;

        output.close();

        return 0;
    }
};
