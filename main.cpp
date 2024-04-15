#include <stdio.h>
#include "atcore.h"
#include "atutility.h"
#include <string>
#include "queue.cpp"
#include <thread>
#include <iostream>
#include <fstream>
#include <ctime>

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

            int result = AT_Command(handle, L"AcquisitionStop");

            // if (result != 0) {
            //     throw result;
            // }

            AT_Flush(handle);

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
            processThread = Thread(&A3C::process, this);
            writeThread   = Thread(&A3C::write, this);
            acquireThread = Thread(&A3C::acquire, this);

        }

        void stop() {

            // Change flag to stop, and wait for all threads to shutdown
            running = false;
            acquireThread.join();
            processThread.join();
            writeThread.join();

        }

        int acquire() {

            AT_64          atSize;
            unsigned char* pBuffer;
            int            size     = 0;

            AT_GetInt(handle, L"ImageSizeBytes", &atSize);

            imageSize = (int) atSize;

            AT_Command(handle, L"AcquisitionStart");

            while (running) {

                // Create new buffer, queue it and await data
                unsigned char* buffer = new unsigned char[imageSize];

                AT_QueueBuffer(handle, buffer, imageSize);
                AT_WaitBuffer(handle, &pBuffer, &size, AT_INFINITE);

                // Push the buffer into the processing queue
                processQueue.push(pBuffer);

                delete buffer;
                
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

            }

            return 0;

        }


        int write() {

            long time = std::time(0);

            AT_64 imageHeight;
            AT_64 imageWidth;
            AT_64 imageStride;

            AT_GetInt(handle, L"AOI Height", &imageHeight);
            AT_GetInt(handle, L"AOI Width", &imageWidth);
            AT_GetInt(handle, L"AOI Stride", &imageStride);

            ofstream output = ofstream(outputPath, ios::binary | ios::out);

            for (int i, j = 0; running; i++, j++) {

                unsigned char* buffer = writeQueue.pop();

                output << buffer;

            }

            output.close();

            return 0;

        }

};

int main() {

    try {

        cout << "A3Capture testing utility." << endl;

        cout << "Initialising..." << endl;

        A3C capture = A3C(1);

        capture.setOutputPath("data.bin");
        capture.start();

        cout << "Capturing, press enter to stop..." << endl;
        cin.ignore();

        capture.stop();

        return 0;

    } catch (int e) {

        cout << "Error encountered. Error Code: " << e <<  "." << endl;
        return e;

    }

}