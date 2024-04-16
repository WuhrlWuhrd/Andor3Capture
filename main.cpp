#include "atcore.h"
#include "atutility.h"
#include "queue.cpp"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <thread>

#define Thread thread
#define String string
#define WString wstring

using namespace std;

String wcToString(AT_WC* wc) {

    WString wString = WString(wc);
    return String(wString.begin(), wString.end());

}

AT_WC* stringToWC(String str) {

    size_t length = str.length();

    WString wString = WString(str.begin(), str.end());
    wString.resize(length);
    AT_WC* wc = new AT_WC[length + 1];
    wString.copy(wc, length, 0);
    wc[length] = L'\0';

    return wc;

}

long getInt(AT_H handle, String feature) {

    AT_64 value;
    int result = AT_GetInt(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

void setInt(AT_H handle, String feature, long value) {


    int result = AT_SetInt(handle, stringToWC(feature), value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

}

bool getBool(AT_H handle, String feature) {

    AT_BOOL value;
    int result = AT_GetBool(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

void setBool(AT_H handle, String feature, bool value) {


    int result = AT_SetBool(handle, stringToWC(feature), value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

}

double getFloat(AT_H handle, String feature) {

    double value;
    int result = AT_GetFloat(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

double getFloatMin(AT_H handle, String feature) {

    double value;
    int result = AT_GetFloatMin(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

void setFloat(AT_H handle, String feature, double value) {


    int result = AT_SetFloat(handle, stringToWC(feature), value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

}

int getEnumInt(AT_H handle, String feature) {

    int value;
    int result = AT_GetEnumIndex(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

void setEnumInt(AT_H handle, String feature, int index) {


    int result = AT_SetEnumIndex(handle, stringToWC(feature), index);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

}

String getEnum(AT_H handle, String feature) {

    int    value  = getEnumInt(handle, feature);
    AT_WC* chars  = new AT_WC[1024];
    int    result = AT_GetEnumStringByIndex(handle, stringToWC(feature), value, chars, 1024);

    return wcToString(chars);

}

void setEnum(AT_H handle, String feature, String value) {


    int result = AT_SetEnumString(handle, stringToWC(feature), stringToWC(value));

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << " = " << value << ": " << result;
        throw oss.str();
    }

}

void printEnum(AT_H handle, String feature) {

    AT_WC* f = stringToWC(feature);

    int count;
    AT_GetEnumCount(handle, f, &count);

    for (int i = 0; i < count; i ++) {
        AT_WC chars[1024];
        AT_GetEnumStringByIndex(handle, f, i, chars, 1024);
        AT_BOOL avail;
        AT_BOOL impl;
        AT_IsEnumIndexAvailable(handle, f, i, &avail);
        AT_IsEnumIndexImplemented(handle, f, i, &impl);

        cout << "Index " << i << ": " << wcToString(chars) << " (" << (impl ? "Implemented" : "Unimplemented") << ", " << (avail ? "Available" : "Unavailable") << ")" << endl;

    }

}

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
                AT_GetFloat(handle, L"SensorTemperature", &temperature);
                cout << "\r\e[K" << std::flush;
                cout << "FPS = " << count / (time(0) - start) << " Hz" << ", T = " << temperature << "*C";
            }

        }

        cout << endl;

        output.close();

        return 0;
    }
};

int main() {

    try {

        cout << "A3Capture testing utility." << endl;

        cout << "Initialising... ";
        AT_InitialiseLibrary();
        cout << "Done." << endl;

        cout << "Opening camera... ";
        AT_H handle;
        AT_Open(0, &handle);
        cout << "Done." << endl;

        AT_SetBool(handle, L"SensorCooling", true);

        int status = 0;
        double temperature;

        cout << "Waiting for temperature to stabilise... " << endl;

        while (false) {

            AT_GetEnumIndex(handle, L"TemperatureStatus", &status);
            AT_GetFloat(handle, L"SensorTemperature", &temperature);

            cout << "T = " << temperature << "*C" << endl;

            this_thread::sleep_for(chrono::milliseconds(1000));

        }

        cout << "Stabilised." << endl;

        setEnum(handle, "AOILayout", "Multitrack");
        setInt(handle, "MultitrackCount", 1);
        setInt(handle, "MultitrackSelector", 0);
        setInt(handle, "MultitrackStart", 1024);
        setInt(handle, "MultitrackEnd", 1024);
        setBool(handle, "MultitrackBinned", false);
        setEnum(handle, "PixelReadoutRate", "270 MHz");
        setEnum(handle, "TriggerMode", "Internal");
        setEnum(handle, "ShutterMode", "Open");
        setEnum(handle, "FanSpeed", "On");
        setBool(handle, "RollingShutterGlobalClear", false);
        setEnum(handle, "ElectronicShutteringMode", "Rolling");
        setBool(handle, "FastAOIFrameRateEnable", true);
        setBool(handle, "Overlap", true);
        double minExp = getFloatMin(handle, "ExposureTime");
        setFloat(handle, "ExposureTime", minExp);

        double rate     = getFloat(handle, "FrameRate");
        double exp      = getFloat(handle, "ExposureTime");
        double rowTime  = getFloat(handle, "RowReadTime");
        double readTime = getFloat(handle, "ReadoutTime");
        double maxRate  = getFloat(handle, "MaxInterfaceTransferRate");
        String mode     = getEnum(handle, "PixelReadoutRate");
        String shutter  = getEnum(handle, "ElectronicShutteringMode");
        long   accCount = getInt(handle, "AccumulateCount");

        cout << "Frame Rate: " << rate << " Hz" << endl;
        cout << "Max Frame Rate: " << maxRate << " Hz" << endl;
        cout << "Exposure Time: " << exp << " s" << endl;
        cout << "Min Exposure Time: " << minExp << " s" << endl;
        cout << "Accumulate Count: " << accCount << endl;
        cout << "Row Read Time: " << rowTime << " s" << endl;
        cout << "Readout Time: " << rowTime << " s" << endl;
        cout << "Readout Rate: " << mode << endl;
        cout << "Shuttering Mode: " << shutter << endl;

        A3C capture = A3C(handle);

        capture.setOutputPath("D:\\Paul\\data.bin");

        cout << "Ready, press enter to start..." << endl;
        cin.ignore();

        capture.start();

        cout << "Capturing, press enter to stop..." << endl;
        cin.ignore();

        capture.stop();

        AT_Close(handle);

        AT_FinaliseLibrary();

        return 0;

    } catch (String e) {

        cerr << "Error encountered. Error Code: " << e << "." << endl;

        AT_FinaliseLibrary();

        return 1;
    }
}