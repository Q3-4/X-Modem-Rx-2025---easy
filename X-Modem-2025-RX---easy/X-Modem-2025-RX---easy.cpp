#include <iostream>
#include <string>
#include "Serial.h"
using namespace std;

// Steuerzeichen
static const unsigned char SOH = 0x01; // Start Of Header
static const unsigned char ETX = 0x03; // Padding
static const unsigned char EOT = 0x04; // End Of Transmission
static const unsigned char ACK = 0x06; // Acknowledge
static const unsigned char NAK = 0x15; // No Acknowledge

// Blocklayout (vereinfachte Übung)
// | SOH | n | 255-n | Daten(5) | Checksum |
//    1     1    1        5          1   = 9
static const int DATABYTES = 5;
static const int BLOCKSIZE = 3 + DATABYTES + 1; // 9

unsigned char checksum5(const unsigned char* d) {
    int sum = 0;
    for (int i = 0; i < DATABYTES; ++i) sum += d[i];
    return (unsigned char)(sum % 256);
}

int main() {
    string portNr;
    cout << "COM Port Nummer: ";
    cin >> portNr;
    string port = "COM" + portNr;

    // Reihenfolge: (port, baud, dataBits, stopBits, parity)
    Serial com(port, 9600, 8, ONESTOPBIT, NOPARITY);

    if (!com.open()) {
        cout << "Konnte " << port << " nicht oeffnen.\n";
        return 1;
    }

    cout << "Empfaenger gestartet auf " << port << "\n";
    com.write(NAK); // Empfaenger signalisiert: bereit

    string nachricht;

    while (true) {
        int b0 = com.read();                 // blockiert bis 1 Byte da
        if (b0 == EOT) {                     // Sender beendet
            com.write(ACK);
            break;
        }
        if (b0 != SOH) continue;             // warte auf Start eines Blocks

        unsigned char blk[BLOCKSIZE];
        blk[0] = (unsigned char)b0;

        // Restliche 8 Bytes einlesen (n, 255-n, 5 Daten, checksum)
        for (int i = 1; i < BLOCKSIZE; ++i) {
            int bi = com.read();
            blk[i] = (unsigned char)bi;
        }

        // Header prüfen
        unsigned char n = blk[1];
        unsigned char inv = blk[2];
        if ((unsigned char)(255 - n) != inv) {
            com.write(NAK);
            continue;
        }

        // Prüfsumme prüfen
        unsigned char calc = checksum5(&blk[3]);
        if (calc != blk[8]) {
            com.write(NAK);
            continue;
        }

        // Daten übernehmen (ETX nicht übernehmen)
        for (int i = 0; i < DATABYTES; ++i) {
            unsigned char c = blk[3 + i];
            if (c != ETX) nachricht.push_back((char)c);
        }

        com.write(ACK); // Alles gut
    }

    cout << "Empfangene Nachricht: " << nachricht << "\n";
    com.close();
    return 0;
}
