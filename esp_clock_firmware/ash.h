/*
 * Arduino SHell
 * 
 * Simple shell for arduino microcontrollers that runs on the serial port
 */
#ifndef ASH_H
#define ASH_H

/*
 * Maximum accepted length for a command.
 * Commands longer than this will be truncated.
 */
#define MAX_CMD_LENGTH 80

struct ash_cmd {
  const char* cmd_name;
  int (*cmd_func)(const char*); // similarly to a shell comand we have an int return value and a string for the command parameters 
};

/*
 * This function runs repeatedly to parse user input
 */
 void ashTask(void);
 
/*
 * Implements a simple version of the echo command, printing param to the serial port
 */
int echo_cb(const char* param);

/*
 * Shows some help text regarding available commands
 */
int help_cb(const char* param);

/*
 * Report current uptime and cpu usage in a way similar to the unix 'uptime' command
 */
int uptime_cb(const char* param);

/*
 * Print some information regarding the wifi connection
 */
int wifi_info_cb(const char* param);

/*
 * Reboot/halt the device
 */
int shutdown_cb(const char* param);

/*
 * List files in the device filesystem
 */
int ls_cb(const char* param);

/*
 * Print file content
 */
int cat_cb(const char* param);

/*
 * Print some information regarding the wifi connection
 */
int invalid_command_cb(const char* param);

/*
 * Do nothing. Just return.
 */
int noop_cb(const char* param) { return 0; }

#endif
