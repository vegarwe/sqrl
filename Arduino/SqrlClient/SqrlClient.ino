#include <RNG.h>
//#include <TransistorNoiseSource.h>
//#include <RingOscillatorNoiseSource.h>

#include "HardwareSerial.h"

#include "sqrl_client.h"
#include "sqrl_comm.h"


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

static volatile sqrl_cmd_t* mp_cmd;


static void on_sqrl_comm_evt(sqrl_comm_evt_t * p_evt)
{
    if (p_evt->evt_type == SQRL_COMM_EVT_COMMAND)
    {
        mp_cmd = p_evt->evt.p_cmd;
    }
}


static void handle_command(sqrl_cmd_t* p_cmd)
{
    if (p_cmd == NULL)
    {
        return;
    }

    if (p_cmd->type == SQRL_CMD_QUERY)
    {
        if (debugger) debugger->println("Got cmd: 'QUERY'");

        ClientResponse resp = sqrl_query(imk, p_cmd->sks, p_cmd->server);

        inStream->printf("\x02resp\x1equery\x1e%s\x1e%s\x1e%s\x03\n",
                resp.client.c_str(), p_cmd->server, resp.ids.c_str());

        if (debugger)
        {
            debugger->println(p_cmd->sks);
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

        ClientResponse resp = sqrl_ident(ilk, imk, rlk, p_cmd->sks, p_cmd->server, p_cmd->sin, p_cmd->create_suk);

        inStream->printf("\x02resp\x1eident\x1e%s\x1e%s\x1e%s\x03\n",
                resp.client.c_str(), p_cmd->server, resp.ids.c_str());
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
		debugger->println("Staring");
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
	if (numBytes = inStream->available())
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
        handle_command();

        mp_cmd = NULL;
        sqrl_comm_command_handled();
    }
}

