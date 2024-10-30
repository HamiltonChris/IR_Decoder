#include "IR_Receiver.h"

#include <string.h>

static uint32_t getPulseTime(uint32_t time0, uint32_t time1, uint32_t period, uint8_t clockSpeed);

void IR_Receiver_Init(IR_Receiver_t *receiver)
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
    }
    memset(receiver->buffer, 0, receiver->bufferSize * sizeof(*(receiver->buffer)));
}

void IR_Receiver_Decode(IR_Receiver_t *receiver)
{
    uint32_t time0 = receiver->buffer[receiver->currentIndex];
    uint32_t time1 = receiver->buffer[receiver->currentIndex + 1];
    uint32_t time2 = receiver->buffer[receiver->currentIndex + 2];

    while (time0 > 0 && time1 > 0 && time2 > 0)
    {
        uint32_t fallingTime = getPulseTime(time0, time1, receiver->period, receiver->clockSpeed);
        uint32_t risingTime = getPulseTime(time1, time2, receiver->period, receiver->clockSpeed);

        switch (receiver->state)
        {
        case LeadIn:
            if (fallingTime < 9250 && fallingTime > 8750 && risingTime < 4750 && risingTime > 4250)
            {
                receiver->state = Address;
                receiver->buffer[receiver->currentIndex] = 0;
                receiver->currentIndex++;
            }
            break;
        case Address:
            if (fallingTime < 600 && fallingTime > 500)
            {
                if (risingTime > 1500 && risingTime < 1800)
                {
                    receiver->message->address |= 1 << receiver->pulseNumber;
                    receiver->buffer[receiver->currentIndex] = 0;
                    receiver->currentIndex++;
                    receiver->pulseNumber++;
                }
                else if (risingTime < 600 && risingTime > 500)
                {
                    receiver->buffer[receiver->currentIndex] = 0;
                    receiver->currentIndex++;
                    receiver->pulseNumber++;
                }
            }

            if (receiver->pulseNumber == 8)
            {
                receiver->pulseNumber = 0;
                receiver->state = AddressInv;
            }

            break;
        case AddressInv:
            if (fallingTime < 600 && fallingTime > 500)
            {
                if (risingTime > 1500 && risingTime < 1800)
                {
                    receiver->message->addressInv |= 1 << receiver->pulseNumber;
                    receiver->buffer[receiver->currentIndex] = 0;
                    receiver->currentIndex++;
                    receiver->pulseNumber++;
                }
                else if (risingTime < 600 && risingTime > 500)
                {
                    receiver->buffer[receiver->currentIndex] = 0;
                    receiver->currentIndex++;
                    receiver->pulseNumber++;
                }
            }

            if (receiver->pulseNumber == 8)
            {
                receiver->pulseNumber = 0;
                receiver->state = Command;
            }
            break;
        case Command:
            if (fallingTime < 600 && fallingTime > 500)
            {
                if (risingTime > 1500 && risingTime < 1800)
                {
                    receiver->message->command |= 1 << receiver->pulseNumber;
                    receiver->buffer[receiver->currentIndex] = 0;
                    receiver->currentIndex++;
                    receiver->pulseNumber++;
                }
                else if (risingTime < 600 && risingTime > 500)
                {
                    receiver->buffer[receiver->currentIndex] = 0;
                    receiver->currentIndex++;
                    receiver->pulseNumber++;
                }
            }

            if (receiver->pulseNumber == 8)
            {
                receiver->pulseNumber = 0;
                receiver->state = CommandInv;
            }
            break;
        case CommandInv:
            break;
        default:
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