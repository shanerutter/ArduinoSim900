#include <Arduino.h>
#include <SoftwareSerial.h>

// Software Serial
SoftwareSerial *SIM900 = NULL;

void sim900SendChar(const char cmd)
{
    SIM900->print(cmd);
}

void sim900SendCmd(const char* cmd)
{
    SIM900->print(cmd);
}

void sim900SendCmdFlash(const __FlashStringHelper* cmd)
{
    SIM900->print(cmd);
}

void sim900FlushSerial()
{
    while (SIM900->available() > 0) {
        SIM900->read();
    }
}

void sim900ClearBuffer(char* buffer, const int bufferSize, const int startingIdx = 0)
{
    for (int a=startingIdx; a < bufferSize; a++) {
        buffer[a] = '\0';
    }
}

bool sim900WaitForResponse(const char* pattern, const signed int timeout, char* buffer = NULL, const int bufferSize = 0)
{
    // This is how often in a 1 by 1 sequence we have matched the correct characters
    // We essentially want to keep checking the reponse data until repeatMatchCount == patternLength as then we have matched our pattern
    int patternMatchCount = 0; 
    int patternLength = strlen(pattern);
    signed long startTime = millis();

    // Keep track of buffer idx
    int bufferIdx = 0;

    if (buffer != NULL) {
        sim900ClearBuffer(buffer, bufferSize);
    }

    while (true) {
        if (SIM900->available() > 0) {
            char c = SIM900->read();

            if (buffer != NULL) {
                buffer[bufferIdx] = c;
            }

            patternMatchCount = (c == pattern[patternMatchCount]) ? patternMatchCount + 1 : 0;

            if (patternMatchCount == patternLength) {
                break;
            }

            bufferIdx = bufferIdx + 1;
        }

        // Has timeout been hit
        if ((signed int)millis() - startTime > timeout) {
            return false;
        }
    }

    // Remove pattern from buffer
    if (buffer != NULL) {
        bufferIdx = (strlen(buffer) - patternLength);
        sim900ClearBuffer(buffer, bufferSize, bufferIdx);
    }

    return true;
}

int sim900WaitForEitherResponse(const char* pattern1, const char* pattern2, const signed int timeout)
{
    // This is how often in a 1 by 1 sequence we have matched the correct characters
    // We essentially want to keep checking the reponse data until repeatMatchCount == patternLength as then we have matched our pattern
    int pattern1MatchCount = 0; 
    int pattern1Length = strlen(pattern1);
    int pattern2MatchCount = 0; 
    int pattern2Length = strlen(pattern2);

    // Keep track when started for timeout
    signed long startTime = millis();

    while (true) {
        if (SIM900->available() > 0) {
            char c = SIM900->read();

            pattern1MatchCount = (c == pattern1[pattern1MatchCount]) ? pattern1MatchCount + 1 : 0;
            pattern2MatchCount = (c == pattern2[pattern2MatchCount]) ? pattern2MatchCount + 1 : 0;

            if (pattern1MatchCount == pattern1Length) {
                return 1;
            }

            if (pattern2MatchCount == pattern2Length) {
                return 2;
            }
        }

        // Has timeout been hit
        if ((signed int)millis() - startTime > timeout) {
            return 0;
        }
    }

    return 0;
}

bool sim900SendAT()
{
    sim900FlushSerial();
    sim900SendCmdFlash(F("AT\r\n"));
    return sim900WaitForResponse("OK\r\n", 10000);
}

void sim900Init(const int pinRx, const int pinTx, const int baud)
{
    // Always check if already setup
    if (SIM900 == NULL) {
        SIM900 = new SoftwareSerial(pinRx, pinTx);
        SIM900->begin(baud);

        // Wait for sim900 serial connection
        while (!SIM900);
    }
}

void sim900End()
{
    // Flush and end
    sim900FlushSerial();
    SIM900->end();

    // Release from memory
    if (SIM900 != NULL) {
        free(SIM900);
    }
}

void sim900FillBufferUntil(char* buffer, const int bufferSize, const char* pattern)
{
    sim900WaitForResponse(pattern, 10, buffer, bufferSize);
}








/*
 * SMS
 */
bool sim900SendSms(const char *mobileNumber, const char *message)
{
    // Is sim900 ready?
    if (sim900SendAT()) {
        // AT command to set SIM900 to SMS mode
        sim900SendCmdFlash(F("AT+CMGF=1\r\n"));
        sim900WaitForResponse("OK\r\n", 2500);

        // Configure target
        sim900SendCmdFlash(F("AT+CMGS=\""));
        sim900SendCmd(mobileNumber);
        sim900SendCmdFlash(F("\"\r\n"));
        sim900WaitForResponse(">", 2500);

        // Send message
        sim900SendCmd(message);
        sim900SendChar((char)26);

        return sim900WaitForResponse("OK\r\n", 2500);
    }

    return false;
}

int sim900GetNextSms(char* buffer, char* message, char* phoneNumber, char* dateTime, const char* status, const int leaveUnmodified = 0)
{
    // "REC UNREAD","REC READ","STO UNSENT","STO SENT","ALL"

    // Convert leaveUnmodified to char*
    char charLeaveUnmodified[10];
    itoa(leaveUnmodified, charLeaveUnmodified, 10);

    // Message id
    int messageId = 0;

    // Is sim900 ready?
    if (sim900SendAT()) {
        sim900SendCmdFlash(F("AT+CMGL=\""));
        sim900SendCmd(status);
        sim900SendCmdFlash(F("\","));
        sim900SendCmd(charLeaveUnmodified);
        sim900SendCmdFlash(F("\r\n"));
        
        int response = sim900WaitForEitherResponse("+CMGL: ", "OK\r\n", 10000);
        if (response == 1) {
            // sms id
            sim900FillBufferUntil(buffer, 160, ",\"");
            messageId = atoi(buffer);

            // sms status
            sim900FillBufferUntil(buffer, 160, "\",\"");
            
            // sms number
            sim900FillBufferUntil(phoneNumber, 25, "\",\"");

            // unkown value, appears to be blank (maybe contact?)
            sim900FillBufferUntil(buffer, 160, "\",\"");

            // sms data / time
            sim900FillBufferUntil(dateTime, 25, "\"\r\n");

            // sms message
            sim900FillBufferUntil(message, 25, "\r\n");

            return messageId;
        } else if(response == 2) {
            return 0;
        }
    }

    return -1;
}

bool sim900DeleteSms(const int idx)
{
    // Convert leaveUnmodified to char*
    char charIdx[10];
    itoa(idx, charIdx, 10);

    if (sim900SendAT()) {
        sim900SendCmdFlash(F("AT+CMGD="));
        sim900SendCmd(charIdx);
        sim900SendCmdFlash(F("\r\n"));

        return sim900WaitForResponse("OK\r\n", 10000);
    }

    return false;
}

bool sim900NotificationsEnable()
{
    if (sim900SendAT()) {
        sim900SendCmdFlash(F("AT+CNMI=1,2,0,0,0\r\n"));
        return sim900WaitForResponse("OK\r\n", 10000);
    }

    return false;
}

bool sim900NotificationsDisable()
{
    if (sim900SendAT()) {
        sim900SendCmdFlash(F("AT+CNMI=0,0,0,0,0\r\n"));
        return sim900WaitForResponse("OK\r\n", 10000);
    }

    return false;
}