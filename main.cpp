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
            AT_FinaliseLibrary();
            return 0;
        }

        int index = 0;
        AT_H handle;
        
        if (devices > 1) {

            index = -1;

            cout << endl << "===================" << endl;
            cout         << " CONNECTED CAMERAS " << endl;
            cout         << "===================" << endl;

            AT_H* handles = new AT_H[devices];

            for (int i = 0; i < devices; i++) {

                cout << i << ": Loading...";

                handles[i] = open(i);

                String model = getString(handles[i], "CameraModel");
                String name  = getString(handles[i], "CameraName");

                cout << "\r\e[K" << std::flush << i << ": " << name << " (" << model << ")" << endl;

            }

            while (index < 0 || index >= devices) {
                cout << endl << "Please select which camera to use: ";
                cin >> index;
            }

            handle = handles[index];

            for (int i = 0; i < devices; i++) {

                if (i != index) {
                    AT_Close(handles[i]);
                }

            }

        } else {
            handle = open(0);
        }

        cin.get();

        cout << "Chosen: " << getString(handle, "CameraName") << endl;

        setBool(handle, "SensorCooling", true);

        int status = 0;
        double temperature;

        cout << "Stabilise temperature? [y/n] ";

        string response = "";

        while (response != "y" && response != "n") {
            cin >> response;
        }

        cin.get();

        if (response == "y") {

            cout << "Waiting for temperature to stabilise... " << endl;

            while (false && status != 1) {

                status      = getEnumInt(handle, "Temperature Status");
                temperature = getFloat(handle, "SensorTemperature");

                cout << "\r\e[K" << std::flush << "T = " << temperature << "*C";

                this_thread::sleep_for(chrono::milliseconds(1000));

            }

            cout << "Stabilised." << endl;

        }


        setEnum(handle, "AOILayout", "Multitrack");
        setInt(handle, "MultitrackCount", 1);
        setInt(handle, "MultitrackSelector", 0);
        setInt(handle, "MultitrackStart", 1);
        setInt(handle, "MultitrackEnd", 5);
        setBool(handle, "MultitrackBinned", false);
        setEnum(handle, "PixelReadoutRate", "270 MHz");
        setEnum(handle, "TriggerMode", "Internal");
        setEnum(handle, "ShutterMode", "Open");
        setEnum(handle, "FanSpeed", "On");
        setBool(handle, "RollingShutterGlobalClear", false);
        setEnum(handle, "ElectronicShutteringMode", "Rolling");
        setBool(handle, "FastAOIFrameRateEnable", true);
        setBool(handle, "Overlap", true);
        setBool(handle, "VerticallyCentreAOI", true);
        double minExp = getFloatMin(handle, "ExposureTime");
        double maxRate = getFloat(handle, "MaxInterfaceTransferRate");
        double minTime = 1.0/maxRate;
        setFloat(handle, "ExposureTime", minTime > minExp ? minExp : minExp);

        cout << getIntMin(handle, "AOIHeight") << endl;

        double rate        = getFloat(handle, "FrameRate");
        double exp         = getFloat(handle, "ExposureTime");
        double minRowTime  = getFloatMin(handle, "RowReadTime");
        double rowTime     = getFloat(handle, "RowReadTime");
        double readTime    = getFloat(handle, "ReadoutTime");
        double transition  = getFloat(handle, "LongExposureTransition");
        String mode        = getEnum(handle, "PixelReadoutRate");
        String shutter     = getEnum(handle, "ElectronicShutteringMode");
        long   accCount    = getInt(handle, "AccumulateCount");

        cout << "Frame Rate: "          << rate       << " Hz" << endl;
        cout << "Max Frame Rate: "      << maxRate    << " Hz" << endl;
        cout << "Exposure Time: "       << exp        << " s"  << endl;
        cout << "Min Exposure Time: "   << minExp     << " s"  << endl;
        cout << "Accumulate Count: "    << accCount            << endl;
        cout << "Min Row Read Time: "   << minRowTime << " s"  << endl;
        cout << "Row Read Time: "       << rowTime    << " s"  << endl;
        cout << "Readout Time: "        << readTime   << " s"  << endl;
        cout << "Readout Rate: "        << mode                << endl;
        cout << "Exposure Transition: " << transition << " s"  << endl;
        cout << "Shuttering Mode: "     << shutter             << endl;

        A3C capture = A3C(handle);

        capture.setOutputPath("C:\\Users\\HERA\\Desktop\\data.bin");

        cout << "Ready, press enter to start..." << endl;
        cin.ignore();

        capture.start();

        cout << "Capturing, press enter to stop..." << endl;
        cin.ignore();

        cout << "Stopping threads..." << endl;

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