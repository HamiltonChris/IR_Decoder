#include "IR_Decoder.h"

#include <string.h>

static uint32_t getPulseTime(uint32_t time0, uint32_t time1, uint32_t period, uint8_t clockSpeed);
static int8_t decodePulse(uint32_t fallingTime, uint32_t risingTime);

void IR_Decoder_Init(IR_Decoder_t *receiver)
{
    receiver->currentIndex = 0;
    receiver->pulseNumber = 0;
    receiver->state = LeadIn;
    if (receiver->message)
    {
        receiver->message->address = 0;
        receiver->message->addressInv = 0;
        receiver->message->command = 0;
        receiver->message->commandInv = 0;
        receiver->message->repeat = 0;
    }
    memset(receiver->buffer, 0, receiver->bufferSize * sizeof(*(receiver->buffer)));
}

void IR_Decoder_Decode(IR_Decoder_t *receiver)
{
    uint32_t time0 = receiver->buffer[receiver->currentIndex];
    uint32_t time1 = receiver->buffer[receiver->currentIndex + 1];
    uint32_t time2 = receiver->buffer[receiver->currentIndex + 2];

    while (time0 > 0 && time1 > 0 && time2 > 0)
    {
        uint32_t fallingTime = getPulseTime(time0, time1, receiver->period, receiver->clockSpeed);
        uint32_t risingTime = getPulseTime(time1, time2, receiver->period, receiver->clockSpeed);
        int8_t signal = decodePulse(fallingTime, risingTime);

        switch (receiver->state)
        {
        case LeadIn:
            if (fallingTime < 9250 && fallingTime > 8750)
            {
                if (risingTime < 4750 && risingTime > 4250)
                {
                    receiver->state = Address;

                    // clear message buffer for new message
                    receiver->message->address = 0;
                    receiver->message->addressInv = 0;
                    receiver->message->command = 0;
                    receiver->message->commandInv = 0;
                    receiver->message->repeat = 0;

                    receiver->buffer[receiver->currentIndex] = 0;
                    receiver->currentIndex++;
                }
                else if (risingTime < 2750 && risingTime > 2250)
                {
                    receiver->message->repeat++;
                    receiver->decodeCallback(receiver->message);
                    receiver->buffer[receiver->currentIndex] = 0;
                    receiver->currentIndex++;
                    receiver->buffer[receiver->currentIndex] = 0;
                    receiver->currentIndex++;
                    receiver->buffer[receiver->currentIndex] = 0;
                    receiver->currentIndex++;
                }
            }
            break;
        case Address:
            if (signal >= 0)
            {
                receiver->message->address |= signal << receiver->pulseNumber;
                receiver->pulseNumber++;
            }

            if (receiver->pulseNumber == 8)
            {
                receiver->pulseNumber = 0;
                receiver->state = AddressInv;
            }

            break;
        case AddressInv:
            if (signal >= 0)
            {
                receiver->message->addressInv |= signal << receiver->pulseNumber;
                receiver->pulseNumber++;
            }

            if (receiver->pulseNumber == 8)
            {
                receiver->pulseNumber = 0;
                receiver->state = Command;
            }
            break;
        case Command:
            if (signal >= 0)
            {
                receiver->message->command |= signal << receiver->pulseNumber;
                receiver->pulseNumber++;
            }

            if (receiver->pulseNumber == 8)
            {
                receiver->pulseNumber = 0;
                receiver->state = CommandInv;
            }
            break;
        case CommandInv:
            if (signal >= 0)
            {
                receiver->message->commandInv |= signal << receiver->pulseNumber;
                receiver->pulseNumber++;
            }

            if (receiver->pulseNumber == 8)
            {
                receiver->decodeCallback(receiver->message);
                receiver->pulseNumber = 0;
                receiver->state = LeadIn;
                // there is an extra rising time at the end of the signal that needs to be removed
                receiver->buffer[receiver->currentIndex] = 0;
                receiver->currentIndex++;
                receiver->buffer[receiver->currentIndex] = 0;
                receiver->currentIndex++;
                // Note: weird edge case when DMA doesn't have the last value but needs to delete it
            }
            break;
        default:
        }

        if (signal >= 0)
        {
            receiver->buffer[receiver->currentIndex] = 0;
            receiver->currentIndex++;
        }

        receiver->buffer[receiver->currentIndex] = 0;
        receiver->currentIndex++;

        time0 = receiver->buffer[receiver->currentIndex];
        time1 = receiver->buffer[receiver->currentIndex + 1];
        time2 = receiver->buffer[receiver->currentIndex + 2];
    }
}

static uint32_t getPulseTime(uint32_t time0, uint32_t time1, uint32_t period, uint8_t clockSpeed)
{
    return time0 > time1 ? (period - time0 + time1) / clockSpeed : (time1 - time0) / clockSpeed;
}

static int8_t decodePulse(uint32_t fallingTime, uint32_t risingTime)
{
    if (fallingTime < 600 && fallingTime > 500)
    {
        if (risingTime > 1500 && risingTime < 1800)
        {
            return 1;
        }
        else if (risingTime < 600 && risingTime > 500)
        {
            return 0;
        }
    }

    return -1;
}