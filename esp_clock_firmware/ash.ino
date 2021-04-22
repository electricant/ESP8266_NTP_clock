#include <FS.h>
#include <ESP8266WiFi.h>

#include "ash.h"

#include "qd_sched.h"
#include "alogger.h"

/*
 * Array containing the available commands.
 */
 struct ash_cmd commands[] = {
  {.cmd_name = "", .cmd_func = &noop_cb},
  {.cmd_name = "cat", .cmd_func = &cat_cb},
  {.cmd_name = "echo", .cmd_func = &echo_cb},
  {.cmd_name = "help", .cmd_func = &help_cb},
  {.cmd_name = "logcat", .cmd_func = &logcat_cb},
  {.cmd_name = "ls", .cmd_func = &ls_cb},
  {.cmd_name = "shutdown", .cmd_func = &shutdown_cb},
  {.cmd_name = "uptime", .cmd_func = &uptime_cb},
  {.cmd_name = "wifi-info", .cmd_func = &wifi_info_cb},
  {.cmd_name = NULL, .cmd_func = &invalid_command_cb} // this should always be the last one
};

void ashTask(void)
{
  static bool showPrompt = true;
  size_t avail = Serial.available();

  if (showPrompt)
  {
    showPrompt = false;

    Serial.print("ash@");
    Serial.print(WiFi.hostname());
    Serial.print(" > ");
  }
  
  if (avail > 0) {
    char buf[MAX_CMD_LENGTH + 1] = {0};
    Serial.readBytesUntil('\n', buf, min((size_t)MAX_CMD_LENGTH, avail));

    // echo received command
    Serial.println(buf);
    
    // a command terminates with a space
    const char* param = "";
    char* spaceptr = strchr(buf, ' ');
    if (spaceptr != NULL) {
      param = spaceptr + 1;
      *spaceptr = 0; // buf now contains the command only (pointer magic!)
    }
    LOG_INFO("command: %s, params: %s", buf, param);
    
    ash_cmd *cmd = commands;
    while(cmd->cmd_name != NULL)
    {
      if(strcmp(cmd->cmd_name, buf) == 0)
        break;
      cmd++;
    }
    (*cmd->cmd_func)(param);
    
    // flush
    while (Serial.available())
      Serial.read();

    showPrompt = true;    
  }
}

int echo_cb(const char* param)
{
  Serial.println(param);
}

int help_cb(const char* param)
{
  Serial.println("Available commands: ");
  
  for(ash_cmd *cmd = commands; cmd->cmd_name != NULL; cmd++) {
    if (strlen(cmd->cmd_name))
      Serial.printf("\t%s\n", cmd->cmd_name);
  }
}

int uptime_cb(const char* param)
{
  Serial.print("up ");
  Serial.print(millis());
  Serial.print(" ms, load %: ");
  Serial.println(sched_get_CPU_usage());
}

int wifi_info_cb(const char* param)
{
  WiFi.printDiag(Serial);
  Serial.print("RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  Serial.print("inet addr: ");
  Serial.print(WiFi.localIP());
  Serial.print(" netmask: ");
  Serial.println(WiFi.subnetMask());
}

int shutdown_cb(const char* param)
{
  // param should be in the form "-h" or "-r"
  if (strlen(param) != 2)
    goto err;

  if (param[1] == 'h')
    ESP.deepSleep(0); // deep sleep forever
  else if (param[1] == 'r')
    ESP.restart();

err:
  Serial.println("Parameters should be '-r' to reboot or '-h' to halt.");
}

int ls_cb(const char* param)
{
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS.");
    return 1;
  }
 
  Dir root = SPIFFS.openDir("/"); // there are actually no dirs in SPIFFS but this works
  
  while (root.next()) {
    String fname = root.fileName();
    int len = fname.length() + 1; 
    char fname_cstr[len];
    fname.toCharArray(fname_cstr, len);
    
    if ((strlen(param) == 2) && (param[1] == 'l'))
      Serial.printf("%s\t\t%ul\n", fname_cstr, root.fileSize());
    else
      Serial.println(fname);
  }
}

int cat_cb(const char* param)
{
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS.");
    return 1;
  }

  File fl = SPIFFS.open(param, "r");
  if(!fl) {
    Serial.printf("Invalid file: %s\n", param);
    return 2;
  }

  while(fl.available()){
    Serial.write(fl.read());
  }

  fl.close();
}

int invalid_command_cb(const char* param)
{
  Serial.println("Command not recognized. Insert 'help' for a list of available commands.");
}
