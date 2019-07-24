#include <ArduinoJson.h>
#include <RNG.h>
//#include <TransistorNoiseSource.h>
//#include <RingOscillatorNoiseSource.h>

#include "sqrl_client.h"


HardwareSerial* debugger = NULL;
HardwareSerial* inStream;

// Noise source to seed the random number generator.
//TransistorNoiseSource noise(A1);
//RingOscillatorNoiseSource noise;

char buffer[1024];
size_t buffer_idx = 0;

uint8_t iuk[] = {0xa2,0x43,0xbf,0xb0,0xed,0x04,0x56,0x5d,0x04,0x61,0xa7,0x73,0x1a,0x8f,0xad,0x18,0xde,0x4b,0xdd,0xf8,0xe3,0x83,0x38,0x53,0x92,0x6c,0xcf,0xaf,0x2e,0x2e,0x04,0xa6};
uint8_t imk[] = {0x21,0xd7,0x08,0x94,0x57,0x5e,0x6b,0x6e,0xfe,0x99,0x1f,0xb8,0x6a,0x98,0x68,0xa4,0x9f,0x3a,0x72,0x04,0x0e,0x88,0x25,0x2d,0x82,0xbe,0x5a,0x3a,0xc6,0xc3,0xaa,0x23};
uint8_t ilk[32];


void setup() {
    inStream = &Serial;
    debugger = &Serial1;

    inStream->begin(115200);
    while (!inStream);

	if (debugger) {
		// Setup serial port for debugging
		debugger->begin(115200);
		while (!debugger);
		debugger->println("");
		debugger->println("Started...");
	}

    RNG.begin("SqrlClient");
    //RNG.addNoiseSource(noise); // TODO: Need noise source!!!

    sqrl_get_ilk_from_iuk(ilk, iuk);
}


void loop() {
    RNG.loop();

	if (inStream->available())
	{
		char newByte = inStream->read();
        buffer[buffer_idx] = newByte;
        buffer_idx++;

        //if (debugger) debugger->print("Got byte: '");
        //if (debugger) debugger->print(newByte);
        //if (debugger) debugger->println("'");

		if (newByte == '\n')
        {
            if (debugger) debugger->print("Got line: ");
            for (int i = 0; i < buffer_idx; i++) {
                if (debugger) debugger->print(buffer[i]);
            }

            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, buffer, buffer_idx);
            buffer_idx = 0;
            if (error)
            {
                if (debugger) debugger->print(F("deserializeJson() failed: "));
                if (debugger) debugger->println(error.c_str());
                return;
            }

            ClientResponse resp;
            if (doc["cmd"] == "query") {
                if (debugger) debugger->println("query");

                String sks = doc["sks"];
                String server = doc["server"];
                if (debugger) debugger->println(sks);
                resp = sqrl_query(imk, sks.c_str(), server.c_str());
            } else if (doc["cmd"] == "ident") {
                if (debugger) debugger->println("ident");

                uint8_t rlk[32];
                while (! RNG.available(sizeof(rlk))) {
                    if (debugger) debugger->println("Gather randomness");
                    delay(500);
                }
                RNG.rand(rlk, sizeof(rlk));

                String sks = doc["sks"];
                String server = doc["server"];
                // TODO: if "sin" not in doc...
                String sin_ = doc["sin"];
                bool create_suk = doc["create_suk"];
                resp = sqrl_ident(ilk, imk, rlk,
                        sks.c_str(), server.c_str(), sin_.c_str(), create_suk);
                if (debugger) debugger->println("Done...");
            }

            doc.clear();
            doc["client"] = resp.client.c_str();
            doc["server"] = resp.server.c_str();
            doc["ids"] = resp.ids.c_str();

            serializeJson(doc, buffer, sizeof(buffer));
            inStream->println(buffer);
            buffer_idx = 0;
            memset(buffer, 0, sizeof(buffer));
        }
	}
    //else
    //{
    //    delay(100);
    //}
}
