#include "atcore.h"
#include "atutility.h"
#include <string>
#include <iostream>
#include <sstream>

using namespace std;


string wcTostring(AT_WC* wc) {

    wstring wString = wstring(wc);
    return string(wString.begin(), wString.end());

}

AT_WC* stringToWC(string str) {

    size_t length = str.length();

    wstring wString = wstring(str.begin(), str.end());
    wString.resize(length);
    AT_WC* wc = new AT_WC[length + 1];
    wString.copy(wc, length, 0); 
    wc[length] = L'\0';

    return wc;

}

long getInt(AT_H handle, string feature) {

    AT_64 value;
    int result = AT_GetInt(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

void setInt(AT_H handle, string feature, long value) {


    int result = AT_SetInt(handle, stringToWC(feature), value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

}

long getIntMin(AT_H handle, string feature) {

    AT_64 value;
    int result = AT_GetIntMin(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

bool getBool(AT_H handle, string feature) {

    AT_BOOL value;
    int result = AT_GetBool(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

void setBool(AT_H handle, string feature, bool value) {


    int result = AT_SetBool(handle, stringToWC(feature), value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

}

double getFloat(AT_H handle, string feature) {

    double value;
    int result = AT_GetFloat(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

double getFloatMin(AT_H handle, string feature) {

    double value;
    int result = AT_GetFloatMin(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

void setFloat(AT_H handle, string feature, double value) {


    int result = AT_SetFloat(handle, stringToWC(feature), value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

}

int getEnumInt(AT_H handle, string feature) {

    int value;
    int result = AT_GetEnumIndex(handle, stringToWC(feature), &value);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return value;
}

void setEnumInt(AT_H handle, string feature, int index) {


    int result = AT_SetEnumIndex(handle, stringToWC(feature), index);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

}

string getEnum(AT_H handle, string feature) {

    int    value  = getEnumInt(handle, feature);
    AT_WC* chars  = new AT_WC[1024];
    int    result = AT_GetEnumStringByIndex(handle, stringToWC(feature), value, chars, 1024);

    return wcTostring(chars);

}

string getString(AT_H handle, string feature) {

    AT_WC* chars  = new AT_WC[1024];
    int    result = AT_GetString(handle, stringToWC(feature), chars, 1024);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return wcTostring(chars);

}

void setEnum(AT_H handle, string feature, string value)
{

    int result = AT_SetEnumString(handle, stringToWC(feature), stringToWC(value));

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << " = " << value << ": " << result;
        throw oss.str();
    }
}

void printEnum(AT_H handle, string feature) {

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

        cout << "Index " << i << ": " << wcTostring(chars) << " (" << (impl ? "Implemented" : "Unimplemented") << ", " << (avail ? "Available" : "Unavailable") << ")" << endl;

    }

}

AT_H open(int index) {

    AT_H handle;
    int result = AT_Open(index, &handle);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << "Open: " << result;
        throw oss.str();
    }

    return handle;
}

// Dummy methods for when atutility.so is missing...
#if defined(NO_LIB_UTILITY)

int AT_ConvertBuffer(AT_U8* inputBuffer, AT_U8* outputBuffer, AT_64 width, AT_64 height, AT_64 stride, const AT_WC * inputPixelEncoding, const AT_WC * outputPixelEncoding) {
    return AT_SUCCESS;
}

int AT_ConvertBufferUsingMetadata(AT_U8* inputBuffer, AT_U8* outputBuffer, AT_64 imagesizebytes, const AT_WC * outputPixelEncoding) {
    return AT_SUCCESS;
}

int AT_GetWidthFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64& width) {
    return AT_SUCCESS;
}

int AT_GetHeightFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64& height) {
    return AT_SUCCESS;
}

int AT_GetStrideFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64& stride) {
    return AT_SUCCESS;
}

int AT_GetPixelEncodingFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_WC* pixelEncoding, AT_U8 pixelEncodingSize) {
    return AT_SUCCESS;
}

int AT_GetTimeStampFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64& timeStamp) {
    return AT_SUCCESS;
}

int AT_GetIRIGFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64* seconds, AT_64* minutes, AT_64* hours, AT_64* days, AT_64* years) {
    return AT_SUCCESS;
}

int AT_GetExtendedIRIGFromMetadata(AT_U8* inputBuffer, AT_64 imagesizebytes, AT_64 clockfrequency, double* nanoseconds, AT_64* seconds, AT_64* minutes, AT_64* hours, AT_64* days, AT_64* years) {
    return AT_SUCCESS;
}

#endif