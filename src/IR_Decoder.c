#include "IR_Decoder.h"

#include <string.h>

#define LEADIN_LOWPULSE_LOWBOUND 8750
#define LEADIN_LOWPULSE_HIGHBOUND 9250
#define LEADIN_HIGHPULSE_LOWBOUND 4250
#define LEADIN_HIGHPULSE_HIGHBOUND 4750
#define SHORTPULSE_LOWBOUND 500
#define SHORTPULSE_HIGHBOUND 600
#define LONGPULSE_LOWBOUND 1500
#define LONGPULSE_HIGHBOUND 1800
#define REPEAT_HIGHPULSE_LOWBOUND 2250
#define REPEAT_HIGHPULSE_HIGHBOUND 2750

#define MAXPULSES 8

static void clearCurrentIndex(IR_Decoder_t *decoder);
static void clearMessage(IR_Message_t* message);
static uint32_t getPulseTime(uint32_t time0, uint32_t time1, uint32_t period, uint8_t clockSpeed);
static int8_t decodePulse(uint32_t fallingTime, uint32_t risingTime);
static uint8_t areTimestampsValid(uint32_t time0, uint32_t time1, uint32_t time2, uint32_t time3);

void IR_Decoder_Init(IR_Decoder_t *decoder)
{
    decoder->currentIndex = 0;
    decoder->pulseNumber = 0;
    decoder->clearLast = 0;
    decoder->state = LeadIn;
    if (decoder->message)
    {
        clearMessage(decoder->message);
    }
    memset(decoder->buffer, 0, decoder->bufferSize * sizeof(*(decoder->buffer)));
}

void IR_Decoder_Decode(IR_Decoder_t *decoder)
{
    uint32_t time0 = decoder->buffer[decoder->currentIndex];
    uint32_t time1 = decoder->buffer[(decoder->currentIndex + 1) % decoder->bufferSize];
    uint32_t time2 = decoder->buffer[(decoder->currentIndex + 2) % decoder->bufferSize];
    uint32_t time3 = decoder->buffer[(decoder->currentIndex + 3) % decoder->bufferSize];

    // check if cleanup from last decode call is required
    if (decoder->clearLast)
    {
        decoder->buffer[decoder->currentIndex ? decoder->currentIndex - 1 : decoder->bufferSize - 1] = 0;
        decoder->clearLast = 0;
    }

    while (areTimestampsValid(time0, time1, time2, time3))
    {
        uint32_t fallingTime = getPulseTime(time0, time1, decoder->period, decoder->clockSpeed);
        uint32_t risingTime = getPulseTime(time1, time2, decoder->period, decoder->clockSpeed);
        int8_t signal = decodePulse(fallingTime, risingTime);

        switch (decoder->state)
        {
        case LeadIn:
            if (fallingTime < LEADIN_LOWPULSE_HIGHBOUND && fallingTime > LEADIN_LOWPULSE_LOWBOUND)
            {
                if (risingTime < LEADIN_HIGHPULSE_HIGHBOUND && risingTime > LEADIN_HIGHPULSE_LOWBOUND)
                {
                    decoder->state = Address;

                    // clear message buffer for new message
                    clearMessage(decoder->message);
                }
                else if (risingTime < REPEAT_HIGHPULSE_HIGHBOUND && risingTime > REPEAT_HIGHPULSE_LOWBOUND)
                {
                    decoder->message->repeat++;
                    decoder->decodeCallback(decoder->message);
                    clearCurrentIndex(decoder);
                    clearCurrentIndex(decoder);
                }
            }
            break;
        case Address:
            if (signal >= 0)
            {
                decoder->message->address |= signal << decoder->pulseNumber;
            }
            else
            {
                decoder->message->addressError |= 1 << decoder->pulseNumber;
            }

            decoder->pulseNumber++;

            if (decoder->pulseNumber == MAXPULSES)
            {
                decoder->pulseNumber = 0;
                decoder->state = AddressInv;
            }

            break;
        case AddressInv:
            if (signal >= 0)
            {
                decoder->message->addressInv |= signal << decoder->pulseNumber;
            }
            else
            {
                decoder->message->addressInvError |= 1 << decoder->pulseNumber;
            }

            decoder->pulseNumber++;

            if (decoder->pulseNumber == MAXPULSES)
            {
                decoder->pulseNumber = 0;
                decoder->state = Command;
            }
            break;
        case Command:
            if (signal >= 0)
            {
                decoder->message->command |= signal << decoder->pulseNumber;
            }
            else
            {
                decoder->message->commandError |= 1 << decoder->pulseNumber;
            }

            decoder->pulseNumber++;

            if (decoder->pulseNumber == MAXPULSES)
            {
                decoder->pulseNumber = 0;
                decoder->state = CommandInv;
            }
            break;
        case CommandInv:
            if (signal >= 0)
            {
                decoder->message->commandInv |= signal << decoder->pulseNumber;
            }
            else
            {
                decoder->message->commandInvError |= 1 << decoder->pulseNumber;
            }

            decoder->pulseNumber++;

            if (decoder->pulseNumber == MAXPULSES)
            {
                decoder->decodeCallback(decoder->message);
                decoder->pulseNumber = 0;
                decoder->state = LeadIn;
                clearCurrentIndex(decoder);
                // there is an extra rising time at the end of the signal that needs to be removed
                clearCurrentIndex(decoder);
                // checking if the extra element is empty and setting a flag to erase it next decode call
                if (decoder->buffer[(decoder->currentIndex + 1) % decoder->bufferSize] == 0)
                {
                    decoder->clearLast = 1;
                }
            }
            break;
        default:
        }

        clearCurrentIndex(decoder);
        clearCurrentIndex(decoder);

        time0 = decoder->buffer[decoder->currentIndex];
        time1 = decoder->buffer[(decoder->currentIndex + 1) % decoder->bufferSize];
        time2 = decoder->buffer[(decoder->currentIndex + 2) % decoder->bufferSize];
        time3 = decoder->buffer[(decoder->currentIndex + 3) % decoder->bufferSize];
    }
}


static uint8_t areTimestampsValid(uint32_t time0, uint32_t time1, uint32_t time2, uint32_t time3)
{
    return (time1 > 0 && time2 > 0) ||
           (time0 > time2 && time0 > 0 && time2 > 0) ||
           (time0 > 0 && time1 > 0 && time1 > time3 && time3 > 0);
}

static void clearCurrentIndex(IR_Decoder_t *decoder)
{
        decoder->buffer[decoder->currentIndex] = 0;
        decoder->currentIndex = (decoder->currentIndex + 1) % decoder->bufferSize;
}

static void clearMessage(IR_Message_t* message)
{
    message->address = 0;
    message->addressInv = 0;
    message->command = 0;
    message->commandInv = 0;
    message->repeat = 0;
    message->addressError = 0;
    message->addressInvError = 0;
    message->commandError = 0;
    message->commandInvError = 0;
}

static uint32_t getPulseTime(uint32_t time0, uint32_t time1, uint32_t period, uint8_t clockSpeed)
{
    return time0 > time1 ? (period - time0 + time1) / clockSpeed : (time1 - time0) / clockSpeed;
}

static int8_t decodePulse(uint32_t fallingTime, uint32_t risingTime)
{
    if (fallingTime < SHORTPULSE_HIGHBOUND && fallingTime > SHORTPULSE_LOWBOUND)
    {
        if (risingTime < LONGPULSE_HIGHBOUND && risingTime > LONGPULSE_LOWBOUND)
        {
            return 1;
        }
        else if (risingTime < SHORTPULSE_HIGHBOUND && risingTime > SHORTPULSE_LOWBOUND)
        {
            return 0;
        }
    }

    return -1;
}