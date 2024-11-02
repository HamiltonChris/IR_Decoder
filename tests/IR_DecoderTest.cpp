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
    message.addressError = 0x11;
    message.addressInvError = 0x22;
    message.commandError = 0x33;
    message.commandInvError = 0x44;

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
    decoder.clearLast = 0xFF;

    IR_Decoder_Init(&decoder);
    BYTES_EQUAL(0, decoder.currentIndex);
    BYTES_EQUAL(0, decoder.pulseNumber);
    BYTES_EQUAL(CLOCK_SPEED_MHZ, decoder.clockSpeed);
    BYTES_EQUAL(0, decoder.clearLast);
    LONGLONGS_EQUAL(PERIOD, decoder.period);
    CHECK(decoder.state == LeadIn);
    CHECK(decoder.message == &message);
    BYTES_EQUAL(0, decoder.message->address);
    BYTES_EQUAL(0, decoder.message->addressInv);
    BYTES_EQUAL(0, decoder.message->command);
    BYTES_EQUAL(0, decoder.message->commandInv);
    BYTES_EQUAL(0, decoder.message->repeat);
    BYTES_EQUAL(0, decoder.message->addressError);
    BYTES_EQUAL(0, decoder.message->addressInvError);
    BYTES_EQUAL(0, decoder.message->commandError);
    BYTES_EQUAL(0, decoder.message->commandInvError);
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
    pDecoder->message->addressError = 0xFF;
    pDecoder->message->addressInvError = 0x11;
    pDecoder->message->commandError = 0x22;
    pDecoder->message->commandInvError = 0x33;

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
    BYTES_EQUAL(0, pDecoder->message->addressError);
    BYTES_EQUAL(0, pDecoder->message->addressInvError);
    BYTES_EQUAL(0, pDecoder->message->commandError);
    BYTES_EQUAL(0, pDecoder->message->commandInvError);
}

TEST(IR_Decoder, Decode_InvalidLeadIn)
{
    data[0] = 705052;
    data[1] = 2462448;
    data[2] = 3844152;
    IR_Decoder_Decode(pDecoder);
 
    BYTES_EQUAL(2, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    CHECK(pDecoder->state == LeadIn);
    LONGLONGS_EQUAL(0, pDecoder->buffer[0]);
    LONGLONGS_EQUAL(0, pDecoder->buffer[1]);
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

TEST(IR_Decoder, FullCommand)
{
    data[0] = 7584738;
    data[1] = 8344355;
    data[2] = 326320;
    data[3] = 376072;
    data[4] = 421711;
    data[5] = 468956;
    data[6] = 517313;
    data[7] = 567149;
    data[8] = 612841;
    data[9] = 660528;
    data[10] = 708440;
    data[11] = 758181;
    data[12] = 803909;
    data[13] = 853780;
    data[14] = 899524;
    data[15] = 949364;
    data[16] = 995015;
    data[17] = 1044881;
    data[18] = 1090559;
    data[19] = 1137822;
    data[20] = 1283950;
    data[21] = 1331698;
    data[22] = 1475124;
    data[23] = 1522802;
    data[24] = 1666220;
    data[25] = 1713890;
    data[26] = 1857192;
    data[27] = 1904889;
    data[28] = 2048264;
    data[29] = 2096006;
    data[30] = 2239465;
    data[31] = 2287126;
    data[32] = 2430557;
    data[33] = 2478259;
    data[34] = 2621684;
    data[35] = 2669393;
    data[36] = 2715072;
    data[37] = 2762247;
    data[38] = 2908427;
    data[39] = 2956076;
    data[40] = 3099483;
    data[41] = 3147226;
    data[42] = 3192856;
    data[43] = 3239977;
    data[44] = 3386079;
    data[45] = 3433757;
    data[46] = 3479429;
    data[47] = 3526532;
    data[48] = 3574913;
    data[49] = 3622075;
    data[50] = 3670341;
    data[51] = 3717544;
    data[52] = 3863565;
    data[53] = 3911263;
    data[54] = 3956969;
    data[55] = 4006688;
    data[56] = 4052515;
    data[57] = 4102233;
    data[58] = 4245695;
    data[59] = 4293461;
    data[60] = 4339088;
    data[61] = 4388890;
    data[62] = 4532287;
    data[63] = 4580019;
    data[64] = 4723359;
    data[65] = 4771131;
    data[66] = 4914414;
    data[67] = 4962171;
    data[68] = 8308051;
    data[69] = 662991;
    data[70] = 857815;
    data[71] = 902921;

    IR_Decoder_Decode(pDecoder);

    CHECK(pDecoder->state == LeadIn);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    BYTES_EQUAL(72, pDecoder->currentIndex);
    BYTES_EQUAL(0, pDecoder->message->address);
    BYTES_EQUAL(0xFF, pDecoder->message->addressInv);
    BYTES_EQUAL(0x16, pDecoder->message->command);
    BYTES_EQUAL(0xE9, pDecoder->message->commandInv);
    BYTES_EQUAL(1, pDecoder->message->repeat);
    BYTES_EQUAL(0x16 , decodedCommand);
    BYTES_EQUAL(1 , repeatCommand);
    for (int i = 0; i < 72; i++)
    {
        LONGLONGS_EQUAL(0, pDecoder->buffer[i]);
    }
}

TEST(IR_Decoder, BadAddressSignal)
{
    pDecoder->currentIndex = 50;
    pDecoder->state = Address;

    data[50] = 674816;
    data[51] = 714818;
    data[52] = 863968;
    data[53] = 906563;
    data[54] = 1045473;
    data[55] = 1095530;
    data[56] = 1153376;
    data[57] = 1323267;
    data[58] = 1344682;
    data[59] = 1382566;
    data[60] = 1526150;
    data[61] = 1576006;
    data[62] = 1717640;
    data[63] = 1767640;
    data[64] = 1811167;
    data[65] = 1856720;
    data[66] = 2000342;

    IR_Decoder_Decode(pDecoder);

    CHECK(pDecoder->state == AddressInv);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    BYTES_EQUAL(66, pDecoder->currentIndex);
    BYTES_EQUAL(0xA2, pDecoder->message->address);
    BYTES_EQUAL(0x1D, pDecoder->message->addressError);
}

TEST(IR_Decoder, BadAddressInvSignal)
{
    pDecoder->currentIndex = 34;
    pDecoder->state = AddressInv;

    data[34] = 8115750;
    data[35] = 8160433;
    data[36] = 8210439;
    data[37] = 8253779;
    data[38] = 8300448;
    data[39] = 8349346;
    data[40] = 98622;
    data[41] = 138657;
    data[42] = 194062;
    data[43] = 232001;
    data[44] = 290125;
    data[45] = 331811;
    data[46] = 385534;
    data[47] = 427877;
    data[48] = 576802;
    data[49] = 619403;
    data[50] = 674816;

    IR_Decoder_Decode(pDecoder);

    CHECK(pDecoder->state == Command);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    BYTES_EQUAL(50, pDecoder->currentIndex);
    BYTES_EQUAL(0x44, pDecoder->message->addressInv);
    BYTES_EQUAL(0xB8, pDecoder->message->addressInvError);
}

TEST(IR_Decoder, BadCommandSignal)
{
    pDecoder->currentIndex = 18;
    pDecoder->state = Command;

    data[18] = 6586529;
    data[19] = 6625051;
    data[20] = 6777920;
    data[21] = 6815620;
    data[22] = 6969129;
    data[23] = 7009342;
    data[24] = 7158375;
    data[25] = 7200671;
    data[26] = 7350002;
    data[27] = 7392114;
    data[28] = 7541521;
    data[29] = 7583499;
    data[30] = 7732701;
    data[31] = 7775244;
    data[32] = 7924400;
    data[33] = 7966736;
    data[34] = 8115750;

    IR_Decoder_Decode(pDecoder);

    CHECK(pDecoder->state == CommandInv);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    BYTES_EQUAL(34, pDecoder->currentIndex);
    BYTES_EQUAL(0xD8, pDecoder->message->command);
    BYTES_EQUAL(0x27, pDecoder->message->commandError);
}

TEST(IR_Decoder, BadCommandInvSignal)
{
    pDecoder->currentIndex = 18;
    pDecoder->state = CommandInv;

    data[18] = 4624934;
    data[19] = 4662240;
    data[20] = 4808805;
    data[21] = 4856615;
    data[22] = 5000517;
    data[23] = 5048330;
    data[24] = 5192198;
    data[25] = 5239983;
    data[26] = 5383859;
    data[27] = 5431637;
    data[28] = 5575589;
    data[29] = 5603424;
    data[30] = 5767302;
    data[31] = 5815121;
    data[32] = 5958981;
    data[33] = 6006806;
    data[34] = 6130709;
    data[35] = 6198580;

    IR_Decoder_Decode(pDecoder);

    CHECK(pDecoder->state == LeadIn);
    BYTES_EQUAL(0, pDecoder->pulseNumber);
    BYTES_EQUAL(36, pDecoder->currentIndex);
    BYTES_EQUAL(0x5E, pDecoder->message->commandInv);
    BYTES_EQUAL(0xA1, pDecoder->message->commandInvError);
}

TEST(IR_Decoder, BadRepeatSignal)
{
    pDecoder->currentIndex = 72;
    pDecoder->message->command = 0xFF;

    data[72] = 4234191;
    data[73] = 4995993;
    data[74] = 5158888;
    data[75] = 5236603;

    IR_Decoder_Decode(pDecoder);
    BYTES_EQUAL(0, decodedCommand);
    BYTES_EQUAL(0, repeatCommand);
    BYTES_EQUAL(0xFF, pDecoder->message->command);
    CHECK(pDecoder->state == LeadIn);
}

TEST(IR_Decoder, CurrentIndexZero)
{
    data[0] = 0;
    data[1] = 759617;
    data[2] = 1139426;

    IR_Decoder_Decode(pDecoder);
    CHECK(pDecoder->state == Address);
}

TEST(IR_Decoder, NextIndexZero)
{
    data[0] = 7640383;
    data[1] = 0;
    data[2] = 379808;

    IR_Decoder_Decode(pDecoder);
    CHECK(pDecoder->state == Address);
}

TEST(IR_Decoder, NextNextIndexZero)
{
    data[0] = 7260575;
    data[1] = 8020192;
    data[2] = 0;
    data[3] = 46200;

    IR_Decoder_Decode(pDecoder);
    CHECK(pDecoder->state == Address);
}

TEST(IR_Decoder, TwoZeroIndices)
{
    data[0] = 0;
    data[1] = 759617;
    data[2] = 0;
    data[3] = 1139426;

    IR_Decoder_Decode(pDecoder);
    CHECK(pDecoder->state == LeadIn);
    BYTES_EQUAL(0, pDecoder->currentIndex);
}

TEST(IR_Decoder, TwoZeroNextIndices)
{
    data[0] = 8020192;
    data[1] = 0;
    data[2] = 0;
    data[3] = 759617;

    IR_Decoder_Decode(pDecoder);
    CHECK(pDecoder->state == LeadIn);
    BYTES_EQUAL(0, pDecoder->currentIndex);
}

TEST(IR_Decoder, CurrentAndNextIndexZero)
{
    data[0] = 0;
    data[1] = 0;
    data[2] = 759617;
    data[3] = 1139426;

    IR_Decoder_Decode(pDecoder);
    CHECK(pDecoder->state == LeadIn);
    BYTES_EQUAL(0, pDecoder->currentIndex);
}

TEST(IR_Decoder, MissingFinalValue)
{
    pDecoder->state = CommandInv;
    pDecoder->currentIndex = 14;
    pDecoder->pulseNumber = 7;

    data[14] = 3967833;
    data[15] = 4015342;
    data[16] = 4158628;

    IR_Decoder_Decode(pDecoder);

    BYTES_EQUAL(18, pDecoder->currentIndex);

    data[17] = 4206150;
    data[18] = 759617;

    IR_Decoder_Decode(pDecoder);

    LONGLONGS_EQUAL(0, pDecoder->buffer[17]);
    LONGLONGS_EQUAL(759617, pDecoder->buffer[18]);
}

TEST(IR_Decoder, MissingFinalValueBoundary)
{
    pDecoder->state = CommandInv;
    pDecoder->currentIndex = BUFFER_SIZE - 4;
    pDecoder->pulseNumber = 7;

    data[BUFFER_SIZE - 4] = 3967833;
    data[BUFFER_SIZE - 3] = 4015342;
    data[BUFFER_SIZE - 2] = 4158628;

    IR_Decoder_Decode(pDecoder);

    data[BUFFER_SIZE - 1] = 4206150;
    data[0] = 759617;

    IR_Decoder_Decode(pDecoder);

    LONGLONGS_EQUAL(0, pDecoder->buffer[BUFFER_SIZE - 1]);
    LONGLONGS_EQUAL(759617, pDecoder->buffer[0]);
}

static void decodeFinished_callback(IR_Message_t *pMessage)
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