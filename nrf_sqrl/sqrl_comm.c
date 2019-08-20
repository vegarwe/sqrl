#include <string.h>

#include "sqrl_comm.h"


#define STX '\x02' // Start of text
#define ETX '\x03' // End of text
#define NAK '\x15' // Negative acknowledge
#define RS  '\x1e' // Record seperator

#define MAX_TOKENS 15


typedef enum {
    sqrl_comm_state_initial,
    sqrl_comm_state_find_end,
    sqrl_comm_state_handle_cmd,
} sqrl_comm_state_t;


static volatile sqrl_comm_state_t   m_state = sqrl_comm_state_initial;
static sqrl_comm_evt_handler_t      m_evt_handler;


static void handle_cmd(char* buffer);


void sqrl_comm_handle_input(char c)
{
    static char     s_buffer[2048]; // TODO: Find proper size
    static uint32_t s_bufidx = 0;

    if (m_evt_handler == NULL)
    {
        return; // TODO: Report error somehow
    }

    if (m_state == sqrl_comm_state_initial)
    {
        if (c == STX)
        {
            s_bufidx = 0;
            m_state = sqrl_comm_state_find_end;
        }
        // Ignore all other characters untill STX
    }
    else if (m_state == sqrl_comm_state_find_end)
    {
        s_buffer[s_bufidx++] = c;

        if (s_bufidx > sizeof(s_buffer))
        {
            m_state = sqrl_comm_state_initial;
            // TODO: Report error somehow
        }

        if (c == '\0')
        {
            m_state = sqrl_comm_state_initial;
            // TODO: Report error somehow
        }

        if (c == ETX)
        {
            m_state = sqrl_comm_state_handle_cmd;
            s_buffer[s_bufidx-1] = '\0';

            handle_cmd(s_buffer);
        }
    }
}

static char *strsep(char ** p_s)
{
    char * ret = *p_s;

    if (*p_s == NULL)
    {
        return NULL;
    }

    while (**p_s != '\0')
    {
        if (**p_s == RS)
        {
            **p_s = '\0';
            (*p_s)++;
            return ret;
        }
        else
        {
            (*p_s)++;
        }
    }

    *p_s = NULL;
    return ret;
}


static void handle_cmd(char* buffer)
{
    static sqrl_cmd_t s_cmd;

    sqrl_comm_evt_t evt;

    char*       tokens[MAX_TOKENS];
    uint32_t    token_cnt = 0;

    // Split into tokens
    while ((tokens[token_cnt] = strsep(&buffer)) != NULL)
    {
        token_cnt++;
        if (token_cnt >= MAX_TOKENS)
        {
            break;
        }
    }

    // Debug output of parsed input
    //printf("token_cnt %d\n", token_cnt);
    //for (int i = 0; i < token_cnt; i++)
    //{
    //    printf("  tokens: '%s'\n", tokens[i]);
    //}

    // Handle invalid (too short) command
    if (token_cnt < 3)
    {
        return; // TODO: Report error somehow
    }

    // Assign fixed params
    s_cmd.sks = tokens[1];
    s_cmd.server = tokens[2];

    // Check command type, handle variable params
    if (strcmp(tokens[0], "query") == 0)
    {
        s_cmd.type = SQRL_CMD_QUERY;

        evt.evt_type = SQRL_COMM_EVT_COMMAND;
        evt.evt.p_cmd = &s_cmd;
        m_evt_handler(&evt);
    }
    else if (strcmp(tokens[0], "ident") == 0)
    {
        s_cmd.type = SQRL_CMD_IDENT;

        if (token_cnt < 5)
        {
            return; // TODO: Report error somehow
        }

        s_cmd.sin = tokens[3];
        s_cmd.create_suk = strcmp(tokens[4], "true") == 0;

        evt.evt_type = SQRL_COMM_EVT_COMMAND;
        evt.evt.p_cmd = &s_cmd;
        m_evt_handler(&evt);
    }
    else
    {
        return; // TODO: Report error somehow
    }
}

uint32_t sqrl_comm_init(sqrl_comm_evt_handler_t evt_handler)
{
    m_state = sqrl_comm_state_initial;
    m_evt_handler = evt_handler;

    return 0;
}

void sqrl_comm_command_handled(void)
{
    m_state = sqrl_comm_state_initial;
}
