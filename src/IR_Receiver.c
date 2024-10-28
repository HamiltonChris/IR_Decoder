#include "IR_Receiver.h"

#include <string.h>


void IR_Receiver_Init(IR_Receiver_t *receiver)
{
    receiver->currentIndex = 0;
    receiver->pulseNumber = 0;
    receiver->state = LeadIn;
    memset(receiver->buffer, 0, receiver->bufferSize * sizeof(*(receiver->buffer)));
}

void IR_Receiver_Decode(IR_Receiver_t *receiver)
{
    while (receiver->buffer[receiver->currentIndex] > 0 &&
           receiver->buffer[receiver->currentIndex + 1] > 0 &&
           receiver->buffer[receiver->currentIndex + 2] > 0)
    {
        switch (receiver->state)
        {
        case LeadIn:
        {
            uint32_t time0 = receiver->buffer[receiver->currentIndex];
            uint32_t time1 = receiver->buffer[receiver->currentIndex + 1];
            uint32_t time2 = receiver->buffer[receiver->currentIndex + 2];

            uint32_t fallingTime = 0;
            uint32_t risingTime = 0;

            if (time0 > time1)
            {
                fallingTime = (receiver->period - time0 + time1) / receiver->clockSpeed;
            }
            else
            {
                fallingTime = (time1 - time0) / receiver->clockSpeed;
            }

            if (time1 > time2)
            {
                risingTime = (receiver->period - time1 + time2) / receiver->clockSpeed;
            }
            else
            {
                risingTime = (time2 - time1) / receiver->clockSpeed;
            }

            if (fallingTime < 9250 && fallingTime > 8750 && risingTime < 4750 && risingTime > 4250)
            {
                receiver->state = Address;
                receiver->buffer[receiver->currentIndex] = 0;
                receiver->currentIndex++;
            }
            break;
        }
        case Address:
        {
            uint32_t time0 = receiver->buffer[receiver->currentIndex];
            uint32_t time1 = receiver->buffer[receiver->currentIndex + 1];
            uint32_t time2 = receiver->buffer[receiver->currentIndex + 2];

            uint32_t fallingTime = 0;
            uint32_t risingTime = 0;

            if (time0 > time1)
            {
                fallingTime = (receiver->period - time0 + time1) / receiver->clockSpeed;
            }
            else
            {
                fallingTime = (time1 - time0) / receiver->clockSpeed;
            }

            if (time1 > time2)
            {
                risingTime = (receiver->period - time1 + time2) / receiver->clockSpeed;
            }
            else
            {
                risingTime = (time2 - time1) / receiver->clockSpeed;
            }

            if (fallingTime < 600 && fallingTime > 500)
            {
                if (risingTime > 1625 && risingTime < 1725)
                {
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
        }
        case AddressInv:
            break;
        default:
        }

        receiver->buffer[receiver->currentIndex] = 0;
        receiver->currentIndex++;
    }
}