#include "IR_Decoder.h"

#include <string.h>

static void clearCurrentIndex(IR_Decoder_t *decoder);
static uint32_t getPulseTime(uint32_t time0, uint32_t time1, uint32_t period, uint8_t clockSpeed);
static int8_t decodePulse(uint32_t fallingTime, uint32_t risingTime);

void IR_Decoder_Init(IR_Decoder_t *decoder)
{
    decoder->currentIndex = 0;
    decoder->pulseNumber = 0;
    decoder->state = LeadIn;
    if (decoder->message)
    {
        decoder->message->address = 0;
        decoder->message->addressInv = 0;
        decoder->message->command = 0;
        decoder->message->commandInv = 0;
        decoder->message->repeat = 0;
    }
    memset(decoder->buffer, 0, decoder->bufferSize * sizeof(*(decoder->buffer)));
}

void IR_Decoder_Decode(IR_Decoder_t *decoder)
{
    uint32_t time0 = decoder->buffer[decoder->currentIndex];
    uint32_t time1 = decoder->buffer[(decoder->currentIndex + 1) % decoder->bufferSize];
    uint32_t time2 = decoder->buffer[(decoder->currentIndex + 2) % decoder->bufferSize];

    while (time0 > 0 && time1 > 0 && time2 > 0)
    {
        uint32_t fallingTime = getPulseTime(time0, time1, decoder->period, decoder->clockSpeed);
        uint32_t risingTime = getPulseTime(time1, time2, decoder->period, decoder->clockSpeed);
        int8_t signal = decodePulse(fallingTime, risingTime);

        switch (decoder->state)
        {
        case LeadIn:
            if (fallingTime < 9250 && fallingTime > 8750)
            {
                if (risingTime < 4750 && risingTime > 4250)
                {
                    decoder->state = Address;

                    // clear message buffer for new message
                    decoder->message->address = 0;
                    decoder->message->addressInv = 0;
                    decoder->message->command = 0;
                    decoder->message->commandInv = 0;
                    decoder->message->repeat = 0;

                    clearCurrentIndex(decoder);
                }
                else if (risingTime < 2750 && risingTime > 2250)
                {
                    decoder->message->repeat++;
                    decoder->decodeCallback(decoder->message);
                    clearCurrentIndex(decoder);
                    clearCurrentIndex(decoder);
                    clearCurrentIndex(decoder);
                }
            }
            break;
        case Address:
            if (signal >= 0)
            {
                decoder->message->address |= signal << decoder->pulseNumber;
                decoder->pulseNumber++;
            }

            if (decoder->pulseNumber == 8)
            {
                decoder->pulseNumber = 0;
                decoder->state = AddressInv;
            }

            break;
        case AddressInv:
            if (signal >= 0)
            {
                decoder->message->addressInv |= signal << decoder->pulseNumber;
                decoder->pulseNumber++;
            }

            if (decoder->pulseNumber == 8)
            {
                decoder->pulseNumber = 0;
                decoder->state = Command;
            }
            break;
        case Command:
            if (signal >= 0)
            {
                decoder->message->command |= signal << decoder->pulseNumber;
                decoder->pulseNumber++;
            }

            if (decoder->pulseNumber == 8)
            {
                decoder->pulseNumber = 0;
                decoder->state = CommandInv;
            }
            break;
        case CommandInv:
            if (signal >= 0)
            {
                decoder->message->commandInv |= signal << decoder->pulseNumber;
                decoder->pulseNumber++;
            }

            if (decoder->pulseNumber == 8)
            {
                decoder->decodeCallback(decoder->message);
                decoder->pulseNumber = 0;
                decoder->state = LeadIn;
                // there is an extra rising time at the end of the signal that needs to be removed
                clearCurrentIndex(decoder);
                clearCurrentIndex(decoder);
                // Note: weird edge case when DMA doesn't have the last value but needs to delete it
            }
            break;
        default:
        }

        if (signal >= 0)
        {
            clearCurrentIndex(decoder);
        }

        clearCurrentIndex(decoder);

        time0 = decoder->buffer[decoder->currentIndex];
        time1 = decoder->buffer[(decoder->currentIndex + 1) % decoder->bufferSize];
        time2 = decoder->buffer[(decoder->currentIndex + 2) % decoder->bufferSize];
    }
}

static void clearCurrentIndex(IR_Decoder_t *decoder)
{
        decoder->buffer[decoder->currentIndex] = 0;
        decoder->currentIndex = (decoder->currentIndex + 1) % decoder->bufferSize;
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