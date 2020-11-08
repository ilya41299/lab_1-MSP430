#include <MSP430.h>
#include <stdint.h>

//объявление для режима шифрования
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


uint8_t init_key[4] = {0, 0, 0, 0};     //исходный ключ
uint8_t init_text[4] = { 0, 0, 0, 0};   //исходный текст
uint8_t key[4] = { 0, 0, 0, 0};         //текущий ключ
uint8_t text[4] = { 0, 0, 0, 0};        //шифртекст
uint8_t counter_iter = 0;               //счетчик итераций 

//переменная для выбора режима (ввод данных, шифрование) 0x00000001 - read text, 0x00000010 - read key, 0x00000100 - encrypt
uint8_t read_encr = 0;

//переменная для выбора режима чтения байтов при шифровании 0x00000001 - show text, 0x00000010 - show key
uint8_t text_key = 0;

//массив для ССИ {0. 1. 2. 3. 4. 5. 6. 7. 8.}, для исбавления от точки будем применять маскирование
uint8_t HG[10] = { 0b11111101, 0b11100000, 0b11010111, 0b11110110, 0b11101010, 0b10111110, 0b10111111, 0b11110000, 0b11111111 };


//функция индикации 0 - HG0, 1 - HG1, 3 - LEDS
void indication(uint8_t data, uint8_t HG)
{
    P1DIR |= 0b11111111;        //конфигурируем P1 на вывод для индикации на ССИ
    P1OUT = data;               //выводим на него данные
    if (HG == 0)
    {
        P2OUT = SEG0_EN;            //разрешаем запись в регистр ССИ HG0
    }
    else if (HG == 1)
    {
        P2OUT = SEG1_EN;        //разрешаем запись в регистр ССИ HG1
    }
    else
        P2OUT = LEDS_EN;        //разрешаем запись в регистр LEDS
    P2OUT &= ~DC_EN;            //отключаем дешифратор    
    P1DIR = 0b00000000;         //конфигурируем P1 на ввод данных с кнопок
}


// обработчик прерываний порта P1
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    //при появлении флага
    if ((P1IFG & WRITE_BYTE) && (read_encr & READ_TEXT))
    {
        init_text[counter_iter] = (P1IN ^ 0b11111111) & 0b01111111;    //сохраняем считанный байт
        indication(init_text[counter_iter], 3);         //выводим на светодиоды
        counter_iter++;                                 //увеличиваем счетчик


        //после чтения данных переходим к чтению ключа
        if (counter_iter == 4)
        {
            counter_iter = 0;          //обнуляем счетчик
            read_encr = 0b00000010;    //устанавливаем режим чтения ключа
            indication(HG[counter_iter] & 0b01111111, 1);    //выводим на ССИ HG1
        }
        else
            indication(HG[counter_iter], 1);                //выводим на ССИ HG1
        P2OUT = BTNS_EN;                                //разрешаем запись в регистр кнопок
    }
    else if ((P1IFG & WRITE_BYTE) && (read_encr & READ_KEY))
    {
        init_key[counter_iter] = (P1IN ^ 0b11111111) & 0b01111111;    //сохраняем считанный байт
        indication(init_key[counter_iter], 3);          //выводим на светодиоды
        counter_iter++;                                 //увеличиваем счетчик
        
       
        //после чтения ключа переходим к шифрованию
        if (counter_iter == 4)
        {
            counter_iter = 0;          //обнуляем счетчик
            read_encr = 0b00000100;    //устанавливаем режим шифрования
            indication(0b00000000, 1); //отключаем индикацию ССИ HG1
            indication(HG[0] & 0b01111111, 0); //выводим "0" на HG0
        }
        else 
            indication(HG[counter_iter] & 0b01111111, 1);    //выводим на ССИ HG1
        P2OUT = BTNS_EN;                                     //разрешаем запись в регистр кнопок
    }
    //для режима шифрования
    //выполить следующую итерацию шифрования
    else if (P1IFG & NEXT_STEP)
    {
        text_key = 0b00000000;
    }
    //шифрование с начала
    else if (P1IFG & ENCRYPT_AGAIN)
    {
        text_key = 0b00000000;
    }
    //активация индикации шифртекста
    else if (P1IFG & SHOW_TEXT)
    {
        text_key = 0b00000001;
    }
    //активация индикации ключа
    else if (P1IFG & SHOW_KEY)
    {
        text_key = 0b00000010;
    }
    //индикация байта данных или ключа
    else if ((P1IFG & (BYTE_3 | BYTE_2 | BYTE_1 | BYTE_0)) && (text_key & (SHOW_TEXT_ACTIVE | SHOW_KEY_ACTIVE)))
    {

    }
    //тут БУДЕТ ОПИСАНИЕ ДЛЯ ПРЕРЫВАНИЙ В РЕЖИМЕ ШИФРОВАНИЯ
    
    P1IFG = 0b00000000;      // опускаем флаг прерывания
}

void init() 
{
    //производим начальную инициализацию
    WDTCTL = WDTPW | WDTHOLD;   //отключаем сторожевой таймер, применяем ключ
    P2DIR |= 0b11101000;        //конфигурируем P2 на вывод для управления защелками
        
    //очищаем тексты и ключи
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

    //выводим на индикатор счетчика "0."
    indication(HG[counter_iter], 0);
    //выводим на индикатор данных "0."
    indication(HG[counter_iter], 1);
    //переходим режим ввода данных
    read_encr = 0b00000001;
 }

void read_text_and_key() 
{
    //подготавливаем микроконтроллер к чтению данных
    P1DIR &= 0b00000000;        //конфигурируем P1 на ввод для чтения
    read_encr = 0b00000001;     //устанавливаем флаг чтения
    P1IES &= ~(WRITE_BYTE);     //устанавливаем прерывание по положительному фронту
    P1IFG &= ~WRITE_BYTE;       //опускаем флаг прерывания
    P1IE |= WRITE_BYTE;         //разрешаем прерывание на ножке с кнопкой SB1
    __enable_interrupt();       //разрешаем работу прерываний
    P2OUT = BTNS_EN;            //включаем регистр кнопок    
}

void encrypt() 
{
    //инициализируем ключ и текст для шифрования
    text[0] = init_text[0];
    text[1] = init_text[1];
    text[2] = init_text[2];
    text[3] = init_text[3];
    key[0] = init_key[0];
    key[1] = init_key[1];
    key[2] = init_key[2];
    key[3] = init_key[3];
    P1DIR &= 0b00000000;        //конфигурируем P1 на ввод для чтения
    P1IES &= ~(0b11111111);     //устанавливаем прерывание по положительному фронту
    P1IFG &= ~0b11111111;       //опускаем флаг прерывания
    P1IE |= 0b11111111;         //разрешаем прерывание на ножке с кнопкой SB1-SB8
    P2OUT = BTNS_EN;            //включаем регистр кнопок    
}

int main(void)
{
    init();                 //первичная инициализация
    read_text_and_key();    //инициализация для чтения данных и ключа
    //ждём окончания чтения данных и ключа
    while (read_encr & (READ_KEY | READ_TEXT)) {
    }
    encrypt();              //настройка для шифрования 
    while (1) {};
    return 0;
}
