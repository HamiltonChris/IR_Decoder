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
    uint8_t repeat;
} IR_Message_t;

typedef struct IR_Decoder_s {
    uint32_t period;
    uint32_t *buffer;
    uint8_t bufferSize;
    uint8_t currentIndex;
    uint8_t pulseNumber;
    uint8_t clockSpeed; // MHz
    DecoderState state;
    IR_Message_t *message; // may need a 2nd struct
    void (*decodeCallback)(IR_Message_t*);
} IR_Decoder_t;

void IR_Decoder_Init(IR_Decoder_t *receiver);
void IR_Decoder_Decode(IR_Decoder_t *receiver);

#endif