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
static IR_Message_t *pMessage;
static uint8_t decodedCommand;
static uint8_t repeatCommand;

static void decodeFinished_callback(IR_Message_t *pMessage);

TEST_GROUP(IR_Receiver)
{
    void setup()
    {
        decodedCommand = 0;
        repeatCommand = 0;
        pReceiver = (IR_Receiver_t*)malloc(sizeof(IR_Receiver_t));
        pMessage = (IR_Message_t*)malloc(sizeof(IR_Message_t));
        pReceiver->buffer = data;
        pReceiver->bufferSize = BUFFER_SIZE;
        pReceiver->currentIndex = 0xFF;
        pReceiver->clockSpeed = CLOCK_SPEED_MHZ;
        pReceiver->period = PERIOD;
        pReceiver->message = pMessage;
        pReceiver->decodeCallback = &decodeFinished_callback;
        IR_Receiver_Init(pReceiver);
    }

    void teardown()
    {
        memset(data, 0, sizeof(data));
        free(pReceiver);
        free(pMessage);
        pReceiver = NULL;
    }
};

TEST(IR_Receiver, Init)
{
    IR_Receiver_t receiver;
    IR_Message_t message;

    message.address = 0xFF;
    message.addressInv = 0xFA;
    message.command = 0x79;
    message.commandInv = 0xA5;
    message.repeat = 1;

    memset(data, 0xFFFFFFFF, sizeof(data));

    receiver.buffer = data;
    receiver.bufferSize = BUFFER_SIZE;
    receiver.state = Address;
    receiver.currentIndex = 0xFF;
    receiver.pulseNumber = 1;
    receiver.clockSpeed = CLOCK_SPEED_MHZ;
    receiver.period = PERIOD;
    receiver.message = &message;
    receiver.decodeCallback = &decodeFinished_callback;

    IR_Receiver_Init(&receiver);
    BYTES_EQUAL(0, receiver.currentIndex);
    BYTES_EQUAL(0, receiver.pulseNumber);
    BYTES_EQUAL(CLOCK_SPEED_MHZ, receiver.clockSpeed);
    LONGLONGS_EQUAL(PERIOD, receiver.period);
    CHECK(receiver.state == LeadIn);
    CHECK(receiver.message == &message);
    CHECK(receiver.message->address == 0);
    CHECK(receiver.message->addressInv == 0);
    CHECK(receiver.message->command == 0);
    CHECK(receiver.message->commandInv == 0);
    CHECK(receiver.message->repeat == 0);
    CHECK(receiver.decodeCallback);
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
    data[0] = 3000;
    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(0, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    CHECK(pReceiver->state == LeadIn);
}

TEST(IR_Receiver, Decode_TwoValues)
{
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
    pReceiver->message->address = 0xAA;
    pReceiver->message->addressInv = 0xBB;
    pReceiver->message->command = 0xCC;
    pReceiver->message->commandInv = 0xDD;
    pReceiver->message->repeat = 0xEE;

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

    BYTES_EQUAL(0, pReceiver->message->address);
    BYTES_EQUAL(0, pReceiver->message->addressInv);
    BYTES_EQUAL(0, pReceiver->message->command);
    BYTES_EQUAL(0, pReceiver->message->commandInv);
    BYTES_EQUAL(0, pReceiver->message->repeat);
}

TEST(IR_Receiver, Decode_InvalidLeadIn)
{
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
    BYTES_EQUAL(0, pReceiver->message->address);
    CHECK(pReceiver->state == AddressInv);
}

TEST(IR_Receiver, Decode_AddressNonZero)
{
    pReceiver->state = Address;

    data[0] = 5266180;
    data[1] = 5313068;
    data[2] = 5362006;
    data[3] = 5408851;
    data[4] = 5555459;
    data[5] = 5600546;
    data[6] = 5649451;
    data[7] = 5696317;
    data[8] = 5745256;
    data[9] = 5792113;
    data[10] = 5938718;
    data[11] = 5983784;
    data[12] = 6130336;
    data[13] = 6175405;
    data[14] = 6322059;
    data[15] = 6367026;
    data[16] = 6513726;

    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(16, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    BYTES_EQUAL(0xF2, pReceiver->message->address);
    CHECK(pReceiver->state == AddressInv);

    for (int i = 0; i < 16; i++)
    {
        LONGLONGS_EQUAL(0, pReceiver->buffer[i]);
    }
}

TEST(IR_Receiver, Decode_InvAddress)
{
    pReceiver->currentIndex = 19;
    pReceiver->state = AddressInv;

    data[19] = 4213821;
    data[20] = 4258903;
    data[21] = 4405568;
    data[22] = 4450663;
    data[23] = 4499576;
    data[24] = 4546507;
    data[25] = 4693069;
    data[26] = 4738127;
    data[27] = 4884711;
    data[28] = 4929856;
    data[29] = 4978728;
    data[30] = 5025635;
    data[31] = 5074582;
    data[32] = 5121466;
    data[33] = 5170432;
    data[34] = 5217320;
    data[35] = 5266180;
    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(35, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    BYTES_EQUAL(0x0D, pReceiver->message->addressInv);
    CHECK(pReceiver->state == Command);
}

TEST(IR_Receiver, Decode_Command)
{
    pReceiver->state = Command;

    data[0] = 4213962;
    data[1] = 4260887;
    data[2] = 4407445;
    data[3] = 4452452;
    data[4] = 4599009;
    data[5] = 4644019;
    data[6] = 4790661;
    data[7] = 4835691;
    data[8] = 4884585;
    data[9] = 4931495;
    data[10] = 4980354;
    data[11] = 5027270;
    data[12] = 5173869;
    data[13] = 5218897;
    data[14] = 5365426;
    data[15] = 5410505;
    data[16] = 5557102;

    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(16, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    BYTES_EQUAL(0xE7, pReceiver->message->command);
    CHECK(pReceiver->state == CommandInv);
}

TEST(IR_Receiver, Decode_CommandInv)
{
    pReceiver->state = CommandInv;
    pReceiver->message->command = 0x16;

    data[0] = 2916332;
    data[1] = 2965982;
    data[2] = 3109336;
    data[3] = 3156946;
    data[4] = 3202733;
    data[5] = 3252275;
    data[6] = 3297932;
    data[7] = 3347536;
    data[8] = 3490895;
    data[9] = 3538350;
    data[10] = 3584017;
    data[11] = 3633728;
    data[12] = 3776956;
    data[13] = 3824498;
    data[14] = 3967833;
    data[15] = 4015342;
    data[16] = 4158628;
    data[17] = 4206150;

    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(18, pReceiver->currentIndex);
    BYTES_EQUAL(0, pReceiver->pulseNumber);
    CHECK(pReceiver->state == LeadIn);
    BYTES_EQUAL(0xE9, pReceiver->message->commandInv);
    BYTES_EQUAL(0x16 , decodedCommand);
}

TEST(IR_Receiver, RepeatCommand)
{
    pMessage->command = 0x9E;
    data[0] = 7309478;
    data[1] = 8065082;
    data[2] = 8260597;
    data[3] = 8305454;
    IR_Receiver_Decode(pReceiver);

    BYTES_EQUAL(0x9E, decodedCommand);
    BYTES_EQUAL(1, repeatCommand);
    BYTES_EQUAL(4, pReceiver->currentIndex);
    CHECK(pReceiver->state == LeadIn);
    for (int i = 0; i < 4; i++)
    {
        LONGLONGS_EQUAL(0, pReceiver->buffer[i]);
    }
}

IGNORE_TEST(IR_Receiver, CircularBuffer)
{

}

IGNORE_TEST(IR_Receiver, FullCommand)
{

}

IGNORE_TEST(IR_Receiver, BadAddressSignal)
{

}

IGNORE_TEST(IR_Receiver, BadAddressInvSignal)
{
    
}

IGNORE_TEST(IR_Receiver, BadCommandSignal)
{
    
}

IGNORE_TEST(IR_Receiver, BadCommandInvSignal)
{
    
}

IGNORE_TEST(IR_Receiver, BadRepeatSignal)
{
    
}

static void decodeFinished_callback (IR_Message_t *pMessage)
{
    if (pMessage)
    {
        decodedCommand = pMessage->command;

        if (pMessage->repeat)
        {
            repeatCommand = pMessage->repeat;
        }
    }
}