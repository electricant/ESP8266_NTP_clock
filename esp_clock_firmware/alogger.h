/*
 * 
 */
#ifndef ALOGGER_H
#define ALOGGER_H

/*
 * Log buffer size is LINE_LENGTH*NUM_LINES.
 * 
 * LINE_LENGTH sets the maximum length for a line in the log. 
 *             Line lengths exceeding this value will be truncated.
 * NUM_LINES sets the maximum number of lines held in the log buffer. 
 *           The buffer is circular and older log entries will be deleted.
 */
#define LINE_LENGTH 100
// using power of 2 makes the logger faster
#define NUM_LINES (1 << 7) // 2^7 = 128

// Facility used for logging info messages
#define LOG_INFO(fmt, ...) logbuf_put("[%lu,%s(),%lu]: " fmt, millis(), __func__, __LINE__, ##__VA_ARGS__)

// Facility used for logging error messages.
// To be used for showstopper errors that should never happen
#define LOG_ERROR(fmt, ...) \
  do { Serial.printf("ERROR [%lu,%s(),%lu]: " fmt "\n", millis(), __func__, __LINE__, ##__VA_ARGS__); \
       logbuf_put("ERROR [%lu,%s(),%lu]: " fmt "\n", millis(), __func__, __LINE__, ##__VA_ARGS__); }while(0)

/*
 * Put a string into the (circular) buffer
 */
void logbuf_put(const char* fmt, ...);

/*
 * For ash to print the log
 */
int logcat_cb(const char* param);

#endif
