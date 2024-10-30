#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include <stdint.h>

typedef enum {
    LeadIn = 0,
    Address,
    AddressInv,
    Command,
    CommandInv,
} DecoderState;

typedef struct IR_Message_s {
    uint8_t address;
    uint8_t addressInv;
    uint8_t command;
    uint8_t commandInv;
} IR_Message_t;

typedef struct IR_Receiver_s {
    uint32_t period;
    uint32_t *buffer;
    uint8_t bufferSize;
    uint8_t currentIndex;
    uint8_t pulseNumber;
    uint8_t clockSpeed; // MHz
    DecoderState state;
    IR_Message_t *message; // may need a 2nd struct
} IR_Receiver_t;

void IR_Receiver_Init(IR_Receiver_t *receiver);
void IR_Receiver_Decode(IR_Receiver_t *receiver);

#endif