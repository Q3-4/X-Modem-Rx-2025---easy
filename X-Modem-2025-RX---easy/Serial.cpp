// Version 24.09.2025 - Schwaiger
/*
  Hinweis zu Zeichensatz-Einstellungen (Visual Studio):
    Diese einfache Variante nutzt std::string (ANSI) und CreateFileA.
    Für den Unterricht ausreichend und übersichtlich.
    Empfohlen: Plattform "x64".
*/

#include "Serial.h"

// ==============================
// Konstruktor / Destruktor
// ==============================

Serial::Serial(string portName, int baudrate, int dataBits, int stopBits, int parity)
{
    this->portName = portName;
    this->baudrate = baudrate;
    this->dataBits = dataBits;
    this->stopBits = stopBits;
    this->parity = parity;
    this->handle = INVALID_HANDLE_VALUE;
}

Serial::~Serial()
{
    close();
}

// ==============================
// Öffnen / Schließen
// ==============================

bool Serial::open()
{
    // COM-Port öffnen (synchron, ohne Overlapped I/O)
    handle = CreateFileA(
        portName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,                // kein Sharing
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (handle == INVALID_HANDLE_VALUE)
        return false;

    // Port-Parameter holen und anpassen
    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(handle, &dcb)) {
        close();
        return false;
    }

    dcb.BaudRate = baudrate;
    dcb.ByteSize = (BYTE)dataBits;
    dcb.StopBits = (BYTE)stopBits;
    dcb.Parity = (BYTE)parity;
    dcb.fParity = (dcb.Parity != NOPARITY);

    if (!SetCommState(handle, &dcb)) {
        close();
        return false;
    }

    // Klassisch/blockierend lesen (wie im Dokument: read() wartet)
    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout = 0;
    to.ReadTotalTimeoutMultiplier = 0;
    to.ReadTotalTimeoutConstant = 0;   // 0 => blockiert
    to.WriteTotalTimeoutMultiplier = 0;
    to.WriteTotalTimeoutConstant = 100; // Schreiben darf kurz blockieren

    if (!SetCommTimeouts(handle, &to)) {
        close();
        return false;
    }

    return true;
}

void Serial::close()
{
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }
}

// ==============================
// Verfügbarkeit / Lesen / Schreiben
// ==============================

int Serial::dataAvailable()
{
    COMSTAT comStat{};
    DWORD errors = 0;

    if (handle != INVALID_HANDLE_VALUE) {
        if (ClearCommError(handle, &errors, &comStat))
            return (int)comStat.cbInQue;
    }
    return 0; // nicht blockierend; 0 wenn nichts/offen?
}

void Serial::write(int value)
{
    if (handle != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten = 0;
        char v = (char)value;
        WriteFile(handle, &v, 1, &bytesWritten, nullptr);
    }
}

void Serial::write(const char* b, int len)
{
    if (handle != INVALID_HANDLE_VALUE && b != nullptr && len > 0) {
        DWORD bytesWritten = 0;
        WriteFile(handle, b, (DWORD)len, &bytesWritten, nullptr);
    }
}

void Serial::write(string s)
{
    // Wichtig: KEIN automatisches '\n' – das fordert das Dokument als Caller-Pflicht.
    if (handle != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten = 0;
        WriteFile(handle, s.c_str(), (DWORD)s.length(), &bytesWritten, nullptr);
    }
}

int Serial::read()
{
    if (handle == INVALID_HANDLE_VALUE)
        return -1; // Dokument: -1 bei „nicht geöffnet“

    DWORD dwRead = 0;
    unsigned char chRead = 0;

    // Blockiert, bis 1 Byte gelesen wurde (oder Fehler)
    if (!ReadFile(handle, &chRead, 1, &dwRead, nullptr))
        return -1; // I/O-Fehler wie „keine Daten“/Abbruch -> -1

    if (dwRead != 1)
        return -1; // sollte bei blockierenden Timeouts nicht passieren

    return (int)chRead; // 0..255
}

int Serial::read(char* b, int len)
{
    // Dokument: -1, wenn „keine Bytes verfügbar“ ODER Port nicht geöffnet.
    if (handle == INVALID_HANDLE_VALUE || b == nullptr || len <= 0)
        return -1;

    DWORD got = 0;
    int i = 0;

    // 1) Mindestens 1 Byte lesen (blockierend)
    if (!ReadFile(handle, &b[i], 1, &got, nullptr) || got != 1)
        return -1; // -1 wie im Dokument

    i += 1;

    // 2) Alle SOFORT verfügbaren Bytes (ohne weiteres Blockieren) nachziehen
    while (i < len) {
        int avail = dataAvailable();
        if (avail <= 0) break;

        int want = std::min(avail, len - i);
        DWORD gotNow = 0;
        if (!ReadFile(handle, &b[i], (DWORD)want, &gotNow, nullptr) || gotNow == 0)
            break;

        i += (int)gotNow;
    }

    return i;  // >= 1
}

string Serial::readLine()
{
    // Dokument sagt: "liefert null, wenn nicht geöffnet". In C++ gibt es kein null-String.
    // Wir geben "" zurück, was im Nutzer-Code so behandelt werden sollte.
    if (handle == INVALID_HANDLE_VALUE)
        return string(); // "" ~ „null“ im Dokument

    const unsigned char LF = 0x0A; // '\n'
    string result;

    // Blockiere bis '\n'
    while (true) {
        int ch = read(); // nutzt unsere -1-Logik
        if (ch < 0) {

            return string();
        }
        if ((unsigned char)ch == LF) {
            // LF nicht übernehmen -> Zeile fertig
            return result;
        }
        result.push_back((char)ch);

        // Sicherheitsnetz gegen Endloszeilen (~1 MB)
        if (result.size() > (1u << 20)) {
            return result; // pragmatisch abbrechen
        }
    }
}

// ==============================
// Modem-/Handshake-Signale
// ==============================

void Serial::setRTS(bool arg)
{
    if (handle != INVALID_HANDLE_VALUE) {
        if (arg) EscapeCommFunction(handle, SETRTS);
        else     EscapeCommFunction(handle, CLRRTS);
    }
}

void Serial::setDTR(bool arg)
{
    if (handle != INVALID_HANDLE_VALUE) {
        if (arg) EscapeCommFunction(handle, SETDTR);
        else     EscapeCommFunction(handle, CLRDTR);
    }
}

bool Serial::isCTS()
{
    DWORD status = 0;
    GetCommModemStatus(handle, &status);
    return (status & MS_CTS_ON) != 0;
}

bool Serial::isDSR()
{
    DWORD status = 0;
    GetCommModemStatus(handle, &status);
    return (status & MS_DSR_ON) != 0;
}
