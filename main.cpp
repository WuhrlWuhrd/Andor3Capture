#include <stdio.h>
#include "atcore.h"
#include "atutility.h"
#include <string>
#include "queue.cpp"
#include <thread>
#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>

#define Thread thread
#define String string

using namespace std;

class A3C {

    private:

        AT_H                       handle;
        String                     outputPath;
        int                        frameLimit;
        int                        imageSize;
        Thread                     acquireThread;
        Thread                     processThread;
        Thread                     writeThread;
        FIFOQueue<unsigned char*>  processQueue;
        FIFOQueue<unsigned char*>  writeQueue;
        
        bool running = false;

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
            cout << "Done."<< endl;

            cout << "Waiting for writing thread to terminate... ";
            writeThread.join();
            cout << "Done." << endl;

        }

        int acquire() {

            AT_64          atSize;
            unsigned char* pBuffer;
            int            size     = 0;

            AT_GetInt(handle, L"ImageSizeBytes", &atSize);

            imageSize = (int) atSize;

            AT_SetEnumString(handle, L"CycleMode", L"Continuous");
            AT_Command(handle, L"AcquisitionStart");

            for (int count = 1; running; count++) {

                // Create new buffer, queue it and await data
                unsigned char* buffer = new unsigned char[imageSize];

                int qCode = AT_QueueBuffer(handle, buffer, imageSize);
                int wCode = AT_WaitBuffer(handle, &pBuffer, &size, 10000);

                if (qCode != AT_SUCCESS || wCode != AT_SUCCESS) {
                    cout << "ERROR CODES: " << qCode << ", " << wCode << endl;
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

                unsigned char* buffer    = processQueue.pop();
                unsigned char* converted = new unsigned char[size];

                int k = 0;
                for (int i = 0; i < imageSize; i+= imageStride) {

                    for (int j = 0; j < imageWidth; j++) {

                        converted[k++] = buffer[i + j];

                    }

                }

                writeQueue.push(converted);

                delete buffer;

            }

            return 0;

        }


        int write() {

            long start = time(0);
            double temperature;

            ofstream output = ofstream(outputPath, ios::binary | ios::out);

            for (int count = 1; running; count++) {

                unsigned char* buffer = writeQueue.pop();

                output << buffer;

                delete buffer;

                if ((count % 10000) == 0) {
                    AT_GetFloat(handle, L"SensorTemperature", &temperature);
                    cout << "FPS = " << count / (time(0) - start) << " Hz" << endl;
                    cout << "T   = " << temperature << "*C" << endl;
                }

            }

            output.close();

            return 0;

        }

};

int main() {

    try {

        cout << "A3Capture testing utility." << endl;

        cout << "Initialising..." << endl;

        AT_InitialiseLibrary();

        AT_H handle;
        AT_Open(0, &handle);
        
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

        int result;

        int enumCount;
        AT_GetEnumCount(handle, L"PixelReadoutRate", &enumCount);

        for (int i = 0; i < enumCount; i++) {
            wchar_t chars[1024];
            AT_GetEnumStringByIndex(handle, L"PixelReadoutRate", i, chars, 1024);
            int available;
            int implemented;
            AT_IsEnumeratedIndexAvailable(handle, L"PixelReadoutRate", i, &available);
            AT_IsEnumeratedIndexImplemented(handle, L"PixelReadoutRate", i, &implemented);
            cout << "Option " << i << ": ";
            wcout << chars;
            cout << " Impl: " << implemented << " Avail: " << available << endl;
        }

        result = AT_SetEnumString(handle, L"AOILayout", L"Multitrack");
        cout << result << endl;
        result = AT_SetInt(handle, L"MultitrackCount", 1);
        cout << result << endl;
        result = AT_SetInt(handle, L"MultitrackSelector", 0);
        cout << result << endl;
        result = AT_SetInt(handle, L"MultitrackStart", 1);
        cout << result << endl;
        result = AT_SetInt(handle, L"MultitrackEnd", 1);
        cout << result << endl;
        result = AT_SetEnumIndex(handle, L"PixelReadoutRate", 3);
        cout << result << endl;
        result = AT_SetEnumeratedString(handle, L"ShutterMode", L"Open");
        cout << result << endl;
        result = AT_SetEnumeratedString(handle, L"FanSpeed", L"On");
        cout << result << endl;
        result = AT_SetEnumeratedString(handle, L"ElectronicShutteringMode", L"Rolling");
        cout << result << endl;
        result = AT_SetBool(handle, L"FastAOIFrameRateEnable", true);
        cout << result << endl;
        result = AT_SetBool(handle, L"Overlap", true);
        cout << result << endl;

        AT_SetFloat(handle, L"ExposureTime", 10e-6);
        cout << result << endl;
        AT_SetFloat(handle, L"FrameRate", 1900.0);
        cout << result << endl;

        double rate;
        AT_GetFloat(handle, L"FrameRate", &rate);

        double exp;
        AT_GetFloat(handle, L"ExposureTime", &exp);

        double rowTime;
        AT_GetFloat(handle, L"RowReadTime", &rowTime);

        double readTime;
        AT_GetFloat(handle, L"ReadoutTime", &readTime);

        double maxRate;
        AT_GetFloat(handle, L"MaxInterfaceTransferRate", &maxRate);

        int mode;
        AT_GetEnumIndex(handle, L"PixelReadoutRate", &mode);

        AT_64 accCount;
        AT_GetInt(handle, L"AccumulateCount", &accCount);

        cout << "Frame Rate: " << rate << " Hz" << endl;
        cout << "Max Frame Rate: " << maxRate << " Hz" << endl;
        cout << "Exposure Time: " << exp << " s" << endl;
        cout << "Accumulate Count: " << accCount << endl;
        cout << "Row Read Time: " << rowTime << " s" << endl;
        cout << "Readout Time: " << rowTime << " s" << endl;
        cout << "Readout Rate: " << mode << endl;

        A3C capture = A3C(handle);

        capture.setOutputPath("D:\\Paul\\data.bin");

        cout << "Ready, press enter to start..." << endl;
        cin.ignore();

        cout << "Starting threads... " << endl;
        capture.start();

        cout << "Capturing, press enter to stop..." << endl;
        cin.ignore();

        cout << "Stopping threads... " << endl;
        capture.stop();

        AT_Close(handle);

        AT_FinaliseLibrary();

        return 0;

    } catch (int e) {

        cout << "Error encountered. Error Code: " << e <<  "." << endl;

        AT_FinaliseLibrary();
        
        return e;

    }

}