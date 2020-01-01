#include <RNG.h>
//#include <TransistorNoiseSource.h>
//#include <RingOscillatorNoiseSource.h>

#include "HardwareSerial.h"

#include "sqrl_client.h"
#include "sqrl_comm.h"
#include "sqrl_s4.h"


HardwareSerial* debugger = NULL;
HardwareSerial* inStream;

// Noise source to seed the random number generator.
//TransistorNoiseSource noise(A1);
//RingOscillatorNoiseSource noise;

char buffer[1024];
size_t buffer_idx = 0;

// Hard coded keys for now
uint8_t iuk[] = {0xa2,0x43,0xbf,0xb0,0xed,0x04,0x56,0x5d,0x04,0x61,0xa7,0x73,0x1a,0x8f,0xad,0x18,0xde,0x4b,0xdd,0xf8,0xe3,0x83,0x38,0x53,0x92,0x6c,0xcf,0xaf,0x2e,0x2e,0x04,0xa6};
uint8_t imk[] = {0x21,0xd7,0x08,0x94,0x57,0x5e,0x6b,0x6e,0xfe,0x99,0x1f,0xb8,0x6a,0x98,0x68,0xa4,0x9f,0x3a,0x72,0x04,0x0e,0x88,0x25,0x2d,0x82,0xbe,0x5a,0x3a,0xc6,0xc3,0xaa,0x23};
uint8_t ilk[32];

// Hard coded sqrl binary
char sqrlbinary[] = "sqrldata}\x00\x01\x00-\x00\"wQ\x12""2\x0e\xb5\x89""1\xfep\x97\xef\xf2""e]\xf6\x0fg\a\x8c_\xda\xd4\xe0Z\xe0\xb8\t\x96\x00\x00\x00\xf3\x01\x04\x05\x0f\x00\x02""3\x88\xcd\xa0\xd7WN\xf7\x8a\xd1""9\xf8\x1c]\x13\x87\x06\xc6\xe8\xf8\xb0""8\xf6\x14\xd9m\x9e\xf6|\x94\xa4\x1f""F\xab}\x0e\xd3\xbf\xa3r\xa3^\xb4\xfb\xcc\xe7\x8cQ\x8d\x8dyRl\x05\xf1\x19|\x90\x03\x06\t\xe0\xb3\x85H\x8c\xe0\xa6\x0fQm\xf6\x94q6-\xee\xe0\xe9I\x00\x02\x00\xea\xde\x04q\xa1\xfaO\x8f\x1c\xf5""e\xea\xb3)-^\t\xa5\x00\x00\x00\xf9o$\"\x9e\x91\xa6\xa9k\xde\xe2z^&j\xa6\x15\xb5\x04\xf4P\x01""e\xcc\xfa\xa8V\xd7\xf4\x94L\xea\xea\xdd><\xcb""C\xc5+\xeb\xaf\x18\x88\xf9\xa6\xd4\xce";

static volatile sqrl_cmd_t* mp_cmd;


static void on_sqrl_comm_evt(sqrl_comm_evt_t * p_evt)
{
    if (p_evt->evt_type == SQRL_COMM_EVT_COMMAND)
    {
        mp_cmd = p_evt->evt.p_cmd;
    }
}


static void handle_command(volatile sqrl_cmd_t* p_cmd)
{
    if (p_cmd == NULL)
    {
        return;
    }

    if (p_cmd->type == SQRL_CMD_QUERY)
    {
        if (debugger) debugger->println("Got cmd: 'QUERY'");

        ClientResponse resp = sqrl_query(imk, p_cmd->params.sqrl_cmd.sks, p_cmd->params.sqrl_cmd.server);

        inStream->printf("\x02resp\x1equery\x1e%s\x1e%s\x1e%s\x03\n",
                resp.client.c_str(), p_cmd->params.sqrl_cmd.server, resp.ids.c_str());

        if (debugger)
        {
            debugger->println(p_cmd->params.sqrl_cmd.sks);
            debugger->print( "resp.client ");
            debugger->println(resp.client.c_str());
        }
    }
    else if (p_cmd->type == SQRL_CMD_IDENT)
    {
        if (debugger) debugger->println("Got cmd: 'IDENT'");

        uint8_t rlk[32];
        while (! RNG.available(sizeof(rlk))) {
            if (debugger) debugger->println("Gather randomness");
            delay(500);
        }
        RNG.rand(rlk, sizeof(rlk));

        ClientResponse resp = sqrl_ident(ilk, imk, rlk, p_cmd->params.sqrl_cmd.sks,
                p_cmd->params.sqrl_cmd.server, p_cmd->params.sqrl_cmd.sin,
                p_cmd->params.sqrl_cmd.create_suk);

        inStream->printf("\x02resp\x1eident\x1e%s\x1e%s\x1e%s\x03\n",
                resp.client.c_str(), p_cmd->params.sqrl_cmd.server, resp.ids.c_str());
    }
    else if (p_cmd->type == SQRL_CMD_UNLOCK)
    {
        // Output from sqrl_s4.py:get_imk_ilk_from_password
        uint8_t key[] = {0x44,0xb6,0xb3,0xea,0x66,0x01,0x59,0x82,0xe5,0x5f,0xe8,0xf0,0xea,0x5c,0x11,0xe0,0x10,0x67,0x61,0x59,0xfe,0x02,0xed,0x70,0xd6,0xfb,0xf6,0x87,0x6b,0x49,0x77,0x4d};

        sqrl_s4_identity_t identity;
        int res = sqrl_s4_decode((uint8_t*)sqrlbinary, &identity);
        if (res != 0)
        {
            inStream->printf("\x02resp\x1eunlock\x1e""failed\x1eUnable to parse identity\x03\n");
            return;
        }

        if (! get_imk_ilk_from_scryptpassword(&identity, key, imk, ilk))
        {
            inStream->printf("\x02resp\x1eunlock\x1e""failed\x1eIdentity decryption failed\x03\n");
            return;
        }

        inStream->printf("\x02resp\x1eunlock\x1esuccess\x1epass\x03\n");
    }
    else
    {
        if (debugger) debugger->println("err: Invalid command");

        inStream->print("\x02log\x1e err: Invalid command\x03\n");
    }
}


void setup() {
    inStream = &Serial;
    debugger = &Serial1;

	if (debugger) {
		// Setup serial port for debugging
		debugger->begin(115200);
		while (!debugger);
		debugger->println("");
		debugger->println("Starting...");
	}

    inStream->begin(115200);
    while (!inStream); // TODO: Dereference to get bool operator overload?

    RNG.begin("SqrlClient");
    //RNG.addNoiseSource(noise); // TODO: Need noise source!!!

    sqrl_get_ilk_from_iuk(ilk, iuk);

    sqrl_comm_init(on_sqrl_comm_evt);
}


void loop() {
    RNG.loop();

    int numBytes = 0;
	if ((numBytes = inStream->available()) > 0)
	{
        while (numBytes--)
        {
            char newByte = inStream->read();
            sqrl_comm_handle_input(&newByte, 1);

            //if (debugger) debugger->print("Got byte: '");
            //if (debugger) debugger->print(newByte);
            //if (debugger) debugger->println("'");
        }
	}

    if (mp_cmd != NULL)
    {
        handle_command(mp_cmd);

        mp_cmd = NULL;
        sqrl_comm_command_handled();
    }
}

