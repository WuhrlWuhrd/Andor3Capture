#pragma once
#include "atfunc.cpp"
#include "queue.cpp"
#include <ctime>
#include <fstream>
#include <thread>
#include <vector>

using namespace std;

/// @brief This class is for controlling an Andor SDK3 camera to rapidly take,
/// process, and store frames, making use of multi-threading. It does this by
/// running four threads: 
///
/// (1) Acquisition, which runs the camera
///
/// (2) Processing, which sequentially takes each image captured by thread #1 and
///     performs necessary processing (i.e., removing padding, extracting metadata
///     etc)
///
/// (3) writing, which takes the output of (2) and writes it directly to disk, 
///     allowing its footprint in memory to freed
/// 
/// (4) monitoring, which keeps an eye on the other three threads and reports their 
///     performance to the user.
///
/// There are two "FIFO" (First-In, First-Out) queues in-between threads #1-#2 and
/// #2-#3. These are effectively pipelines between the threads, allowing for data
/// flow between the threads in an orderly manner. If there is a bottleneck in the
/// system (for instance, the writing thread only be able to write to disk at a
/// fraction of the speed that frames are coming in), then one of both of these 
/// queues will start to grow in size, and cause memory usage to steadily climb.
class A3C {

private:

    AT_H   handle;
    string outputPath;
    int    frameLimit;
    int    imageSize;

    thread acquireThread;
    thread processThread;
    thread writeThread;
    thread monitorThread;

    FIFOQueue<unsigned char *>  processQueue;
    FIFOQueue<unsigned short *> writeQueue;

    vector<string> errors;

    ostream* out          = &cout;
    bool     running      = false;
    bool     monitoring   = false;
    long     acquireCount = 0;
    long     processCount = 0;
    long     writeCount   = 0;

public:

    A3C(const A3C& other) {
        this->handle     = other.handle;
        this->outputPath = other.outputPath;
        this->frameLimit = other.frameLimit;
    }

    A3C(AT_H handle) {

        this->handle     = handle;
        this->outputPath = "output.h5";
        this->frameLimit = -1;

        // Check that the camera is connected
        int result = AT_Flush(handle);

        if (result != AT_SUCCESS) {
            throw result;
        }
        
    }

    void setVerbose(bool flag) {
        out = flag ? &cout : new ostream(0);
    }

    void setFrameLimit(int limit) {
        frameLimit = limit;
    }

    void setOutputPath(string path) {
        outputPath = path;
    }

    int getFrameLimit() {
        return frameLimit;
    }

    string getOutputPath() {
        return outputPath;
    }

    void start() {

        // Set both flags to true so that loops do the looping
        running    = true;
        monitoring = true;

        // Clear both queues
        processQueue.clear();
        writeQueue.clear();

        // Set all threads running
        *out << "Starting writing thread... ";
        writeThread = thread(&A3C::write, this);
        *out << "Done." << endl;

        *out << "Starting processing thread... ";
        processThread = thread(&A3C::process, this);
        *out << "Done." << endl;

        *out << "Starting acquisition thread... ";
        acquireThread = thread(&A3C::acquire, this);
        *out << "Done." << endl;

        *out << "Starting monitoring thread... ";
        monitorThread = thread(&A3C::monitor, this);
        *out << "Done." << endl;
        
    }

    void stop() {

        // Indicate to all threads that we wish to stop
        running = false;

        // Wait for the acquisition thread to stop
        acquireThread.join();

        // Push a null pointer to the processing queue, in case it is currently
        // blocking then wait for it to finish
        processQueue.push(nullptr);
        processThread.join();

        // Same for writing queue
        writeQueue.push(nullptr);
        writeThread.join();

        // Tell the monitoring thread to stop and wait for it to do so
        monitoring = false;
        monitorThread.join();

    }

    int acquire() {

        // Declare variables
        unsigned char* pBuffer;
        int            size = 0;
        long           start = time(0);
        double         temperature;

        // Calculate timeout to use for acquisitions in ms (min 500 ms)
        float  frameRate  = getFloat(handle, "FrameRate");
        float  frameTime  = 1.0 / frameRate;
        long   time       = (long) (1000.0 * 2.0 * frameTime);
        long   timeOut    = time > 500 ? time : 500;

        // Tell the camera to include metadata and to continuously capture
        setBool(handle, "MetadataEnable", true);
        setBool(handle, "MetadataFrameInfo", true);
        setBool(handle, "MetadataTimestamp", true);
        setBool(handle, "MetadataEnable", true);
        setEnum(handle, "CycleMode", "Continuous");

        // Query camera for buffer size
        imageSize = getInt(handle, "ImageSizeBytes");

        // Start the acquisition
        AT_Command(handle, L"AcquisitionStart");

        for (acquireCount = 0; running; acquireCount++) {

            // Create new buffer, queue it and await data
            unsigned char *buffer = new unsigned char[imageSize];
            int            qCode  = AT_QueueBuffer(handle, buffer, imageSize);
            int            wCode  = AT_WaitBuffer(handle, &pBuffer, &size, timeOut);

            // If there was an error, record it and restart acquisition
            if (qCode != AT_SUCCESS || wCode != AT_SUCCESS) {

                if (wCode == 13) {
                    errors.push_back("Acquisition Time-Out (" + to_string(qCode) + ", " + to_string(wCode) + "), restarting acquisition.");
                } else {
                    errors.push_back("Error (" + to_string(qCode) + ", " + to_string(wCode) + "), restarting acquisition.");
                }

                AT_Command(handle, L"AcquisitionStop");
                AT_Flush(handle);
                AT_Command(handle, L"AcquisitionStart");

                continue;

            }

            // Push the buffer into the processing queue
            processQueue.push(pBuffer);

        }

        // Stop acquisition and flush through any remaining buffers
        AT_Command(handle, L"AcquisitionStop");
        AT_Flush(handle);

        return 0;

    }

    int process() {

        // Keep going so long as we're runnning, or there's still frames to process in the queue
        for (processCount = 0; running || processQueue.hasWaiting(); processCount++) {

            // Get next frame in queue, if none present then this blocks (waits) until there is one
            unsigned char *buffer = processQueue.pop();

            // If we've been given a nullt pointer, ignore it
            if (buffer == nullptr) {
                continue;
            }

            // Extract timestamp and image size from meta data
            AT_64 timestamp;
            AT_64 imageHeight;
            AT_64 imageWidth;
            AT_GetTimeStampFromMetadata(buffer, imageSize, timestamp);
            AT_GetWidthFromMetadata(buffer, imageSize, imageWidth);
            AT_GetHeightFromMetadata(buffer, imageSize, imageHeight);

            int size = imageWidth * imageHeight;

            // Create buffer for processed image
            unsigned short *converted = new unsigned short[size];

            // Convert image into an array of shorts (i.e., 16-bit integers) without padding etc
            AT_ConvertBufferUsingMetadata(buffer, (unsigned char *)converted, imageSize, L"Mono16");

            // Push to the converted image back of the write queue
            writeQueue.push(converted);

            // Free the original image's memory
            delete[] buffer;
        }

        return 0;

    }

    int write() {

        remove(outputPath.c_str());

        ofstream output = ofstream(outputPath, ios::binary | ios::out);

        long imageHeight = getInt(handle, "AOIHeight");
        long imageWidth  = getInt(handle, "AOIWidth");
        long size        = imageHeight * imageWidth;

        for (writeCount = 0; running || writeQueue.hasWaiting(); writeCount++) {

            unsigned short *buffer = writeQueue.pop();

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

        long last = time(0);
        long lastAcquireCount = 0;
        long lastProcessCount = 0;
        long lastWriteCount   = 0;

        this_thread::sleep_for(chrono::seconds(1));

        *out << "...";

        while (monitoring) {

            if (running) {

                long   duration = time(0) - last;
                double aRate    = (acquireCount - lastAcquireCount) / duration;
                double pRate    = (processCount - lastProcessCount) / duration;
                double wRate    = (writeCount - lastWriteCount) / duration;
                double temp     = getFloat(handle, "SensorTemperature");
                int    pQueue   = processQueue.size();
                int    wQueue   = writeQueue.size();

                lastAcquireCount = acquireCount;
                lastProcessCount = processCount;
                lastWriteCount   = writeCount;
                last             = time(0);

                *out << "\r\e[K"
                    << "A = " << aRate << " Hz, P = " << pRate << " Hz, "
                    << ", W = " << wRate << " Hz, PQ = " << pQueue << ", WQ = " << wQueue;

            } else {

                *out << "\r\e[K" << "Stopping threads: Left to Process = " << processQueue.size() << ", Left to Write = " << writeQueue.size();

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
