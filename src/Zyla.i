%module PyZyla
%include std_string.i
%include std_map.i
%include stdint.i
%include std_vector.i
%include exception.i   
%inline %{

%}
%{
#include "Zyla.cpp"
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

%template(ushort_vector) std::vector<unsigned short>;
%template(options) std::map<int, std::string>;

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

    long getAcquireFPS();

    long getProcessFPS();

    long getWriteFPS();

    long getProcessQueueSize();

    long getWriteQueueSize();

    long getAcquireCount();

    bool isRunning();

    bool isMonitoring();
};

class Track {

   public:

    Track(const Track& other);

    Track(long start, long end, bool binned);

    long getStart();

    long getEnd();

    bool isBinned();

    void setStart(long value);

    void setEnd(long value);

    void setBinned(bool value);

};

class Acquisition {

    public:

    Acquisition(unsigned short* data, long width, long height);

    Acquisition(const Acquisition& other);

    ~Acquisition();

    long getWidth();

    long getHeight();

    long getSize();

    unsigned short getPixel(int row, int column);

    std::vector<unsigned short> getArray();

    static Acquisition example();

    unsigned short operator()(int row, int column);

    %pythoncode %{

        def numpy(self):

            import numpy as np
            return np.reshape(self.getArray(), (self.getHeight(), self.getWidth()))

    %}
    
};

class Zyla {

    public:

    Zyla(int index);

    ~Zyla();

    A3C capturePipeline();

    long getInt(std::string feature);

    void setInt(std::string feature, long value);

    long getIntMin(std::string feature);

    long getIntMax(std::string feature);

    bool getBool(std::string feature);

    void setBool(std::string feature, bool value);

    double getFloat(std::string feature);

    double getFloatMin(std::string feature);

    double getFloatMax(std::string feature);

    void setFloat(std::string feature, double value);

    int getEnumInt(std::string feature);

    void setEnumInt(std::string feature, int index);

    std::string getEnum(std::string feature);

    std::string getString(std::string feature);

    void setEnum(std::string feature, std::string value);

    void command(std::string feature);

    std::map<int, std::string> getEnumOptions(std::string feature);

    void queueBuffer(unsigned char buffer[], int bufferSize);

    void queueBuffer(int size);

    void queueBuffer();

    unsigned char* awaitBuffer(unsigned int timeout, int* size);

    unsigned char* acquireRaw(int timeout, int* size);

    Acquisition acquire(int timeout);

    long getAccumulateCount();

    void setAccumulateCount(long value);

    long getAccumulateCountMin();

    long getAccumulateCountMax();

    void acquisitionStart();

    void acquisitionStop();

    bool isAlternatingReadoutDirection();

    void setAlternatingReadoutDirection(bool value);

    int getAOIBinning();

    void setAOIBinning(int index);

    std::map<int, std::string> getAOIBinningOptions();

    long getAOIHBin();

    void setAOIHBin(long value);

    long getAOIHBinMin();

    long getAOIHBinMax();

    long getAOIHeight();

    void setAOIHeight(long value);

    long getAOIHeightMin();

    long getAOIHeightMax();

    int getAOILayout();

    void setAOILayout(int index);

    std::map<int, std::string> getAOILayoutOptions();

    long getAOILeft();

    void setAOILeft(long value);

    long getAOILeftMin();

    long getAOILeftMax();

    long getAOIStride();

    void setAOIStride(long value);

    long getAOIStrideMin();

    long getAOIStrideMax();

    long getAOITop();

    void setAOITop(long value);

    long getAOITopMin();

    long getAOITopMax();

    long getAOIVBin();

    void setAOIVBin(long value);

    long getAOIVBinMin();

    long getAOIVBinMax();

    long getAOIWidth();

    void setAOIWidth(long value);

    long getAOIWidthMin();

    long getAOIWidthMax();

    int getAuxiliaryOutSource();

    void setAuxiliaryOutSource(int index);

    std::map<int, std::string> getAuxiliaryOutSourceOptions();

    int getAuxOutSourceTwo();

    void setAuxOutSourceTwo(int index);

    std::map<int, std::string> getAuxOutSourceTwoOptions();

    long getBaseline();

    void setBaseline(long value);

    long getBaselineMin();

    long getBaselineMax();

    int getBitDepth();

    void setBitDepth(int index);

    std::map<int, std::string> getBitDepthOptions();

    long getBufferOverflowEvent();

    void setBufferOverflowEvent(long value);

    long getBufferOverflowEventMin();

    long getBufferOverflowEventMax();

    double getBytesPerPixel();

    void setBytesPerPixel(double value);

    double getBytesPerPixelMin();

    double getBytesPerPixelMax();

    bool isCameraAcquiring();

    void setCameraAcquiring(bool value);

    bool isCameraPresent();

    void setCameraPresent(bool value);

    std::string getControllerID();

    double getCoolerPower();

    void setCoolerPower(double value);

    double getCoolerPowerMin();

    double getCoolerPowerMax();

    int getCycleMode();

    void setCycleMode(int index);

    std::map<int, std::string> getCycleModeOptions();

    long getDeviceCount();

    void setDeviceCount(long value);

    long getDeviceCountMin();

    long getDeviceCountMax();

    long getDeviceVideoIndex();

    void setDeviceVideoIndex(long value);

    long getDeviceVideoIndexMin();

    long getDeviceVideoIndexMax();

    int getElectronicShutteringMode();

    void setElectronicShutteringMode(int index);

    std::map<int, std::string> getElectronicShutteringModeOptions();

    bool isEventEnable();

    void setEventEnable(bool value);

    int getEventSelector();

    void setEventSelector(int index);

    std::map<int, std::string> getEventSelectorOptions();

    long getEventsMissedEvent();

    void setEventsMissedEvent(long value);

    long getEventsMissedEventMin();

    long getEventsMissedEventMax();

    long getExposedPixelHeight();

    void setExposedPixelHeight(long value);

    long getExposedPixelHeightMin();

    long getExposedPixelHeightMax();

    long getExposureEndEvent();

    void setExposureEndEvent(long value);

    long getExposureEndEventMin();

    long getExposureEndEventMax();

    long getExposureStartEvent();

    void setExposureStartEvent(long value);

    long getExposureStartEventMin();

    long getExposureStartEventMax();

    double getExposureTime();

    void setExposureTime(double value);

    double getExposureTimeMin();

    double getExposureTimeMax();

    double getExternalTriggerDelay();

    void setExternalTriggerDelay(double value);

    double getExternalTriggerDelayMin();

    double getExternalTriggerDelayMax();

    int getFanSpeed();

    void setFanSpeed(int index);

    std::map<int, std::string> getFanSpeedOptions();

    bool isFastAOIFrameRateEnable();

    void setFastAOIFrameRateEnable(bool value);

    long getFrameCount();

    void setFrameCount(long value);

    long getFrameCountMin();

    long getFrameCountMax();

    double getFrameRate();

    void setFrameRate(double value);

    double getFrameRateMin();

    double getFrameRateMax();

    bool isFullAOIControl();

    void setFullAOIControl(bool value);

    long getImageSizeBytes();

    void setImageSizeBytes(long value);

    long getImageSizeBytesMin();

    long getImageSizeBytesMax();

    bool isIOInvert();

    void setIOInvert(bool value);

    int getIOSelector();

    void setIOSelector(int index);

    std::map<int, std::string> getIOSelectorOptions();

    double getLineScanSpeed();

    void setLineScanSpeed(double value);

    double getLineScanSpeedMin();

    double getLineScanSpeedMax();

    int getLogLevel();

    void setLogLevel(int index);

    std::map<int, std::string> getLogLevelOptions();

    double getLongExposureTransition();

    void setLongExposureTransition(double value);

    double getLongExposureTransitionMin();

    double getLongExposureTransitionMax();

    double getMaxInterfaceTransferRate();

    void setMaxInterfaceTransferRate(double value);

    double getMaxInterfaceTransferRateMin();

    double getMaxInterfaceTransferRateMax();

    bool isMetadataEnable();

    void setMetadataEnable(bool value);

    bool isMetadataFrame();

    void setMetadataFrame(bool value);

    bool isMetadataTimestamp();

    void setMetadataTimestamp(bool value);

    bool isMultitrackBinned();

    void setMultitrackBinned(bool value);

    long getMultitrackCount();

    void setMultitrackCount(long value);

    long getMultitrackCountMin();

    long getMultitrackCountMax();

    long getMultitrackEnd();

    void setMultitrackEnd(long value);

    long getMultitrackEndMin();

    long getMultitrackEndMax();

    long getMultitrackSelector();

    void setMultitrackSelector(long value);

    long getMultitrackSelectorMin();

    long getMultitrackSelectorMax();

    long getMultitrackStart();

    void setMultitrackStart(long value);

    long getMultitrackStartMin();

    long getMultitrackStartMax();

    std::vector<Track> getTracks();

    void setTracks(std::vector<Track> tracks);

    bool isOverlap();

    void setOverlap(bool value);

    int getPixelEncoding();

    void setPixelEncoding(int index);

    std::map<int, std::string> getPixelEncodingOptions();

    double getPixelHeight();

    void setPixelHeight(double value);

    double getPixelHeightMin();

    double getPixelHeightMax();

    int getPixelReadoutRate();

    void setPixelReadoutRate(int index);

    std::map<int, std::string> getPixelReadoutRateOptions();

    double getPixelWidth();

    void setPixelWidth(double value);

    double getPixelWidthMin();

    double getPixelWidthMax();

    double getReadoutTime();

    void setReadoutTime(double value);

    double getReadoutTimeMin();

    double getReadoutTimeMax();

    bool isRollingShutterGlobalClear();

    void setRollingShutterGlobalClear(bool value);

    long getRowNExposureEndEvent();

    void setRowNExposureEndEvent(long value);

    long getRowNExposureEndEventMin();

    long getRowNExposureEndEventMax();

    long getRowNExposureStartEvent();

    void setRowNExposureStartEvent(long value);

    long getRowNExposureStartEventMin();

    long getRowNExposureStartEventMax();

    bool isScanSpeedControlEnable();

    void setScanSpeedControlEnable(bool value);

    bool isSensorCooling();

    void setSensorCooling(bool value);

    long getSensorHeight();

    void setSensorHeight(long value);

    long getSensorHeightMin();

    long getSensorHeightMax();

    int getSensorReadoutMode();

    void setSensorReadoutMode(int index);

    std::map<int, std::string> getSensorReadoutModeOptions();

    double getSensorTemperature();

    void setSensorTemperature(double value);

    double getSensorTemperatureMin();

    double getSensorTemperatureMax();

    long getSensorWidth();

    void setSensorWidth(long value);

    long getSensorWidthMin();

    long getSensorWidthMax();

    std::string getSerialNumber();

    int getShutterMode();

    void setShutterMode(int index);

    std::map<int, std::string> getShutterModeOptions();

    int getShutterOutputMode();

    void setShutterOutputMode(int index);

    std::map<int, std::string> getShutterOutputModeOptions();

    double getShutterTransferTime();

    void setShutterTransferTime(double value);

    double getShutterTransferTimeMin();

    double getShutterTransferTimeMax();

    int getSimplePreAmpGainControl();

    void setSimplePreAmpGainControl(int index);

    std::map<int, std::string> getSimplePreAmpGainControlOptions();

    void softwareTrigger();

    std::string getSoftwareVersion();

    bool isSpuriousNoiseFilter();

    void setSpuriousNoiseFilter(bool value);

    bool isStaticBlemishCorrection();

    void setStaticBlemishCorrection(bool value);

    int getTemperatureControl();

    void setTemperatureControl(int index);

    std::map<int, std::string> getTemperatureControlOptions();

    int getTemperatureStatus();

    void setTemperatureStatus(int index);

    std::map<int, std::string> getTemperatureStatusOptions();

    long getTimestampClock();

    void setTimestampClock(long value);

    long getTimestampClockMin();

    long getTimestampClockMax();

    long getTimestampClockFrequency();

    void setTimestampClockFrequency(long value);

    long getTimestampClockFrequencyMin();

    long getTimestampClockFrequencyMax();

    void timestampClockReset();

    int getTriggerMode();

    void setTriggerMode(int index);

    std::map<int, std::string> getTriggerModeOptions();

    bool isVerticallyCentreAOI();

    void setVerticallyCentreAOI(bool value);
    
};
