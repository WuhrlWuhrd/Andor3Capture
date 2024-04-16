#include "atcore.h"
#include "atutility.h"
#include <string>
#include <iostream>
#include <sstream>

using namespace std;

#define String string
#define WString wstring


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

String getString(AT_H handle, String feature) {

    AT_WC* chars  = new AT_WC[1024];
    int    result = AT_GetString(handle, stringToWC(feature), chars, 1024);

    if (result != AT_SUCCESS) {
        ostringstream oss;
        oss << feature << ": " << result;
        throw oss.str();
    }

    return wcToString(chars);

}

void setEnum(AT_H handle, String feature, String value)
{

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

#if !(defined(_WIN64) || defined(_WIN32))
int AT_ConvertBuffer(AT_U8* inputBuffer, AT_U8* outputBuffer, AT_64 width, AT_64 height, AT_64 stride, const AT_WC * inputPixelEncoding, const AT_WC * outputPixelEncoding) {
    return AT_SUCCESS;
}
#endif