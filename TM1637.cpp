#include "TM1637.h" // подключаю мою библиотеку из папки со скетчем
#include <Arduino.h> // подлючаю библиотеку стандартных функций из папки библиотек

/* В этом файле описываю реализацию методов моей библиотеки */
int DigitSegment[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; // массив соответсвия десятичной цифры её отображению на сегменте дисплея

unsigned long _workTime = 0; // в этой переменной храним время работы ардуины с момента включения
unsigned long _alarmStartTime = 0; // в этой переменной храним время включения будильника (системное)
uint16_t _alarmTime = 60000; // длительность будильника
bool Point = 0; // состояние разделительной точки

TM1637::TM1637(uint8_t CLK, uint8_t DIO) { // конструктор объектов класса TM1637
	pinMode(CLK, OUTPUT); // конфигурирование пинов микроконтроллера
	pinMode(DIO, OUTPUT);
	_CLK = CLK; // записись значения в приватную переменную класса
	_DIO = DIO;
}

void TM1637::startTransmission(){ // начало посылки данных
	digitalWrite(_CLK, HIGH);
	digitalWrite(_DIO, HIGH);
	digitalWrite(_DIO, LOW);
}
// функция заимствована из группы вк
/*
void TM1637::writeByte(byte wr_data)
{
  uint8_t i,count1;   
  for(i=0;i<8;i++)        //sent 8bit data
  {
    digitalWrite(_CLK,LOW);      
    if(wr_data & 0x01)digitalWrite(_DIO,HIGH);//LSB first
    else digitalWrite(_DIO,LOW);
	delayMicroseconds(3);
    wr_data >>= 1;      
    digitalWrite(_CLK,HIGH);
	delayMicroseconds(3);
      
  }  
  digitalWrite(_CLK,LOW); //wait for the ACK
  digitalWrite(_DIO,HIGH);
  digitalWrite(_CLK,HIGH);     
  pinMode(_DIO,INPUT);
  while(digitalRead(_DIO))    
  { 
    count1 +=1;
    if(count1 == 200)//
    {
     pinMode(_DIO,OUTPUT);
     digitalWrite(_DIO,LOW);
     count1 =0;
     break;
    }
    pinMode(_DIO,INPUT);
  }
  pinMode(_DIO,OUTPUT);
  
}*/

void TM1637::writeByte(byte wrData) { // посылка одного байта данных в устройство
	for (int i = 0; i < 8; i++) //  передача 8 бит
    {
      digitalWrite(_CLK, LOW);
      if (wrData & 1) {digitalWrite(_DIO, HIGH);}
      else {digitalWrite(_DIO, LOW);}
      wrData >>= 1;
      digitalWrite(_CLK, HIGH);
    }
    digitalWrite(_CLK, LOW); // ожидание АСК - сигнал устройства об успешном принятии 1 байта
    digitalWrite(_DIO, HIGH);
    digitalWrite(_CLK, HIGH);
    pinMode(_DIO, INPUT);

    delayMicroseconds(50);
    bool ACK = true;
    ACK = digitalRead(_DIO);
    if (ACK == false)
      {
        pinMode(_DIO, OUTPUT);
        digitalWrite(_DIO, LOW);
      }
   delayMicroseconds(50);
   pinMode(_DIO, OUTPUT);
   delayMicroseconds(50);   
}

void TM1637::endTransmission(){ // завершение посылки данных
	digitalWrite(_CLK, LOW);
	digitalWrite(_DIO, LOW);
	digitalWrite(_CLK, HIGH);
	digitalWrite(_DIO, HIGH);
}

void TM1637::setBright(uint8_t Bright){ // установка яркости дисплея
	_Bright = Bright; // записись значения в приватную переменную класса
	startTransmission();
	switch(Bright)
	{
		case 0:
			writeByte(B10001000);
		break;
    
		case 1:
			writeByte(B10001001);
		break;
		
		case 2:
			writeByte(B10001010);
		break;
		
		case 3:
			writeByte(B10001011);
		break;
		
		case 4:
			writeByte(B10001100);
		break;
		
		case 5:
			writeByte(B10001101);
		break;
		
		case 6:
			writeByte(B10001110);
		break;
		
		default:
			writeByte(B10001111);
		break;
	}
	endTransmission();
	 // отправляем байт настройки дисплея на работу с фиксированным адресом
	startTransmission();
	writeByte(B01000100); 
	endTransmission();
}

void TM1637::clearDisp(){ // функция очистки дисплея
	startTransmission();
	writeByte(0xC0);
	writeByte(0x00);
	endTransmission();
	startTransmission();
	writeByte(0xC1);
	writeByte(0x00);
	endTransmission();
	startTransmission();
	writeByte(0xC2);
	writeByte(0x00);
	endTransmission();
	startTransmission();
	writeByte(0xC3);
	writeByte(0x00);
	endTransmission();
}

void TM1637::dispDigit(int8_t Segment, int8_t Digit, bool Point = 0) { // отображение в выбранном сегменте одной цифры
	startTransmission();
	switch(Segment) { // выбор сегмента для записи
		case 0:
			writeByte(0xC0);
			break;
		case 1:
			writeByte(0xC1);
			break;
		case 2:
			writeByte(0xC2);
			break;
		default:
			writeByte(0xC3);
			break;
	}
  if (Point == 1) // зажигать точку или нет
	  writeByte(DigitSegment[Digit] xor B10000000); // выбор цифры из массива соответствия
  else 
    writeByte(DigitSegment[Digit]);
	endTransmission();
}

void TM1637::dispNumber(int16_t Number) { // отображение 4-х значного числа на дисплее
	dispDigit(0, (Number / 1000));
	dispDigit(1, ( Number / 100 ) % 10);
	dispDigit(2, ( Number / 10 ) % 10);
	dispDigit(3, (Number % 10));
}

void TM1637::dispTime(int8_t Hour, int8_t Minute) { // отображение времени (часы и минуты)
  dispDigit(0, (Hour / 10));
  if (millis() - _workTime > 1000) // инвертирование разделительной точки через 1 сек
  {
    Point = !Point;
    _workTime = millis();
  }
	dispDigit(1, (Hour % 10), Point);
	dispDigit(2, (Minute / 10));
	dispDigit(3, (Minute % 10));
}

void TM1637::dispDate(int8_t Date, int8_t Month) { // отображение даты (день и месяц)
	dispDigit(0, (Date / 10));
	dispDigit(1, (Date % 10), 1);
	dispDigit(2, (Month / 10));
	dispDigit(3, (Month % 10));
}	

void TM1637::dispTemp(float Temperature) { // отображение температуры
  int Temp = round(Temperature * 10); // округляем значение температуры, и приводим к типу int
  dispDigit(0, (Temp / 100) );
  dispDigit(1, (Temp / 10 % 10), 1);
  dispDigit(2, (Temp % 10));
  
  // в 3 сегменте отображаем символ градуса
  startTransmission();
  writeByte(0xC3);
  writeByte(B01100011);
  endTransmission();
}

void TM1637::hello() { // вывод надписи "HELO"
	startTransmission();
	writeByte(0xC0);
	writeByte(B01110110);
  endTransmission();
  startTransmission();
	writeByte(0xC1);
	writeByte(B01111001);
  endTransmission();
  startTransmission();
	writeByte(0xC2);
	writeByte(B00111000);
  endTransmission();
  startTransmission();
	writeByte(0xC3);
	writeByte(B00111111);
	endTransmission();
}

void TM1637::succes(int8_t Hour, int8_t Minute) { // 5 - кратное моргание, означает успех
  for (int i = 0; i < 5; i++) {
    dispTime(Hour, Minute);
    delay(100);
    clearDisp();
    delay(100);
  }
}
