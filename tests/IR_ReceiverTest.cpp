extern "C"
{
#include "IR_Receiver.h"

#include <string.h>
}

#include "CppUTest/TestHarness.h"

#define BUFFER_SIZE     136
#define CLOCK_SPEED_MHZ 84
#define PERIOD          8400000


static uint32_t data[BUFFER_SIZE];
static IR_Receiver_t *pReceiver;

TEST_GROUP(IR_Receiver)
{
    void setup()
    {
        pReceiver = (IR_Receiver_t*)malloc(sizeof(IR_Receiver_t));
        pReceiver->buffer = data;
        pReceiver->bufferSize = BUFFER_SIZE;
        pReceiver->currentIndex = 0xFF;
        pReceiver->clockSpeed = CLOCK_SPEED_MHZ;
        pReceiver->period = PERIOD;
        IR_Receiver_Init(pReceiver);
    }

    void teardown()
    {
        memset(data, 0, sizeof(data));
        free(pReceiver);
        pReceiver = NULL;
    }
};

TEST(IR_Receiver, Init)
{
    IR_Receiver_t receiver;

    memset(data, 0xFFFFFFFF, sizeof(data));

    receiver.buffer = data;
    receiver.bufferSize = BUFFER_SIZE;
    receiver.state = Address;
    receiver.currentIndex = 0xFF;
    receiver.pulseNumber = 1;
    receiver.clockSpeed = CLOCK_SPEED_MHZ;
    receiver.period = PERIOD;

    IR_Receiver_Init(&receiver);
    BYTES_EQUAL(0, receiver.currentIndex);
    BYTES_EQUAL(0, receiver.pulseNumber);
    BYTES_EQUAL(CLOCK_SPEED_MHZ, receiver.clockSpeed);
    LONGLONGS_EQUAL(PERIOD, receiver.period);
    CHECK(receiver.state == LeadIn);
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        LONGLONGS_EQUAL(0, receiver.buffer[i]);
    }
}

TEST(IR_Receiver, Decode_Empty)
{
    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(0, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    CHECK(pReceiver->state == LeadIn);
}

TEST(IR_Receiver, Decode_OneValue)
{
    IR_Receiver_Init(pReceiver);
    data[0] = 3000;
    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(0, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    CHECK(pReceiver->state == LeadIn);
}

TEST(IR_Receiver, Decode_TwoValues)
{
    IR_Receiver_Init(pReceiver);
    data[0] = 3000;
    data[1] = 3500;
    IR_Receiver_Decode(pReceiver);
 
    BYTES_EQUAL(0, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    CHECK(pReceiver->state == LeadIn);
    LONGLONGS_EQUAL(3000, pReceiver->buffer[0]);
    LONGLONGS_EQUAL(3500, pReceiver->buffer[1]);
}

TEST(IR_Receiver, Decode_ValidLeadIn)
{
    IR_Receiver_Init(pReceiver);
    data[0] = 1705052;
    data[1] = 2462448;
    data[2] = 2844152;
    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(2, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    CHECK(pReceiver->state == Address);
    LONGLONGS_EQUAL(0, pReceiver->buffer[0]);
    LONGLONGS_EQUAL(0, pReceiver->buffer[1]);
    LONGLONGS_EQUAL(2844152, pReceiver->buffer[2]);
}

TEST(IR_Receiver, Decode_InvalidLeadIn)
{
    IR_Receiver_Init(pReceiver);
    data[0] = 705052;
    data[1] = 2462448;
    data[2] = 3844152;
    IR_Receiver_Decode(pReceiver);
 
    BYTES_EQUAL(1, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    CHECK(pReceiver->state == LeadIn);
    LONGLONGS_EQUAL(0, pReceiver->buffer[0]);
    LONGLONGS_EQUAL(2462448, pReceiver->buffer[1]);
}


TEST(IR_Receiver, Decode_Address)
{
    IR_Receiver_Init(pReceiver);
    data[0] = 8240243;
    data[1] = 601040;
    data[2] = 983884;
    data[3] = 1033748;
    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(2, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    LONGLONGS_EQUAL(0, pReceiver->buffer[0]);
    LONGLONGS_EQUAL(0, pReceiver->buffer[1]);
    LONGLONGS_EQUAL(983884, pReceiver->buffer[2]);
    CHECK(pReceiver->state == Address);

    data[4] = 1079765;

    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(4, pReceiver->currentIndex);
    BYTES_EQUAL(1, pReceiver->pulseNumber);
    CHECK(pReceiver->state == Address);

    data[5] = 1129501;
    data[6] = 1175385;
    data[7] = 1223303;
    data[8] = 1271170;
    data[9] = 1318990;
    data[10] = 1367045;
    data[11] = 1414668;
    data[12] = 1462614;
    data[13] = 1510254;
    data[14] = 1558142;
    data[15] = 1607996;
    data[16] = 1653958;
    data[17] = 1701731;
    data[18] = 1749561;
    data[19] = 1797381;

    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(18, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    CHECK(pReceiver->state == AddressInv);
}