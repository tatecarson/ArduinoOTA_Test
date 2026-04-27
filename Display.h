#pragma once
#include <Arduino.h>

extern char currentMessage[5];
extern bool otaInProgress;
extern bool otaError;

void displayBegin();
void renderMessage(const char* text);
void setMatrixMessage(const char* text);
void applyRemoteMessage(const char* text);
void showReady();
void showUpload();
void showError();
char cleanDisplayChar(char c);
