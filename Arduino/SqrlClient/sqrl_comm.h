#ifndef _SQRL_COMM_H_
#define _SQRL_COMM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**@brief SQRL command type. */
typedef enum {
    SQRL_CMD_INVALID = 0,
    SQRL_CMD_QUERY,
    SQRL_CMD_IDENT,
} sqrl_cmd_type_t;


/**@brief SQRL command. */
typedef struct {
    sqrl_cmd_type_t type;                                       //<< Type of SQRL command
    char* sks;                                                  //<< Site Key String
    char* server;                                               //<< Server response
    char* sin;                                                  //<< Secret index
    bool create_suk;                                            //<< Whether to create Server Unlock Key (or not)
} sqrl_cmd_t;


/**@brief SQRL comm module event type. */
typedef enum
{
    SQRL_COMM_EVT_COMMAND                                       //<< Received (and parsed) new SQRL command
} sqrl_comm_evt_type_t;

/**@brief SQRL comm module event. */
typedef struct
{
    sqrl_comm_evt_type_t evt_type;                              //<< Type of event.

    union
    {
        sqrl_cmd_t* p_cmd;                                      //<< SQRL command
    } evt;
} sqrl_comm_evt_t;


/**@brief SQRL comm module event handler type. */
typedef void (*sqrl_comm_evt_handler_t) (sqrl_comm_evt_t * p_evt);


/**@brief Initialize SQRL comm module.
 *
 * @param[in]   evt_handler     Event handler to be called for handling events in the SQRL comm module
 *
 * @return      NRF_SUCCESS on successful initialization of module, otherwise an error code.
 */
uint32_t sqrl_comm_init(sqrl_comm_evt_handler_t evt_handler);


/**@brief Handle single character building an SQRL command. */
void sqrl_comm_handle_input(char* in_buffer, size_t in_len);

/**@brief Signal that a command event has been handled (so we can start parsing the next). */
void sqrl_comm_command_handled(void);


#endif//_SQRL_COMM_H_
