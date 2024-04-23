#pragma once

#include <cstdint>
#include <string>
#include <stdlib.h>
#include <unistd.h>

typedef int HANDLE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef DWORD* LPDWORD;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef bool BOOL;
typedef int32_t LONG;
typedef uint UINT;
typedef unsigned char BYTE;
typedef const unsigned char* LPCBYTE;
typedef char16_t* LPWSTR;
typedef const char16_t* LPCWSTR;
typedef const char* LPCSTR;
typedef char16_t WCHAR;

#define TRUE 1
#define FALSE 0

#define BI_RGB 1

#define WINAPI

#define CP_ACP 65001

#define __declspec(dllexport)

typedef char TCHAR;
typedef std::basic_string<TCHAR> tstring;

typedef struct tagBITMAPFILEHEADER
{
    uint16_t    bfType; // 2  /* Magic identifier */
    uint32_t   bfSize; // 4  /* File size in bytes */
    uint16_t    bfReserved1; // 2
    uint16_t    bfReserved2; // 2
    uint32_t   bfOffBits; // 4 /* Offset to image data, bytes */ 
} __attribute__((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    uint32_t    biSize; // 4 /* Header size in bytes */
    int32_t     biWidth; // 4 /* Width of image */
    int32_t     biHeight; // 4 /* Height of image */
    uint16_t     biPlanes; // 2 /* Number of colour planes */
    uint16_t     biBitCount; // 2 /* Bits per pixel */
    uint32_t    biCompression; // 4 /* Compression type */
    uint32_t    biSizeImage; // 4 /* Image size in bytes */
    int32_t     biXPelsPerMeter; // 4
    int32_t     biYPelsPerMeter; // 4 /* Pixels per meter */
    uint32_t    biClrUsed; // 4 /* Number of colours */ 
    uint32_t    biClrImportant; // 4 /* Important colours */ 
} __attribute__((packed)) BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
  int rgbBlue;
  int rgbGreen;
  int rgbRed;
  int rgbReserved;
} RGBQUAD;

typedef struct tagRECT {
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
} RECT, *PRECT, *NPRECT, *LPRECT;

typedef struct _OVERLAPPED {} OVERLAPPED, *LPOVERLAPPED;

inline BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumerOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
  *lpNumberOfBytesWritten = write(hFile, lpBuffer, nNumerOfBytesToWrite);
  return *lpNumberOfBytesWritten > 0;
}

inline char16_t* _wcsdup(const char16_t* str) {
  size_t length = std::char_traits<char16_t>::length(str);
  char16_t* duped = (char16_t*)malloc(length * sizeof(char16_t));
  std::char_traits<char16_t>::copy(duped, str, length);
  return duped;
}

inline int SetEnvironmentVariable(const char* name, const char* value) {
  if (value == NULL) {
    return unsetenv(name);
  }
  return setenv(name, value, 1);
}

inline BOOL ReadFile(HANDLE hFile, LPVOID buffer, DWORD nNumberOfBytesToRead, LPDWORD lpNmumberOfBytesRead, LPOVERLAPPED lpOverlapped=NULL) {
    *lpNmumberOfBytesRead = read(hFile, buffer, nNumberOfBytesToRead);
    return *lpNmumberOfBytesRead > 0;
}