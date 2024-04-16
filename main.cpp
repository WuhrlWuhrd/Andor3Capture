#include "A3C.cpp"
#include <chrono>
#include <stdio.h>


int main() {

    try {

        cout << "A3Capture testing utility." << endl;

        cout << "Initialising... ";
        AT_InitialiseLibrary();

        cout << "Done." << endl;
        long devices = getInt(AT_HANDLE_SYSTEM, "DeviceCount");
        cout << "Found " << devices << " connected cameras." << endl;

        if (devices == 0) {
            cout << "Exiting." << endl;
            return 0;
        }

        int index = 0;

        if (devices > 1) {

            index = -1;

            cout << endl << "===================" << endl;
            cout         << " CONNECTED CAMERAS "  << endl;
            cout         << "===================" << endl << endl;
   
            for (int i = 0; i < devices; i++) {

                AT_H handle = open(i);

                String model = getString(handle, "CameraModel");
                String name  = getString(handle, "CameraName");

                cout << "0: " << name << " (" << model << ")" << endl;

                AT_Close(handle);

            }

            while (index < 0 || index >= devices) {
                cout << endl << "Please select which camera to use: ";
                cin >> index;
            }

        }



        cout << "Opening camera... ";
        AT_H handle = open(index);
        cout << "Done." << endl;

        setBool(handle, "SensorCooling", true);

        int status = 0;
        double temperature;

        cout << "Waiting for temperature to stabilise... " << endl;

        while (status != 1) {

            status      = getEnumInt(handle, "Temperature Status");
            temperature = getFloat(handle, "SensorTemperature");

            cout << "\r\e[K" << std::flush << "T = " << temperature << "*C";

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