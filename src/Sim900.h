#include <Arduino.h>
#include <SoftwareSerial.h>

void sim900Init(int pinRx, int pinTx, int baud);
void sim900End();
void sim900SendChar(const char cmd);
void sim900SendCmd(const char* cmd);
void sim900SendCmdFlash(const __FlashStringHelper* cmd);
void sim900FlushSerial();
void sim900ClearBuffer(char* buffer, const int bufferSize, const int startingIdx = 0);
bool sim900SendAT();
bool sim900WaitForResponse(const char* pattern, const signed int timeout, char* buffer = NULL, const int bufferSize = 0);
int sim900WaitForEitherResponse(const char* pattern1, const char* pattern2, const signed int timeout);
void sim900FillBufferUntil(char* buffer, const int bufferSize, const char* pattern);



/*
 * SMS
 */
bool sim900SendSms(const char *mobileNumber, const char *message);
int sim900GetNextSms(char* buffer, char* message, char* phoneNumber, char* dateTime, const char* status, const int leaveUnmodified = 0);
bool sim900DeleteSms(const int idx);
bool sim900NotificationsEnable();
bool sim900NotificationsDisable();