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
uint8_t LFSR[4] = { 0x8, 0x0, 0xA, 0xE };
uint8_t shift_n = 0x7;


uint8_t init_key[4] = {0, 0, 0, 0};     //�������� ����
uint8_t init_text[4] = { 0, 0, 0, 0};   //�������� �����
uint8_t key[4] = { 0, 0, 0, 0};         //������� ����
uint8_t text[4] = { 0, 0, 0, 0};        //���������
uint8_t counter_iter = 0;               //������� �������� 

//���������� ��� ������ ������ (���� ������, ����������) 0x00000001 - read text, 0x00000010 - read key, 0x00000100 - encrypt
uint8_t read_encr = 0;

//���������� ��� ������ ������ ������ ������ ��� ���������� 0x00000001 - show text, 0x00000010 - show key
uint8_t text_key = 0;

//������ ��� ��� {0. 1. 2. 3. 4. 5. 6. 7. 8.}, ��� ���������� �� ����� ����� ��������� ������������
uint8_t HG[10] = { 0b11111101, 0b11100000, 0b11010111, 0b11110110, 0b11101010, 0b10111110, 0b10111111, 0b11110000, 0b11111111 };


//������� ��������� 0 - HG0, 1 - HG1, 3 - LEDS
void indication(uint8_t data, uint8_t HG)
{
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
}

//������� ���������� P-box. i - ����� �����, text_or_key - 0 (text), 1 (key)
void make_p_box(uint8_t i, uint8_t text_or_key)
{
    uint8_t new_value = 0;
    if (text_or_key)
    {
        //new_value |= key[];
    }
}


void make_s_box()
{

}

void make_LFSR()
{

}

// ���������� ���������� ����� P1
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
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
            indication(HG[counter_iter] & 0b01111111, 1);    //������� �� ��� HG1
        }
        else
            indication(HG[counter_iter], 1);                //������� �� ��� HG1
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
            indication(HG[0] & 0b01111111, 0); //������� "0" �� HG0
        }
        else 
            indication(HG[counter_iter] & 0b01111111, 1);    //������� �� ��� HG1
        P2OUT = BTNS_EN;                                     //��������� ������ � ������� ������
    }
    //��� ������ ����������
    //�������� ��������� �������� ����������
    else if (P1IFG & NEXT_STEP)
    {
        text_key = 0b00000000;
    }
    //���������� � ������
    else if (P1IFG & ENCRYPT_AGAIN)
    {
        text_key = 0b00000000;
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

    }
    //��� ����� �������� ��� ���������� � ������ ����������
    
    P1IFG = 0b00000000;      // �������� ���� ����������
}



void init() 
{
    //���������� ��������� �������������
    WDTCTL = WDTPW | WDTHOLD;   //��������� ���������� ������, ��������� ����
    P2DIR |= 0b11101000;        //������������� P2 �� ����� ��� ���������� ���������
        
    //������� ������ � �����
    init_key[0] = 0;
    init_key[1] = 0;
    init_key[2] = 0;
    init_key[3] = 0;

    init_text[0] = 0;
    init_text[1] = 0;
    init_text[2] = 0;
    init_text[3] = 0;

    key[0] = 0;
    key[1] = 0;
    key[2] = 0;
    key[3] = 0;

    text[0] = 0;
    text[1] = 0;
    text[2] = 0;
    text[3] = 0;

    //������� �� ��������� �������� "0."
    indication(HG[counter_iter], 0);
    //������� �� ��������� ������ "0."
    indication(HG[counter_iter], 1);
    //��������� ����� ����� ������
    read_encr = 0b00000001;
 }

void read_text_and_key() 
{
    //�������������� ��������������� � ������ ������
    P1DIR &= 0b00000000;        //������������� P1 �� ���� ��� ������
    read_encr = 0b00000001;     //������������� ���� ������
    P1IES &= ~(WRITE_BYTE);     //������������� ���������� �� �������������� ������
    P1IFG &= ~WRITE_BYTE;       //�������� ���� ����������
    P1IE |= WRITE_BYTE;         //��������� ���������� �� ����� � ������� SB1
    __enable_interrupt();       //��������� ������ ����������
    P2OUT = BTNS_EN;            //�������� ������� ������    
}

void encrypt() 
{
    //�������������� ���� � ����� ��� ����������
    text[0] = init_text[0];
    text[1] = init_text[1];
    text[2] = init_text[2];
    text[3] = init_text[3];
    key[0] = init_key[0];
    key[1] = init_key[1];
    key[2] = init_key[2];
    key[3] = init_key[3];
    P1DIR &= 0b00000000;        //������������� P1 �� ���� ��� ������
    P1IES &= ~(0b11111111);     //������������� ���������� �� �������������� ������
    P1IFG &= ~0b11111111;       //�������� ���� ����������
    P1IE |= 0b11111111;         //��������� ���������� �� ����� � ������� SB1-SB8
    P2OUT = BTNS_EN;            //�������� ������� ������    
}

int main(void)
{
    init();                 //��������� �������������
    read_text_and_key();    //������������� ��� ������ ������ � �����
    //��� ��������� ������ ������ � �����
    while (read_encr & (READ_KEY | READ_TEXT)) {
    }
    encrypt();              //��������� ��� ���������� 
    while (1) {};
    return 0;
}
