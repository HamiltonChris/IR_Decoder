extern "C"
{
#include "IR_Decoder.h"

#include <string.h>
}

#include "CppUTest/TestHarness.h"

#define BUFFER_SIZE     136
#define CLOCK_SPEED_MHZ 84
#define PERIOD          8400000


static uint32_t data[BUFFER_SIZE];
static IR_Decoder_t *pDecoder;
static IR_Message_t *pMessage;
static uint8_t decodedCommand;
static uint8_t repeatCommand;

static void decodeFinished_callback(IR_Message_t *pMessage);

TEST_GROUP(IR_Decoder)
{
    void setup()
    {
        decodedCommand = 0;
        repeatCommand = 0;
        pDecoder = (IR_Decoder_t*)malloc(sizeof(IR_Decoder_t));
        pMessage = (IR_Message_t*)malloc(sizeof(IR_Message_t));
        pDecoder->buffer = data;
        pDecoder->bufferSize = BUFFER_SIZE;
        pDecoder->currentIndex = 0xFF;
        pDecoder->clockSpeed = CLOCK_SPEED_MHZ;
        pDecoder->period = PERIOD;
        pDecoder->message = pMessage;
        pDecoder->decodeCallback = &decodeFinished_callback;
        IR_Decoder_Init(pDecoder);
    }

    void teardown()
    {
        memset(data, 0, sizeof(data));
        free(pDecoder);
        free(pMessage);
        pDecoder = NULL;
    }
};

TEST(IR_Decoder, Init)
{
    IR_Decoder_t decoder;
    IR_Message_t message;

    message.address = 0xFF;
    message.addressInv = 0xFA;
    message.command = 0x79;
    message.commandInv = 0xA5;
    message.repeat = 1;

    memset(data, 0xFFFFFFFF, sizeof(data));

    decoder.buffer = data;
    decoder.bufferSize = BUFFER_SIZE;
    decoder.state = Address;
    decoder.currentIndex = 0xFF;
    decoder.pulseNumber = 1;
    decoder.clockSpeed = CLOCK_SPEED_MHZ;
    decoder.period = PERIOD;
    decoder.message = &message;
    decoder.decodeCallback = &decodeFinished_callback;

    IR_Decoder_Init(&decoder);
    BYTES_EQUAL(0, decoder.currentIndex);
    BYTES_EQUAL(0, decoder.pulseNumber);
    BYTES_EQUAL(CLOCK_SPEED_MHZ, decoder.clockSpeed);
    LONGLONGS_EQUAL(PERIOD, decoder.period);
    CHECK(decoder.state == LeadIn);
    CHECK(decoder.message == &message);
    CHECK(decoder.message->address == 0);
    CHECK(decoder.message->addressInv == 0);
    CHECK(decoder.message->command == 0);
    CHECK(decoder.message->commandInv == 0);
    CHECK(decoder.message->repeat == 0);
    CHECK(decoder.decodeCallback);
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        LONGLONGS_EQUAL(0, decoder.buffer[i]);
    }
}

TEST(IR_Decoder, Decode_Empty)
{
    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(0, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    CHECK(pDecoder->state == LeadIn);
}

TEST(IR_Decoder, Decode_OneValue)
{
    data[0] = 3000;
    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(0, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    CHECK(pDecoder->state == LeadIn);
}

TEST(IR_Decoder, Decode_TwoValues)
{
    data[0] = 3000;
    data[1] = 3500;
    IR_Decoder_Decode(pDecoder);
 
    BYTES_EQUAL(0, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    CHECK(pDecoder->state == LeadIn);
    LONGLONGS_EQUAL(3000, pDecoder->buffer[0]);
    LONGLONGS_EQUAL(3500, pDecoder->buffer[1]);
}

TEST(IR_Decoder, Decode_ValidLeadIn)
{
    pDecoder->message->address = 0xAA;
    pDecoder->message->addressInv = 0xBB;
    pDecoder->message->command = 0xCC;
    pDecoder->message->commandInv = 0xDD;
    pDecoder->message->repeat = 0xEE;

    data[0] = 1705052;
    data[1] = 2462448;
    data[2] = 2844152;
    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(2, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    CHECK(pDecoder->state == Address);
    LONGLONGS_EQUAL(0, pDecoder->buffer[0]);
    LONGLONGS_EQUAL(0, pDecoder->buffer[1]);
    LONGLONGS_EQUAL(2844152, pDecoder->buffer[2]);

    BYTES_EQUAL(0, pDecoder->message->address);
    BYTES_EQUAL(0, pDecoder->message->addressInv);
    BYTES_EQUAL(0, pDecoder->message->command);
    BYTES_EQUAL(0, pDecoder->message->commandInv);
    BYTES_EQUAL(0, pDecoder->message->repeat);
}

TEST(IR_Decoder, Decode_InvalidLeadIn)
{
    data[0] = 705052;
    data[1] = 2462448;
    data[2] = 3844152;
    IR_Decoder_Decode(pDecoder);
 
    BYTES_EQUAL(1, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    CHECK(pDecoder->state == LeadIn);
    LONGLONGS_EQUAL(0, pDecoder->buffer[0]);
    LONGLONGS_EQUAL(2462448, pDecoder->buffer[1]);
}


TEST(IR_Decoder, Decode_Address)
{
    data[0] = 8240243;
    data[1] = 601040;
    data[2] = 983884;
    data[3] = 1033748;
    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(2, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    LONGLONGS_EQUAL(0, pDecoder->buffer[0]);
    LONGLONGS_EQUAL(0, pDecoder->buffer[1]);
    LONGLONGS_EQUAL(983884, pDecoder->buffer[2]);
    CHECK(pDecoder->state == Address);

    data[4] = 1079765;

    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(4, pDecoder->currentIndex);
    BYTES_EQUAL(1, pDecoder->pulseNumber);
    CHECK(pDecoder->state == Address);

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

    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(18, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    BYTES_EQUAL(0, pDecoder->message->address);
    CHECK(pDecoder->state == AddressInv);
}

TEST(IR_Decoder, Decode_AddressNonZero)
{
    pDecoder->state = Address;

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

    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(16, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    BYTES_EQUAL(0xF2, pDecoder->message->address);
    CHECK(pDecoder->state == AddressInv);

    for (int i = 0; i < 16; i++)
    {
        LONGLONGS_EQUAL(0, pDecoder->buffer[i]);
    }
}

TEST(IR_Decoder, Decode_InvAddress)
{
    pDecoder->currentIndex = 19;
    pDecoder->state = AddressInv;

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
    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(35, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    BYTES_EQUAL(0x0D, pDecoder->message->addressInv);
    CHECK(pDecoder->state == Command);
}

TEST(IR_Decoder, Decode_Command)
{
    pDecoder->state = Command;

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

    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(16, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    BYTES_EQUAL(0xE7, pDecoder->message->command);
    CHECK(pDecoder->state == CommandInv);
}

TEST(IR_Decoder, Decode_CommandInv)
{
    pDecoder->state = CommandInv;
    pDecoder->message->command = 0x16;

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

    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(18, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    CHECK(pDecoder->state == LeadIn);
    BYTES_EQUAL(0xE9, pDecoder->message->commandInv);
    BYTES_EQUAL(0x16 , decodedCommand);
}

TEST(IR_Decoder, RepeatCommand)
{
    pMessage->command = 0x9E;
    data[0] = 7309478;
    data[1] = 8065082;
    data[2] = 8260597;
    data[3] = 8305454;
    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(0x9E, decodedCommand);
    BYTES_EQUAL(1, repeatCommand);
    BYTES_EQUAL(4, pDecoder->currentIndex);
    CHECK(pDecoder->state == LeadIn);
    for (int i = 0; i < 4; i++)
    {
        LONGLONGS_EQUAL(0, pDecoder->buffer[i]);
    }
}

TEST(IR_Decoder, BufferOverrunLeadin)
{
    pDecoder->currentIndex = BUFFER_SIZE - 1;

    data[BUFFER_SIZE - 1] = 5936509;
    data[0] = 6698756;
    data[1] = 7078454;

    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(1, pDecoder->currentIndex);
    CHECK(pDecoder->state == Address);
    LONGLONGS_EQUAL(0, pDecoder->buffer[BUFFER_SIZE - 1]);
    LONGLONGS_EQUAL(0, pDecoder->buffer[0]);
}

TEST(IR_Decoder, BufferOverrunRepeat)
{
    pDecoder->currentIndex = BUFFER_SIZE - 2;
    pMessage->command = 0x81;
    data[BUFFER_SIZE - 2] = 659007;
    data[BUFFER_SIZE - 1] = 1417851;
    data[0] = 1612417;
    data[1] = 1658377;

    IR_Decoder_Decode(pDecoder);
    
    BYTES_EQUAL(2, pDecoder->currentIndex);
    BYTES_EQUAL(0x81, decodedCommand);
}

TEST(IR_Decoder, BufferOverrunAddress)
{
    pDecoder->currentIndex = BUFFER_SIZE - 4;

    data[BUFFER_SIZE - 4] = 2534622;
    data[BUFFER_SIZE - 3] = 3295670;
    data[BUFFER_SIZE - 2] = 3677807;
    data[BUFFER_SIZE - 1] = 3725829;
    data[0] = 3773840;
    data[1] = 3821574;
    data[2] = 3871233;
    data[3] = 3917219;
    data[4] = 3967004;
    data[5] = 4012900;
    data[6] = 4062604;

    IR_Decoder_Decode(pDecoder);

    CHECK(pDecoder->state == Address);
    BYTES_EQUAL(6, pDecoder->currentIndex);
}

IGNORE_TEST(IR_Decoder, FullCommand)
{

}

IGNORE_TEST(IR_Decoder, BadAddressSignal)
{

}

IGNORE_TEST(IR_Decoder, BadAddressInvSignal)
{
    
}

IGNORE_TEST(IR_Decoder, BadCommandSignal)
{
    
}

IGNORE_TEST(IR_Decoder, BadCommandInvSignal)
{
    
}

IGNORE_TEST(IR_Decoder, BadRepeatSignal)
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