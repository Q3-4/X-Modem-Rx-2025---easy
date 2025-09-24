// Version 24.09.2025 - Schwaiger
#pragma once
#define NOMINMAX
#include <iostream>
#include <string>
#include <algorithm>   
#include <Windows.h>

using namespace std;

/**
 * @brief Einfache Wrapper-Klasse für eine serielle Schnittstelle unter Windows.
 *
 * Verhalten gemäß Word-Dokument „Serielle Schnittstellen“ (Abi-Standardklasse):
 *  - dataAvailable(): nicht blockierend, liefert Anzahl aktuell lesbarer Bytes.
 *  - read(): blockierend, liefert 0..255; -1 falls Port nicht offen (oder Fehler).
 *  - read(b,len): blockierend bis mind. 1 Byte; liefert Anzahl gelesener Bytes,
 *                 oder -1, falls Port nicht offen / keine Bytes verfügbar (Fehler).
 *  - readLine(): blockierend bis '\n'; entspricht "null bei nicht offen" aus dem Dokument:
 *                 In C++ geben wir dafür den leeren String "" zurück.
 *
 *  - write(value|b|s): schreiben; wenn Port nicht offen, passiert nichts.
 */
class Serial
{
private:
    string  portName;   ///< COM-Port-Name, z. B. "COM3"
    int     baudrate;   ///< z. B. 9600, 115200
    int     dataBits;   ///< 5–8
    int     stopBits;   ///< ONESTOPBIT, ONE5STOPBITS, TWOSTOPBITS
    int     parity;     ///< NOPARITY, ODDPARITY, EVENPARITY, ...
    HANDLE  handle;     ///< INVALID_HANDLE_VALUE, wenn geschlossen

public:
    // -------- Konstruktor / Destruktor --------
    Serial(string portName, int baudrate, int dataBits, int stopBits, int parity);
    ~Serial();

    // -------- Basis --------
    bool open();   ///< Öffnen; true bei Erfolg
    void close();  ///< Schließen

    int  dataAvailable(); ///< Anzahl Bytes im Eingabepuffer (nicht blockierend)

    // -------- Lesen (blockierend) --------
    int     read();                       ///< 1 Byte (0..255) oder -1
    int     read(char* b, int len);       ///< >=1 gelesene Bytes oder -1
    string  readLine();                   ///< bis '\n'; "" falls „Port nicht offen“ (C++-Ersatz für null)

    // -------- Schreiben --------
    void write(int value);                ///< 1 Byte schreiben
    void write(const char* b, int len);   ///< Puffer schreiben
    void write(string s);                 ///< String schreiben (ohne auto-'\n')

    // -------- Modem-/Handshake-Signale --------
    void setRTS(bool arg);
    void setDTR(bool arg);
    bool isCTS();
    bool isDSR();

    // -------- Komfort --------
    bool isOpen() const { return handle != INVALID_HANDLE_VALUE; } ///< Für Nutzer-Code (z. B. vor readLine)
};
