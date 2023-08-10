#pragma once // для того чтобы не подключить библиотеку более 1 раза
#include <Arduino.h> // обязательно подключаем библиотеку из папки библиотек

/* в этом файле только объявляем методы класса, реализация этих методов в файле TM1637V2.cpp */
// объявляем класс
class TM1637
{
	public:
	TM1637(uint8_t CLK, uint8_t DIO); // объявление конструктора класса
	void startTransmission(); // функция начала посылки данных
	void writeByte(byte wrData); // функция посылки одного байта данных в устройство
	void endTransmission(); // функция окончания посылки данных
	void setBright(uint8_t Bright); // установка яркости дисплея
	void clearDisp(); // очистка дисплея
	void dispDigit(int8_t Segment, int8_t Digit, bool Point = 0); // отображение цифры в выбранном сегменте
	void dispNumber(int16_t Number); // отображение числа на дисплее
	void dispTime(int8_t Hour, int8_t Minute);  // отображение времени (часы и минуты)
	void dispDate(int8_t Date, int8_t Month);  // отображение даты (день и месяц)
	void dispTemp(float Temperature); // отображение температуры
	void hello(); // вывод надписи "HELO"
	void succes(int8_t Hour, int8_t Minute); // 5 - кратное моргание, означает успех
	
	private:
		uint8_t _CLK; // пин синхронизации CLK
		uint8_t _DIO; // пин передачи данных DIO
		uint8_t _Bright; // яркость дисплея
};		
