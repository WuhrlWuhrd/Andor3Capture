#include "atfunc.cpp"
#include "queue.cpp"
#include <map>
#include <ctime>
#include <fstream>
#include <thread>
#include <vector>
#include <iterator>

bool initialised = false;

std::map<int, std::string> errorNames {

    {AT_SUCCESS, "SUCCESS"},
    {AT_ERR_NOTINITIALISED, "ERR_NOTINITIALISED"},
    {AT_ERR_NOTIMPLEMENTED, "ERR_NOTIMPLEMENTED"},
    {AT_ERR_READONLY, "ERR_READONLY"},
    {AT_ERR_NOTREADABLE, "ERR_NOTREADABLE"},
    {AT_ERR_NOTWRITABLE, "ERR_NOTWRITABLE"},
    {AT_ERR_OUTOFRANGE, "ERR_OUTOFRANGE"},
    {AT_ERR_INDEXNOTAVAILABLE, "ERR_INDEXNOTAVAILABLE"},
    {AT_ERR_INDEXNOTIMPLEMENTED, "ERR_INDEXNOTIMPLEMENTED"},
    {AT_ERR_EXCEEDEDMAXSTRINGLENGTH, "ERR_EXCEEDEDMAXSTRINGLENGTH"},
    {AT_ERR_CONNECTION, "ERR_CONNECTION"},
    {AT_ERR_NODATA, "ERR_NODATA"},
    {AT_ERR_INVALIDHANDLE, "ERR_INVALIDHANDLE"},
    {AT_ERR_TIMEDOUT, "ERR_TIMEDOUT"},
    {AT_ERR_BUFFERFULL, "ERR_BUFFERFULL"},
    {AT_ERR_INVALIDSIZE, "ERR_INVALIDSIZE"},
    {AT_ERR_INVALIDALIGNMENT, "ERR_INVALIDALIGNMENT"},
    {AT_ERR_COMM, "ERR_COMM"},
    {AT_ERR_STRINGNOTAVAILABLE, "ERR_STRINGNOTAVAILABLE"},
    {AT_ERR_STRINGNOTIMPLEMENTED, "ERR_STRINGNOTIMPLEMENTED"},
    {AT_ERR_NULL_FEATURE, "ERR_NULL_FEATURE"},
    {AT_ERR_NULL_HANDLE, "ERR_NULL_HANDLE"},
    {AT_ERR_NULL_IMPLEMENTED_VAR, "ERR_NULL_IMPLEMENTED_VAR"},
    {AT_ERR_NULL_READABLE_VAR, "ERR_NULL_READABLE_VAR"},
    {AT_ERR_NULL_READONLY_VAR, "ERR_NULL_READONLY_VAR"},
    {AT_ERR_NULL_WRITABLE_VAR, "ERR_NULL_WRITABLE_VAR"},
    {AT_ERR_NULL_MINVALUE, "ERR_NULL_MINVALUE"},
    {AT_ERR_NULL_MAXVALUE, "ERR_NULL_MAXVALUE"},
    {AT_ERR_NULL_VALUE, "ERR_NULL_VALUE"},
    {AT_ERR_NULL_STRING, "ERR_NULL_STRING"},
    {AT_ERR_NULL_COUNT_VAR, "ERR_NULL_COUNT_VAR"},
    {AT_ERR_NULL_ISAVAILABLE_VAR, "ERR_NULL_ISAVAILABLE_VAR"},
    {AT_ERR_NULL_MAXSTRINGLENGTH, "ERR_NULL_MAXSTRINGLENGTH"},
    {AT_ERR_NULL_EVCALLBACK, "ERR_NULL_EVCALLBACK"},
    {AT_ERR_NULL_QUEUE_PTR, "ERR_NULL_QUEUE_PTR"},
    {AT_ERR_NULL_WAIT_PTR, "ERR_NULL_WAIT_PTR"},
    {AT_ERR_NULL_PTRSIZE, "ERR_NULL_PTRSIZE"},
    {AT_ERR_NOMEMORY, "ERR_NOMEMORY"},
    {AT_ERR_DEVICEINUSE, "ERR_DEVICEINUSE"},
    {AT_ERR_DEVICENOTFOUND, "ERR_DEVICENOTFOUND"},
    {AT_ERR_HARDWARE_OVERFLOW, "ERR_HARDWARE_OVERFLOW"}

};

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
    long     acquireFPS   = 0;
    long     processFPS   = 0;
    long     writeFPS     = 0;

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
            throw errorNames[result];
        }
        
    }

    void setVerbose(bool flag) {
        out = flag ? &cout : new ostream(0);
    }

    void setFrameLimit(int limit) {
        frameLimit = limit;
    }

    void setOutputPath(std::string path) {
        outputPath = path;
    }

    int getFrameLimit() {
        return frameLimit;
    }

    std::string getOutputPath() {
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

        for (acquireCount = 0; running && (frameLimit <= 0 || acquireCount < frameLimit); acquireCount++) {

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

        running = false;

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


                acquireFPS = aRate;
                processFPS = pRate;
                writeFPS   = wRate;

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

            out->flush();

            this_thread::sleep_for(chrono::seconds(1));

        }

        *out << "\r\e[K" << "Shutdown complete." << endl;
        *out << "Total frames written to disk: " << writeCount << endl;

        return 0;

    }

    long getAcquireFPS() {
        return acquireFPS;
    }

    long getProcessFPS() {
        return processFPS;
    }

    long getWriteFPS() {
        return writeFPS;
    }

    long getProcessQueueSize() {
        return processQueue.size();
    }

    long getWriteQueueSize() {
        return writeQueue.size();
    }

    long getAcquireCount() {
        return acquireCount;
    }

    bool isRunning() {
        return running;
    }

    bool isMonitoring() {
        return monitoring;
    }
};

class Track {

    private:

    long start;
    long end;
    bool binned;

   public:

    Track(const Track& other) {

        this->start      = other.start;
        this->end        = other.end;
        this->binned     = other.binned;

    }

    Track(long start, long end, bool binned) {

        this->start      = start;
        this->end        = end;
        this->binned     = binned;

    }

    long getStart() {
        return start;
    }

    long getEnd() {
        return end;
    }

    bool isBinned() {
        return binned;
    }

    void setStart(long value) {
        start = value;
    }

    void setEnd(long value) {
        end = value;
    }

    void setBinned(bool value) {
        binned = value;
    }

};

class Acquisition {
    
    private:

    unsigned short* data;
    long width  = 0;
    long height = 0;
    long size   = 0;

    public:
     
    Acquisition(unsigned short* data, long width, long height) {
        this->data   = data;
        this->width  = width;
        this->height = height;
        this->size   = width * height;
    }

    Acquisition(const Acquisition& other) {
        this->width  = other.width;
        this->height = other.height;
        this->size   = other.size;
        this->data   = new unsigned short[size];

        for (int i = 0; i < size; i++) {
            data[i] = other.data[i];
        }

    }

    ~Acquisition() {
        delete[] data;
    }

    long getWidth() {
        return width;
    }

    long getHeight() {
        return height;
    }

    long getSize() {
        return size;
    }

    std::vector<unsigned short> getArray() {
        return std::vector<unsigned short>(data, data + size);
    }

    unsigned short getPixel(int row, int column) {

        if (row < 0 || row >= height) {
            throw std::string("Row index out of range");
        }

        if (column < 0 || column >= width) {
            throw std::string("Column index out of range");
        }

        return data[row * width + column];

    }

    unsigned short operator()(int row, int column) {
        return getPixel(row, column);
    }

    static Acquisition example() {

        unsigned short* array = new unsigned short[8];

        for (int i = 0; i < 8; i++){
            array[i] = i + 1;
        }

        return Acquisition(array, 2, 4);

    }

};

class Zyla {

    private:

    AT_H handle;
    A3C  *pipeline;

    public:

    Zyla(int index) {
        
        if (!initialised) {
            AT_InitialiseLibrary();
            AT_InitialiseUtilityLibrary();
            initialised = true;
        }

        int result = AT_Open(index, &handle);


        if (result != AT_SUCCESS) {
            throw errorNames[result];
        }

        pipeline = new A3C(handle);

    }

    ~Zyla() {
        delete pipeline;
    }

    A3C capturePipeline() {
        return *pipeline;
    }

    long getInt(std::string feature) {

        AT_64 value;
        int result = AT_GetInt(handle, stringToWC(feature), &value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return value;

    }

    void setInt(std::string feature, long value) {

        int result = AT_SetInt(handle, stringToWC(feature), value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

    }

    long getIntMin(std::string feature) {

        AT_64 value;
        int result = AT_GetIntMin(handle, stringToWC(feature), &value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return value;

    }

    long getIntMax(std::string feature) {

        AT_64 value;
        int result = AT_GetIntMax(handle, stringToWC(feature), &value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return value;

    }

    bool getBool(std::string feature) {

        AT_BOOL value;
        int result = AT_GetBool(handle, stringToWC(feature), &value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return value;

    }

    void setBool(std::string feature, bool value) {

        int result = AT_SetBool(handle, stringToWC(feature), value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

    }

    double getFloat(std::string feature) {

        double value;
        int result = AT_GetFloat(handle, stringToWC(feature), &value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return value;

    }

    double getFloatMin(std::string feature) {

        double value;
        int result = AT_GetFloatMin(handle, stringToWC(feature), &value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return value;

    }

    double getFloatMax(std::string feature) {

        double value;
        int result = AT_GetFloatMax(handle, stringToWC(feature), &value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return value;

    }

    void setFloat(std::string feature, double value) {

        int result = AT_SetFloat(handle, stringToWC(feature), value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

    }

    int getEnumInt(std::string feature) {

        int value;
        int result = AT_GetEnumIndex(handle, stringToWC(feature), &value);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return value;

    }

    void setEnumInt(std::string feature, int index) {

        int result = AT_SetEnumIndex(handle, stringToWC(feature), index);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }
        
    }

    std::string getEnum(std::string feature) {

        int    value  = getEnumInt(feature);
        AT_WC* chars  = new AT_WC[1024];
        int    result = AT_GetEnumStringByIndex(handle, stringToWC(feature), value, chars, 1024);

        return wcTostring(chars);

    }

    std::string getString(std::string feature) {

        AT_WC* chars  = new AT_WC[1024];
        int    result = AT_GetString(handle, stringToWC(feature), chars, 1024);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return wcTostring(chars);

    }

    void setEnum(std::string feature, std::string value) {

        int result = AT_SetEnumString(handle, stringToWC(feature), stringToWC(value));

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << " = " << value << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

    }

    void command(std::string feature) {

        int result = AT_Command(handle, stringToWC(feature));

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

    }

    std::map<int, std::string> getEnumOptions(std::string feature) {

        AT_WC* ft = stringToWC(feature);

        int count;
        int result = AT_GetEnumeratedCount(handle, ft, &count);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << feature << ": " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        std::map<int, std::string> options;

        for (int i = 0; i < count; i++) {
            AT_BOOL available;
            AT_BOOL implemented;

            int result1 = AT_IsEnumeratedIndexAvailable(handle, ft, i, &available);
            int result2 =
                AT_IsEnumeratedIndexImplemented(handle, ft, i, &implemented);

            if (result1 != AT_SUCCESS || result2 != AT_SUCCESS) {
                continue;
            }

            if (available && implemented) {
                AT_WC chars[1024];
                AT_GetEnumStringByIndex(handle, ft, i, chars, 1024);
                options[i] = wcTostring(chars);
            }
        }

        return options;

    }

    void queueBuffer(unsigned char buffer[], int bufferSize) { 

        int result = AT_QueueBuffer(handle, buffer, bufferSize);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << "Queuing Buffer: " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

    }

    void queueBuffer(int size) {
        queueBuffer(new unsigned char[size], size);
    }

    void queueBuffer() {
        int size = getImageSizeBytes();
        queueBuffer(new unsigned char[size], size);
    }

    unsigned char* awaitBuffer(unsigned int timeout, int* size) {

        unsigned char* pointer;

        int result = AT_WaitBuffer(handle, &pointer, size, timeout);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << "Awaiting Buffer: " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return pointer;

    }

    unsigned char* acquireRaw(int timeout, int* size) {

        queueBuffer();
        acquisitionStart();

        unsigned char* buffer = awaitBuffer(timeout, size);

        acquisitionStop();
        flush();

        return buffer;

    }

    Acquisition acquire(int timeout) {

        long width      = getAOIWidth();
        long height     = getAOIHeight();
        long stride     = getAOIStride();
        std::string enc = getPixelEncodingOptions()[getPixelEncoding()];

        int size;
        unsigned char* pointer = acquireRaw(timeout, &size);
        unsigned char* output  = new unsigned char[2 * width * height];

        int result = AT_ConvertBuffer(pointer, output, width, height, stride, stringToWC(enc), L"Mono16");

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << "Acquiring Image: " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

        return Acquisition((unsigned short*) output, width, height);
        
    }

    long getAccumulateCount() {
        return getInt("AccumulateCount");
    }

    void setAccumulateCount(long value) {
        setInt("AccumulateCount", value);
    }

    long getAccumulateCountMin() {
        return getIntMin("AccumulateCount");
    }

    long getAccumulateCountMax() {
        return getIntMax("AccumulateCount");
    }

    void acquisitionStart() {
        command("AcquisitionStart");
    }

    void acquisitionStop() {
        command("AcquisitionStop");
    }

    void flush() {

        int result = AT_Flush(handle);

        if (result != AT_SUCCESS) {
            ostringstream oss;
            oss << "Flushing Buffers: " << errorNames[result] << " (" << result << ")";
            throw oss.str();
        }

    }

    bool isAlternatingReadoutDirection() {
        return getBool("AlternatingReadoutDirection");
    }

    void setAlternatingReadoutDirection(bool value) {
        setBool("AlternatingReadoutDirection", value);
    }

    int getAOIBinning() {
        return getEnumInt("AOIBinning");
    }

    void setAOIBinning(int index) {
        setEnumInt("AOIBinning", index);
    }

    std::map<int, std::string> getAOIBinningOptions() {
        return getEnumOptions("AOIBinning");
    }

    long getAOIHBin() {
        return getInt("AOIHBin");
    }

    void setAOIHBin(long value) {
        setInt("AOIHBin", value);
    }

    long getAOIHBinMin() {
        return getIntMin("AOIHBin");
    }

    long getAOIHBinMax() {
        return getIntMax("AOIHBin");
    }

    long getAOIHeight() {
        return getInt("AOIHeight");
    }

    void setAOIHeight(long value) {
        setInt("AOIHeight", value);
    }

    long getAOIHeightMin() {
        return getIntMin("AOIHeight");
    }

    long getAOIHeightMax() {
        return getIntMax("AOIHeight");
    }

    int getAOILayout() {
        return getEnumInt("AOILayout");
    }

    void setAOILayout(int index) {
        setEnumInt("AOILayout", index);
    }

    std::map<int, std::string> getAOILayoutOptions() {
        return getEnumOptions("AOILayout");
    }

    long getAOILeft() {
        return getInt("AOILeft");
    }

    void setAOILeft(long value) {
        setInt("AOILeft", value);
    }

    long getAOILeftMin() {
        return getIntMin("AOILeft");
    }

    long getAOILeftMax() {
        return getIntMax("AOILeft");
    }

    long getAOIStride() {
        return getInt("AOIStride");
    }

    void setAOIStride(long value) {
        setInt("AOIStride", value);
    }

    long getAOIStrideMin() {
        return getIntMin("AOIStride");
    }

    long getAOIStrideMax() {
        return getIntMax("AOIStride");
    }

    long getAOITop() {
        return getInt("AOITop");
    }

    void setAOITop(long value) {
        setInt("AOITop", value);
    }

    long getAOITopMin() {
        return getIntMin("AOITop");
    }

    long getAOITopMax() {
        return getIntMax("AOITop");
    }

    long getAOIVBin() {
        return getInt("AOIVBin");
    }

    void setAOIVBin(long value) {
        setInt("AOIVBin", value);
    }

    long getAOIVBinMin() {
        return getIntMin("AOIVBin");
    }

    long getAOIVBinMax() {
        return getIntMax("AOIVBin");
    }

    long getAOIWidth() {
        return getInt("AOIWidth");
    }

    void setAOIWidth(long value) {
        setInt("AOIWidth", value);
    }

    long getAOIWidthMin() {
        return getIntMin("AOIWidth");
    }

    long getAOIWidthMax() {
        return getIntMax("AOIWidth");
    }

    int getAuxiliaryOutSource() {
        return getEnumInt("AuxiliaryOutSource");
    }

    void setAuxiliaryOutSource(int index) {
        setEnumInt("AuxiliaryOutSource", index);
    }

    std::map<int, std::string> getAuxiliaryOutSourceOptions() {
        return getEnumOptions("AuxiliaryOutSource");
    }

    int getAuxOutSourceTwo() {
        return getEnumInt("AuxOutSourceTwo");
    }

    void setAuxOutSourceTwo(int index) {
        setEnumInt("AuxOutSourceTwo", index);
    }

    std::map<int, std::string> getAuxOutSourceTwoOptions() {
        return getEnumOptions("AuxOutSourceTwo");
    }

    long getBaseline() {
        return getInt("Baseline");
    }

    void setBaseline(long value) {
        setInt("Baseline", value);
    }

    long getBaselineMin() {
        return getIntMin("Baseline");
    }

    long getBaselineMax() {
        return getIntMax("Baseline");
    }

    int getBitDepth() {
        return getEnumInt("BitDepth");
    }

    void setBitDepth(int index) {
        setEnumInt("BitDepth", index);
    }

    std::map<int, std::string> getBitDepthOptions() {
        return getEnumOptions("BitDepth");
    }

    long getBufferOverflowEvent() {
        return getInt("BufferOverflowEvent");
    }

    void setBufferOverflowEvent(long value) {
        setInt("BufferOverflowEvent", value);
    }

    long getBufferOverflowEventMin() {
        return getIntMin("BufferOverflowEvent");
    }

    long getBufferOverflowEventMax() {
        return getIntMax("BufferOverflowEvent");
    }

    double getBytesPerPixel() {
        return getFloat("BytesPerPixel");
    }

    void setBytesPerPixel(double value) {
        setFloat("BytesPerPixel", value);
    }

    double getBytesPerPixelMin() {
        return getFloatMin("BytesPerPixel");
    }

    double getBytesPerPixelMax() {
        return getFloatMax("BytesPerPixel");
    }

    bool isCameraAcquiring() {
        return getBool("CameraAcquiring");
    }

    void setCameraAcquiring(bool value) {
        setBool("CameraAcquiring", value);
    }

    bool isCameraPresent() {
        return getBool("CameraPresent");
    }

    void setCameraPresent(bool value) {
        setBool("CameraPresent", value);
    }

    std::string getControllerID() {
        return getString("ControllerID");
    }

    double getCoolerPower() {
        return getFloat("CoolerPower");
    }

    void setCoolerPower(double value) {
        setFloat("CoolerPower", value);
    }

    double getCoolerPowerMin() {
        return getFloatMin("CoolerPower");
    }

    double getCoolerPowerMax() {
        return getFloatMax("CoolerPower");
    }

    int getCycleMode() {
        return getEnumInt("CycleMode");
    }

    void setCycleMode(int index) {
        setEnumInt("CycleMode", index);
    }

    std::map<int, std::string> getCycleModeOptions() {
        return getEnumOptions("CycleMode");
    }

    long getDeviceCount() {
        return getInt("DeviceCount");
    }

    void setDeviceCount(long value) {
        setInt("DeviceCount", value);
    }

    long getDeviceCountMin() {
        return getIntMin("DeviceCount");
    }

    long getDeviceCountMax() {
        return getIntMax("DeviceCount");
    }

    long getDeviceVideoIndex() {
        return getInt("DeviceVideoIndex");
    }

    void setDeviceVideoIndex(long value) {
        setInt("DeviceVideoIndex", value);
    }

    long getDeviceVideoIndexMin() {
        return getIntMin("DeviceVideoIndex");
    }

    long getDeviceVideoIndexMax() {
        return getIntMax("DeviceVideoIndex");
    }

    int getElectronicShutteringMode() {
        return getEnumInt("ElectronicShutteringMode");
    }

    void setElectronicShutteringMode(int index) {
        setEnumInt("ElectronicShutteringMode", index);
    }

    std::map<int, std::string> getElectronicShutteringModeOptions() {
        return getEnumOptions("ElectronicShutteringMode");
    }

    bool isEventEnable() {
        return getBool("EventEnable");
    }

    void setEventEnable(bool value) {
        setBool("EventEnable", value);
    }

    int getEventSelector() {
        return getEnumInt("EventSelector");
    }

    void setEventSelector(int index) {
        setEnumInt("EventSelector", index);
    }

    std::map<int, std::string> getEventSelectorOptions() {
        return getEnumOptions("EventSelector");
    }

    long getEventsMissedEvent() {
        return getInt("EventsMissedEvent");
    }

    void setEventsMissedEvent(long value) {
        setInt("EventsMissedEvent", value);
    }

    long getEventsMissedEventMin() {
        return getIntMin("EventsMissedEvent");
    }

    long getEventsMissedEventMax() {
        return getIntMax("EventsMissedEvent");
    }

    long getExposedPixelHeight() {
        return getInt("ExposedPixelHeight");
    }

    void setExposedPixelHeight(long value) {
        setInt("ExposedPixelHeight", value);
    }

    long getExposedPixelHeightMin() {
        return getIntMin("ExposedPixelHeight");
    }

    long getExposedPixelHeightMax() {
        return getIntMax("ExposedPixelHeight");
    }

    long getExposureEndEvent() {
        return getInt("ExposureEndEvent");
    }

    void setExposureEndEvent(long value) {
        setInt("ExposureEndEvent", value);
    }

    long getExposureEndEventMin() {
        return getIntMin("ExposureEndEvent");
    }

    long getExposureEndEventMax() {
        return getIntMax("ExposureEndEvent");
    }

    long getExposureStartEvent() {
        return getInt("ExposureStartEvent");
    }

    void setExposureStartEvent(long value) {
        setInt("ExposureStartEvent", value);
    }

    long getExposureStartEventMin() {
        return getIntMin("ExposureStartEvent");
    }

    long getExposureStartEventMax() {
        return getIntMax("ExposureStartEvent");
    }

    double getExposureTime() {
        return getFloat("ExposureTime");
    }

    void setExposureTime(double value) {
        setFloat("ExposureTime", value);
    }

    double getExposureTimeMin() {
        return getFloatMin("ExposureTime");
    }

    double getExposureTimeMax() {
        return getFloatMax("ExposureTime");
    }

    double getExternalTriggerDelay() {
        return getFloat("ExternalTriggerDelay");
    }

    void setExternalTriggerDelay(double value) {
        setFloat("ExternalTriggerDelay", value);
    }

    double getExternalTriggerDelayMin() {
        return getFloatMin("ExternalTriggerDelay");
    }

    double getExternalTriggerDelayMax() {
        return getFloatMax("ExternalTriggerDelay");
    }

    int getFanSpeed() {
        return getEnumInt("FanSpeed");
    }

    void setFanSpeed(int index) {
        setEnumInt("FanSpeed", index);
    }

    std::map<int, std::string> getFanSpeedOptions() {
        return getEnumOptions("FanSpeed");
    }

    bool isFastAOIFrameRateEnable() {
        return getBool("FastAOIFrameRateEnable");
    }

    void setFastAOIFrameRateEnable(bool value) {
        setBool("FastAOIFrameRateEnable", value);
    }

    long getFrameCount() {
        return getInt("FrameCount");
    }

    void setFrameCount(long value) {
        setInt("FrameCount", value);
    }

    long getFrameCountMin() {
        return getIntMin("FrameCount");
    }

    long getFrameCountMax() {
        return getIntMax("FrameCount");
    }

    double getFrameRate() {
        return getFloat("FrameRate");
    }

    void setFrameRate(double value) {
        setFloat("FrameRate", value);
    }

    double getFrameRateMin() {
        return getFloatMin("FrameRate");
    }

    double getFrameRateMax() {
        return getFloatMax("FrameRate");
    }

    bool isFullAOIControl() {
        return getBool("FullAOIControl");
    }

    void setFullAOIControl(bool value) {
        setBool("FullAOIControl", value);
    }

    long getImageSizeBytes() {
        return getInt("ImageSizeBytes");
    }

    void setImageSizeBytes(long value) {
        setInt("ImageSizeBytes", value);
    }

    long getImageSizeBytesMin() {
        return getIntMin("ImageSizeBytes");
    }

    long getImageSizeBytesMax() {
        return getIntMax("ImageSizeBytes");
    }

    bool isIOInvert() {
        return getBool("IOInvert");
    }

    void setIOInvert(bool value) {
        setBool("IOInvert", value);
    }

    int getIOSelector() {
        return getEnumInt("IOSelector");
    }

    void setIOSelector(int index) {
        setEnumInt("IOSelector", index);
    }

    std::map<int, std::string> getIOSelectorOptions() {
        return getEnumOptions("IOSelector");
    }

    double getLineScanSpeed() {
        return getFloat("LineScanSpeed");
    }

    void setLineScanSpeed(double value) {
        setFloat("LineScanSpeed", value);
    }

    double getLineScanSpeedMin() {
        return getFloatMin("LineScanSpeed");
    }

    double getLineScanSpeedMax() {
        return getFloatMax("LineScanSpeed");
    }

    int getLogLevel() {
        return getEnumInt("LogLevel");
    }

    void setLogLevel(int index) {
        setEnumInt("LogLevel", index);
    }

    std::map<int, std::string> getLogLevelOptions() {
        return getEnumOptions("LogLevel");
    }

    double getLongExposureTransition() {
        return getFloat("LongExposureTransition");
    }

    void setLongExposureTransition(double value) {
        setFloat("LongExposureTransition", value);
    }

    double getLongExposureTransitionMin() {
        return getFloatMin("LongExposureTransition");
    }

    double getLongExposureTransitionMax() {
        return getFloatMax("LongExposureTransition");
    }

    double getMaxInterfaceTransferRate() {
        return getFloat("MaxInterfaceTransferRate");
    }

    void setMaxInterfaceTransferRate(double value) {
        setFloat("MaxInterfaceTransferRate", value);
    }

    double getMaxInterfaceTransferRateMin() {
        return getFloatMin("MaxInterfaceTransferRate");
    }

    double getMaxInterfaceTransferRateMax() {
        return getFloatMax("MaxInterfaceTransferRate");
    }

    bool isMetadataEnable() {
        return getBool("MetadataEnable");
    }

    void setMetadataEnable(bool value) {
        setBool("MetadataEnable", value);
    }

    bool isMetadataFrame() {
        return getBool("MetadataFrame");
    }

    void setMetadataFrame(bool value) {
        setBool("MetadataFrame", value);
    }

    bool isMetadataTimestamp() {
        return getBool("MetadataTimestamp");
    }

    void setMetadataTimestamp(bool value) {
        setBool("MetadataTimestamp", value);
    }

    bool isMultitrackBinned() {
        return getBool("MultitrackBinned");
    }

    void setMultitrackBinned(bool value) {
        setBool("MultitrackBinned", value);
    }

    long getMultitrackCount() {
        return getInt("MultitrackCount");
    }

    void setMultitrackCount(long value) {
        setInt("MultitrackCount", value);
    }

    long getMultitrackCountMin() {
        return getIntMin("MultitrackCount");
    }

    long getMultitrackCountMax() {
        return getIntMax("MultitrackCount");
    }

    long getMultitrackEnd() {
        return getInt("MultitrackEnd");
    }

    void setMultitrackEnd(long value) {
        setInt("MultitrackEnd", value);
    }

    long getMultitrackEndMin() {
        return getIntMin("MultitrackEnd");
    }

    long getMultitrackEndMax() {
        return getIntMax("MultitrackEnd");
    }

    long getMultitrackSelector() {
        return getInt("MultitrackSelector");
    }

    void setMultitrackSelector(long value) {
        setInt("MultitrackSelector", value);
    }

    long getMultitrackSelectorMin() {
        return getIntMin("MultitrackSelector");
    }

    long getMultitrackSelectorMax() {
        return getIntMax("MultitrackSelector");
    }

    long getMultitrackStart() {
        return getInt("MultitrackStart");
    }

    void setMultitrackStart(long value) {
        setInt("MultitrackStart", value);
    }

    long getMultitrackStartMin() {
        return getIntMin("MultitrackStart");
    }

    long getMultitrackStartMax() {
        return getIntMax("MultitrackStart");
    }

    std::vector<Track> getTracks() {

        int count = getMultitrackCount();

        std::vector<Track> tracks;

        for (int i = 0; i < count; i++) {
            
            setMultitrackSelector(i);

            tracks.push_back(Track(
                getMultitrackStart(),
                getMultitrackEnd(),
                isMultitrackBinned()
            ));

        }

        return tracks;

    }

    void setTracks(std::vector<Track> tracks) {

        setMultitrackCount(tracks.size());

        for (int i = 0; i < tracks.size(); i++) {
            setMultitrackSelector(i);
            setMultitrackStart(tracks[i].getStart());
            setMultitrackEnd(tracks[i].getEnd());
            setMultitrackBinned(tracks[i].isBinned());
        }

    }

    bool isOverlap() {
        return getBool("Overlap");
    }

    void setOverlap(bool value) {
        setBool("Overlap", value);
    }

    int getPixelEncoding() {
        return getEnumInt("PixelEncoding");
    }

    void setPixelEncoding(int index) {
        setEnumInt("PixelEncoding", index);
    }

    std::map<int, std::string> getPixelEncodingOptions() {
        return getEnumOptions("PixelEncoding");
    }

    double getPixelHeight() {
        return getFloat("PixelHeight");
    }

    void setPixelHeight(double value) {
        setFloat("PixelHeight", value);
    }

    double getPixelHeightMin() {
        return getFloatMin("PixelHeight");
    }

    double getPixelHeightMax() {
        return getFloatMax("PixelHeight");
    }

    int getPixelReadoutRate() {
        return getEnumInt("PixelReadoutRate");
    }

    void setPixelReadoutRate(int index) {
        setEnumInt("PixelReadoutRate", index);
    }

    std::map<int, std::string> getPixelReadoutRateOptions() {
        return getEnumOptions("PixelReadoutRate");
    }

    double getPixelWidth() {
        return getFloat("PixelWidth");
    }

    void setPixelWidth(double value) {
        setFloat("PixelWidth", value);
    }

    double getPixelWidthMin() {
        return getFloatMin("PixelWidth");
    }

    double getPixelWidthMax() {
        return getFloatMax("PixelWidth");
    }

    double getReadoutTime() {
        return getFloat("ReadoutTime");
    }

    void setReadoutTime(double value) {
        setFloat("ReadoutTime", value);
    }

    double getReadoutTimeMin() {
        return getFloatMin("ReadoutTime");
    }

    double getReadoutTimeMax() {
        return getFloatMax("ReadoutTime");
    }

    bool isRollingShutterGlobalClear() {
        return getBool("RollingShutterGlobalClear");
    }

    void setRollingShutterGlobalClear(bool value) {
        setBool("RollingShutterGlobalClear", value);
    }

    long getRowNExposureEndEvent() {
        return getInt("RowNExposureEndEvent");
    }

    void setRowNExposureEndEvent(long value) {
        setInt("RowNExposureEndEvent", value);
    }

    long getRowNExposureEndEventMin() {
        return getIntMin("RowNExposureEndEvent");
    }

    long getRowNExposureEndEventMax() {
        return getIntMax("RowNExposureEndEvent");
    }

    long getRowNExposureStartEvent() {
        return getInt("RowNExposureStartEvent");
    }

    void setRowNExposureStartEvent(long value) {
        setInt("RowNExposureStartEvent", value);
    }

    long getRowNExposureStartEventMin() {
        return getIntMin("RowNExposureStartEvent");
    }

    long getRowNExposureStartEventMax() {
        return getIntMax("RowNExposureStartEvent");
    }

    bool isScanSpeedControlEnable() {
        return getBool("ScanSpeedControlEnable");
    }

    void setScanSpeedControlEnable(bool value) {
        setBool("ScanSpeedControlEnable", value);
    }

    bool isSensorCooling() {
        return getBool("SensorCooling");
    }

    void setSensorCooling(bool value) {
        setBool("SensorCooling", value);
    }

    long getSensorHeight() {
        return getInt("SensorHeight");
    }

    void setSensorHeight(long value) {
        setInt("SensorHeight", value);
    }

    long getSensorHeightMin() {
        return getIntMin("SensorHeight");
    }

    long getSensorHeightMax() {
        return getIntMax("SensorHeight");
    }

    int getSensorReadoutMode() {
        return getEnumInt("SensorReadoutMode");
    }

    void setSensorReadoutMode(int index) {
        setEnumInt("SensorReadoutMode", index);
    }

    std::map<int, std::string> getSensorReadoutModeOptions() {
        return getEnumOptions("SensorReadoutMode");
    }

    double getSensorTemperature() {
        return getFloat("SensorTemperature");
    }

    void setSensorTemperature(double value) {
        setFloat("SensorTemperature", value);
    }

    double getSensorTemperatureMin() {
        return getFloatMin("SensorTemperature");
    }

    double getSensorTemperatureMax() {
        return getFloatMax("SensorTemperature");
    }

    long getSensorWidth() {
        return getInt("SensorWidth");
    }

    void setSensorWidth(long value) {
        setInt("SensorWidth", value);
    }

    long getSensorWidthMin() {
        return getIntMin("SensorWidth");
    }

    long getSensorWidthMax() {
        return getIntMax("SensorWidth");
    }

    std::string getSerialNumber() {
        return getString("SerialNumber");
    }

    int getShutterMode() {
        return getEnumInt("ShutterMode");
    }

    void setShutterMode(int index) {
        setEnumInt("ShutterMode", index);
    }

    std::map<int, std::string> getShutterModeOptions() {
        return getEnumOptions("ShutterMode");
    }

    int getShutterOutputMode() {
        return getEnumInt("ShutterOutputMode");
    }

    void setShutterOutputMode(int index) {
        setEnumInt("ShutterOutputMode", index);
    }

    std::map<int, std::string> getShutterOutputModeOptions() {
        return getEnumOptions("ShutterOutputMode");
    }

    double getShutterTransferTime() {
        return getFloat("ShutterTransferTime");
    }

    void setShutterTransferTime(double value) {
        setFloat("ShutterTransferTime", value);
    }

    double getShutterTransferTimeMin() {
        return getFloatMin("ShutterTransferTime");
    }

    double getShutterTransferTimeMax() {
        return getFloatMax("ShutterTransferTime");
    }

    int getSimplePreAmpGainControl() {
        return getEnumInt("SimplePreAmpGainControl");
    }

    void setSimplePreAmpGainControl(int index) {
        setEnumInt("SimplePreAmpGainControl", index);
    }

    std::map<int, std::string> getSimplePreAmpGainControlOptions() {
        return getEnumOptions("SimplePreAmpGainControl");
    }

    void softwareTrigger() {
        command("SoftwareTrigger");
    }

    std::string getSoftwareVersion() {
        return getString("SoftwareVersion");
    }

    bool isSpuriousNoiseFilter() {
        return getBool("SpuriousNoiseFilter");
    }

    void setSpuriousNoiseFilter(bool value) {
        setBool("SpuriousNoiseFilter", value);
    }

    bool isStaticBlemishCorrection() {
        return getBool("StaticBlemishCorrection");
    }

    void setStaticBlemishCorrection(bool value) {
        setBool("StaticBlemishCorrection", value);
    }

    int getTemperatureControl() {
        return getEnumInt("TemperatureControl");
    }

    void setTemperatureControl(int index) {
        setEnumInt("TemperatureControl", index);
    }

    std::map<int, std::string> getTemperatureControlOptions() {
        return getEnumOptions("TemperatureControl");
    }

    int getTemperatureStatus() {
        return getEnumInt("TemperatureStatus");
    }

    void setTemperatureStatus(int index) {
        setEnumInt("TemperatureStatus", index);
    }

    std::map<int, std::string> getTemperatureStatusOptions() {
        return getEnumOptions("TemperatureStatus");
    }

    long getTimestampClock() {
        return getInt("TimestampClock");
    }

    void setTimestampClock(long value) {
        setInt("TimestampClock", value);
    }

    long getTimestampClockMin() {
        return getIntMin("TimestampClock");
    }

    long getTimestampClockMax() {
        return getIntMax("TimestampClock");
    }

    long getTimestampClockFrequency() {
        return getInt("TimestampClockFrequency");
    }

    void setTimestampClockFrequency(long value) {
        setInt("TimestampClockFrequency", value);
    }

    long getTimestampClockFrequencyMin() {
        return getIntMin("TimestampClockFrequency");
    }

    long getTimestampClockFrequencyMax() {
        return getIntMax("TimestampClockFrequency");
    }

    void timestampClockReset() {
        command("TimestampClockReset");
    }

    int getTriggerMode() {
        return getEnumInt("TriggerMode");
    }

    void setTriggerMode(int index) {
        setEnumInt("TriggerMode", index);
    }

    std::map<int, std::string> getTriggerModeOptions() {
        return getEnumOptions("TriggerMode");
    }

    bool isVerticallyCentreAOI() {
        return getBool("VerticallyCentreAOI");
    }

    void setVerticallyCentreAOI(bool value) {
        setBool("VerticallyCentreAOI", value);
    }
    
};