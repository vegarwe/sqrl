#include <stdio.h>

#include "sqrl_comm.h"

static sqrl_cmd_t* p_cmd = NULL;

// !gcc.exe -o sqrl_comm sqrl_comm.c sqrl_comm_test.c && ./sqrl_comm

void on_sqrl_comm_evt(sqrl_comm_evt_t * p_evt)
{
    printf("\n\nOhhh, yes...\n");

    if (p_evt->evt_type == SQRL_COMM_EVT_COMMAND)
    {
        printf("sks:    '%s'\n", p_evt->evt.p_cmd->sks);
        printf("server: '%s'\n", p_evt->evt.p_cmd->server);

        if (p_evt->evt.p_cmd->type == SQRL_CMD_IDENT)
        {
            printf("sin:    '%s'\n", p_evt->evt.p_cmd->sin);
            if (p_evt->evt.p_cmd->create_suk)
            {
                printf("create server unlock key\n");
            }
        }

        sqrl_comm_command_handled();
    }
}

int main(int argc, char** argv)
{
    printf("Hello\n");

    sqrl_comm_init(on_sqrl_comm_evt);

    char cmd[] = "garbage\x02query\x1e""www.grc.com\x1e""c3FybDovL3d3dy5ncmMuY29tL3Nxcmw_bnV0PW9HWEVVRW1Ua1BHMHowRWthM3BISlE\x03more Garbage!" \
        "\x02ident\x1e""www.grc.com\x1e""dmVyPTENCm51dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQp0aWY9NQ0KcXJ5PS9zcXJsP251dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzdWs9UEJGdWZRNmR2emgtYXB3dU1tXzR6MmFybmZNdjRDVUxVRTRWZVVFYWdWOA0KdXJsPWh0dHBzOi8vd3d3LmdyYy5jb20vc3FybC9kaWFnLmh0bT9fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzaW49MA0K\x1e""0\x1e""true\x03more Garbage!" \
        "\x02ident\x1e""www.grc.com\x1e""dmVyP[...]W49MA0K\x1e""3\x1e""false\x03" \
        "";

    for (int i = 0; i < sizeof(cmd); i++)
    {
        sqrl_comm_handle_input(cmd[i]);
    }

    return 0;
}
