#include <MSP430.h>
#include <stdint.h>

//���������� ��� ������ ����������
#define NEXT_STEP       BIT7
#define ENCRYPT_AGAIN   BIT6
#define SHOW_TEXT       BIT5
#define SHOW_KEY        BIT4
#define BYTE_3          BIT3
#define BYTE_2          BIT2
#define BYTE_1          BIT1
#define BYTE_0          BIT0

#define SHOW_TEXT_ACTIVE    BIT0
#define SHOW_KEY_ACTIVE     BIT1



#define DC_A0       BIT7
#define DC_A1       BIT6
#define DC_A2       BIT5
#define DC_EN       BIT3

#define WRITE_BYTE  BIT7

#define READ_TEXT   BIT0
#define READ_KEY    BIT1
#define ENCRYPT     BIT2

#define SEG0_EN     (DC_EN | DC_A2)
#define SEG1_EN     (DC_EN | DC_A2 | DC_A0)
#define LEDS_EN     (DC_EN | DC_A1 | DC_A0)
#define BTNS_EN     (DC_EN)

//�������� ������ ��� S-box, P-box, LFSR, shift_n
uint8_t P_box[8] = { 0x4, 0x0, 0x6, 0x1, 0x3, 0x5, 0x7, 0x2 };
uint8_t S_box_high[16] = { 0xF, 0xD, 0x0, 0x6, 0x9, 0x5, 0xA, 0x1, 0x2, 0xB, 0x3, 0x8, 0xC, 0x7, 0xE, 0x4 };
uint8_t S_box_low[16] = { 0x4, 0xF, 0x3, 0x6, 0xB, 0xC, 0x0, 0x9, 0x1, 0xA, 0x8, 0x5, 0xD, 0x7, 0xE, 0x2 };
uint16_t LFSR = 0b0100010100000001;
uint8_t shift_n = 0x7;


uint8_t init_key[4] = { 0, 0, 0, 0 };     //�������� ����
uint8_t init_text[4] = { 0, 0, 0, 0 };   //�������� �����
uint8_t key[4] = { 0, 0, 0, 0 };         //������� ����
uint8_t text[4] = { 0, 0, 0, 0 };        //���������
uint8_t counter_iter = 0;               //������� �������� 

//���������� ��� ������ ������ (���� ������, ����������) 0x00000001 - read text, 0x00000010 - read key, 0x00000100 - encrypt
uint8_t read_encr = 0;

//���������� ��� ������ ������ ������ ������ ��� ���������� 0x00000001 - show text, 0x00000010 - show key
uint8_t text_key = 0b00000001;

//������ ��� ��� {0. 1. 2. 3. 4. 5. 6. 7. 8.}, ��� ���������� �� ����� ����� ��������� ������������
uint8_t HG[10] = { 0b11111101, 0b11100000, 0b11010111, 0b11110110, 0b11101010, 0b10111110, 0b10111111, 0b11110000, 0b11111111 };

//������� ��������� 0 - HG0, 1 - HG1, 3 - LEDS
void indication(uint8_t data, uint8_t HG)
{
    P2OUT &= ~DC_EN;            //��������� ����������    
    P1DIR |= 0b11111111;        //������������� P1 �� ����� ��� ��������� �� ���
    P1OUT = data;               //������� �� ���� ������
    if (HG == 0)
    {
        P2OUT = SEG0_EN;            //��������� ������ � ������� ��� HG0
    }
    else if (HG == 1)
    {
        P2OUT = SEG1_EN;        //��������� ������ � ������� ��� HG1
    }
    else
        P2OUT = LEDS_EN;        //��������� ������ � ������� LEDS
    P2OUT &= ~DC_EN;            //��������� ����������    
    P1DIR = 0b00000000;         //������������� P1 �� ���� ������ � ������
    P2OUT = BTNS_EN;            //�������� ������� ������
}




void encrypt()
{
    //�������������� ���� � ����� ��� ����������
    counter_iter = 0;
    uint8_t i = 0;
    indication(0b00000000, 3);
    while (i < 4)
    {
        text[i] = init_text[i];
        key[i] = init_key[i];
        i++;
    }
    P1DIR &= 0b00000000;        //������������� P1 �� ���� ��� ������
    P1IFG &= ~0b11111111;       //�������� ���� ����������
    P1IES |= 0b01111111;        //������������� ���������� �� �������������� ������

    P1IE |= 0b01111111;         //��������� ���������� �� ����� � ������� SB1-SB8

}


//������� ���������� P-box. i - ����� �����, text_or_key - 0 (text), 1 (key)
void make_p_box(uint8_t* data)
{
    *data = (
        ((*data & 0b00000001) << P_box[0]) | (((*data & 0b00000010) >> 1) << P_box[1]) |
        (((*data & 0b00000100) >> 2) << P_box[2]) | (((*data & 0b00001000) >> 3) << P_box[3]) |
        (((*data & 0b00010000) >> 4) << P_box[4]) | (((*data & 0b00100000) >> 5) << P_box[5]) |
        (((*data & 0b01000000) >> 6) << P_box[6]) | (((*data & 0b10000000) >> 7) << P_box[7])
        );
}


void make_s_box(uint8_t* data)
{
    uint8_t low = *data & 0b00001111;   //�������� ������� ��������
    uint8_t high = *data >> 4;          //�������� ������� ��������
    low = S_box_low[low];               //��������� ����������� � �������� ���������
    high = S_box_high[high];             //��������� ����������� � �������� ���������
    *data = (high << 4) | low;          //���������� ��������� ���������� Sbox
}

void make_LFSR(uint8_t* high, uint8_t* low)
{
    uint16_t reg = (*high << 8) | *low;
    uint16_t sum = reg & LFSR;

    uint16_t new_bit_15 = 0;

    while (sum)
    {
        new_bit_15 ^= sum & 0b00000001;
        sum = sum >> 1;
    }
    reg = reg >> 1;
    *low = reg & 0b0000000011111111;
    *high = (reg >> 8) | (new_bit_15 << 7);
}

//������� ������������ ������ shift_t_k: 0 - text, 1 - key
void make_shift(uint8_t shift_t_k)
{
    uint8_t shift_arr[4] = { 0, 0, 0, 0 };
    uint8_t i = 0;
    while (i < 4)
    {
        if (shift_t_k)
        {
            shift_arr[i] = key[i];

        }
        else
            shift_arr[i] = text[i];
        i++;
    }

    i = 1;
    uint32_t shift_data = 0;
    shift_data |= shift_arr[3];
    while (i < 4)
    {
        shift_data = shift_data << 8;
        shift_data |= shift_arr[3 - i];
        i++;
    }
    // uint32_t shift_data = (shift_value[3] << 24) | (*shift_value[2] << 16) | (*shift_value[1] << 8) | *shift_value[0];
    i = shift_n;
    uint8_t new_bit_0 = 0;

    while (i--)
    {
        new_bit_0 = !!(shift_data & 0x80000000);
        shift_data = shift_data << 1;
        shift_data |= new_bit_0;
    }

    shift_arr[0] = shift_data & 0x000000FF;
    shift_arr[1] = (shift_data & 0x0000FF00) >> 8;
    shift_arr[2] = (shift_data & 0x00FF0000) >> 16;
    shift_arr[3] = (shift_data & 0xFF000000) >> 24;

    i = 0;
    while (i < 4)
    {
        if (shift_t_k)
        {
            key[i] = shift_arr[i];

        }
        else
            text[i] = shift_arr[i];
        i++;
    }
}

void make_xor()
{
    text[0] ^= key[0];
    text[1] ^= key[1];
    text[2] ^= key[2];
    text[3] ^= key[3];
}

void reset()
{
    //��������������� ����������
    indication(0b00000000, 3);
    indication(0b00000000, 1);
    indication(HG[0] & 0b01111111, 0);
    //������������������ ������
    uint8_t i = 0;
    while (i < 4)
    {
        key[i] = init_key[i];
        text[i] = init_text[i];
        i++;
    }
    //��������� �������
    counter_iter = 0;
    text_key = 0b00000001;

}

// ���������� ���������� ����� P1
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    //��������, ��� ������ �� ������ ��������� ����� ������ ��� �����, ���� �� ������ ������ �����

    //��� ��������� �����
    if ((P1IFG & WRITE_BYTE) && (read_encr & READ_TEXT))
    {
        init_text[counter_iter] = (P1IN ^ 0b11111111) & 0b01111111;    //��������� ��������� ����
        indication(init_text[counter_iter], 3);         //������� �� ����������
        counter_iter++;                                 //����������� �������


        //����� ������ ������ ��������� � ������ �����
        if (counter_iter == 4)
        {
            counter_iter = 0;          //�������� �������
            read_encr = 0b00000010;    //������������� ����� ������ �����
            indication(HG[counter_iter], 1);    //������� �� ��� HG1
        }
        else
            indication(HG[counter_iter] & 0b01111111, 1);                //������� �� ��� HG1
        P2OUT = BTNS_EN;                                //��������� ������ � ������� ������
    }
    else if ((P1IFG & WRITE_BYTE) && (read_encr & READ_KEY))
    {
        init_key[counter_iter] = (P1IN ^ 0b11111111) & 0b01111111;    //��������� ��������� ����
        indication(init_key[counter_iter], 3);          //������� �� ����������
        counter_iter++;                                 //����������� �������


        //����� ������ ����� ��������� � ����������
        if (counter_iter == 4)
        {
            counter_iter = 0;          //�������� �������
            read_encr = 0b00000100;    //������������� ����� ����������
            indication(0b00000000, 1); //��������� ��������� ��� HG1
            indication(HG[0], 0); //������� "0" �� HG0
            encrypt();              //��������� ��� ����������
        }
        else
            indication(HG[counter_iter], 1);    //������� �� ��� HG1
    }
    //��� ������ ����������
    //�������� ��������� �������� ����������
    else
    {
        if ((text_key & (SHOW_KEY_ACTIVE | SHOW_TEXT_ACTIVE)) && !(P1IFG & (BYTE_0 | BYTE_1 | BYTE_2 | BYTE_3)))
        {
            text_key = 0b00000001;
            indication(0b00000000, 1); //��������� ��������� ��� HG1
            indication(0b00000000, 3); //��������� ��������� LEDS
        }
        if (P1IFG & NEXT_STEP)
        {
            if (counter_iter == 8)
            {
                indication(HG[8], 0);
                P1IFG = 0b00000000;
                return;
            }
            else
            {
                counter_iter++;
                indication((HG[counter_iter] & 0b01111111), 0);
            }


            uint8_t i = 0;
            // P -> shift -> S (text)
            while (i < 4)
            {
                make_p_box(&text[i]);
                i++;
            }
            make_shift(0);
            i = 0;
            while (i < 4)
            {
                make_s_box(&text[i]);
                i++;
            }
            // shift -> P -> LFSR (key)
            make_shift(1);
            make_p_box(&key[0]);
            make_p_box(&key[3]);
            make_LFSR(&key[3], &key[1]);
            make_LFSR(&key[2], &key[0]);
            uint8_t temp = key[1];
            key[1] = key[2];
            key[2] = temp;
            //text xor key
            make_xor();
        }
        //���������� � ������
        else if (P1IFG & ENCRYPT_AGAIN)
        {
            reset();
        }
        //��������� ��������� ����������
        else if (P1IFG & SHOW_TEXT)
        {
            text_key = 0b00000001;

        }
        //��������� ��������� �����
        else if (P1IFG & SHOW_KEY)
        {
            text_key = 0b00000010;
        }
        //��������� ����� ������ ��� �����
        else if ((P1IFG & (BYTE_3 | BYTE_2 | BYTE_1 | BYTE_0)) && (text_key & (SHOW_TEXT_ACTIVE | SHOW_KEY_ACTIVE)))
        {
            uint8_t i_byte = 0;
            if (P1IFG & BYTE_0)
                i_byte = 0;
            else if (P1IFG & BYTE_1)
                i_byte = 1;
            else if (P1IFG & BYTE_2)
                i_byte = 2;
            else i_byte = 3;
            if (text_key & SHOW_KEY_ACTIVE)
            {
                indication(key[i_byte], 3);
                indication((HG[i_byte]), 1);
            }
            else if (text_key & SHOW_TEXT_ACTIVE)
            {
                indication(text[i_byte], 3);
                indication((HG[i_byte] & 0b01111111), 1);
            }
        }
    }



    P1IFG = 0b00000000;      // �������� ���� ����������
}



void init()
{
    //���������� ��������� �������������
    WDTCTL = WDTPW | WDTHOLD;   //��������� ���������� ������, ��������� ����
    P2DIR |= 0b11101000;        //������������� P2 �� ����� ��� ���������� ���������

    //������� ������ � �����
    uint8_t i = 0;
    while (i < 4)
    {
        init_key[i] = 0;
        init_text[0] = 0;
        key[i] = 0;
        text[i] = 0;
        i++;
    }

    //������� �� ��������� �������� "0."
    indication(HG[counter_iter], 0);
    //������� �� ��������� ������ "0"
    indication(HG[counter_iter] & 0b01111111, 1);
    //������� ����������
    indication(0b00000000, 3);
    //��������� ����� ����� ������
    read_encr = 0b00000001;
}

void read_text_and_key()
{
    //�������������� ��������������� � ������ ������
    P1DIR &= 0b00000000;        //������������� P1 �� ���� ��� ������
    read_encr = 0b00000001;     //������������� ���� ������
    P1IES |= WRITE_BYTE;     //������������� ���������� �� �������������� ������
    P1IFG &= ~WRITE_BYTE;       //�������� ���� ����������
    P1IE |= WRITE_BYTE;         //��������� ���������� �� ����� � ������� SB1
    __enable_interrupt();       //��������� ������ ���������� 
}



int main(void)
{
    init();                 //��������� �������������
    read_text_and_key();    //������������� ��� ������ ������ � �����
    //��� ��������� ������ ������ � �����
    while (read_encr & (READ_KEY | READ_TEXT)) {
    }
    // encrypt();              //��������� ��� ���������� 
    while (1) {};
    return 0;
}
