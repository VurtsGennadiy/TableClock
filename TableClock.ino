/*  
    Проект часов c термометром и функцией будильника на модуле DS1307, датчик температуры DS18B20, дисплейный модуль TM1637 на 4-х 7-ми сегментных индикаторах.
    Для установки времени и даты открыть монитор порта, установить "нет конца строки", на часах зажать и удерживать кнопку пока не заморгает дисплей, далее по инструкции.
    По умолчанию будильник отключен, для включения установить константу ENABLE_ALARM = HIGH. Время включения будильника можно установить отдельно на будний и выходной дни константами
    hourAlarmWeekday, minuteAlarmWeekday и hourAlarmWeekend, minuteAlarmWeekend соответственно.
    Яркость дисплея автоматически уменьшается в ночное время, час перехода задаётся константой hourNight и час возвращения hourDay.
    Автор: Вурц Геннадий, 2022 год.
*/

#include <Wire.h> // стандартная библиотека для связи с модулем времени по I2C
#include <OneWire.h> // библиотека для работы с датчиком температуры по протоколу OneWire, требуется установка
#include <DateTime.h> // моя библиотека для работы с типом данных DateTime, требуется установка
#include "TM1637.h" // моя библиотека для дисплейного модуля, установлена локально в папке проекта
#include "pitches.h"  // ноты, установлена локально в папке проекта
#include "themes.h" // мелодии, установлена локально в папке проекта

const int8_t DS1307 = 104; // адрес модуля, заложен производителем
const int8_t BUZZER_PIN = A0; // подключаем пьезодинамик (зумер) на пин A7
const int8_t BTN_PIN = 2; // подключаем кнопку на пин D2
const int btnRetention = 3000; // время удержания кнопки
const int8_t DsSensor_PIN = 6; // подключаем цифровой датчик температуры DS18B20 на пин D2
const int UPDATE_TEMPERATURE_TIME = 10000; // период обновления температуры
const bool ENABLE_ALARM = HIGH; // включение будильника
const int8_t ALARM_MELODY = 1; // мелодия будильника (0 - пираты, 1 - крэйзи фрог, 2 - марио, 3 - титаник)

OneWire DsTemp(DsSensor_PIN); // создаём объект класса OneWire, подключаем датчик температуры на пин D6
TM1637 Disp(5, 4); // создаём объект класса TM1637, подключаем дисплей на пин D5 - CLK, D4 - DIO

const byte brightNight = 0; // яркость дисплея ночью
const byte brightDay = 4; // яркость дисплея днём
const uint8_t hourNight = 22; // час перехода на ночной режим
const uint8_t hourDay = 7; // час перехода на дневной режим
const uint8_t hourAlarmWeekday = 6; // час включения будильника в будни
const uint8_t minuteAlarmWeekday = 50; // минута включения будильника в будни
const uint8_t hourAlarmWeekend = 9; // час включения будильника в выходной
const uint8_t minuteAlarmWeekend = 0; // минута включения будильника в выходной

const uint16_t timeClock = 10000; // длительность отображения времени
const uint16_t timeDate = 2000; // длительность отображения даты
const uint16_t timeTemp = 2000; // длительность отображения температуры

unsigned long btnTimer = 0; // системное время изменения состояния кнопки
unsigned long workTime = 0; // время работы ардуины с момента включения
unsigned long lastUpdateTemperature = 0; // время последнего обновления температуры

DateTime currentDateTime; // текущее время

bool btnFlag = false; // флаг состояния кнопки
bool btnState = false; // состояние кнопки

byte indicationMode = 0; // режим индикации (0 - время, 1 - дата, 2 - температура)
uint8_t Bright = 0; // текущая яркость
float temperature = 25.0; // текущая температура

/*  функция конвертации двухзначного десятичного числа в его двоично-десятичный код
    модуль RTC DS1307 работает в двоично - десятичном формате
    двочно - десятичный формат - это представление десятичного число двоичным кодом
    где каждому разряду десятичного числа соответствуют 4 бита двоичного кода */
byte binaryCodedDecimal(byte var) {
  if (var > 100)
    return 0;
  else return((var / 10 << 4) + (var % 10));
}

//функция конвертации из двоично - десятичного формата в десятичный (обратное преобразование)
byte binaryCodedDecimalReverse(byte var) {
  return(((var >> 4) * 10) + (var & B1111));
}

// функция записи времени в модуль DS1307
void setTimeToDS1307 (DateTime curDateTime) {
  Wire.beginTransmission(DS1307); // инициализируем передачу данных
  Wire.write(0x00); // посылаем регистр с которого начнется запись данных в модуль
  Wire.write(binaryCodedDecimal(curDateTime.Second)); // посылаем секунды
  Wire.write(binaryCodedDecimal(curDateTime.Minute)); // посылаем минуты
  Wire.write(binaryCodedDecimal(curDateTime.Hour));   // посылаем часы
  Wire.write(binaryCodedDecimal(curDateTime.Day));    // посылаем день недели
  Wire.write(binaryCodedDecimal(curDateTime.Date));   // посылаем дата
  Wire.write(binaryCodedDecimal(curDateTime.Month));  // посылаем месяц
  Wire.write(binaryCodedDecimal(curDateTime.Year));   // посылаем год
  Wire.endTransmission(); // посылка данных и вывод результата передачи
}

// функция чтения данных из модуля DS1307
void readTimeFromDS1307() {
  Wire.beginTransmission(DS1307); // инициализируем передачу данных
  Wire.write(0x00); // посылаем регистр с которого начнется чтение данных из модуля
  Wire.endTransmission();  
  Wire.requestFrom(DS1307, 7); // запрашиваем 7 байт данных
  currentDateTime.Second = binaryCodedDecimalReverse(Wire.read());
  currentDateTime.Minute = binaryCodedDecimalReverse(Wire.read());
  currentDateTime.Hour   = binaryCodedDecimalReverse(Wire.read());
  currentDateTime.Day    = binaryCodedDecimalReverse(Wire.read());
  currentDateTime.Date   = binaryCodedDecimalReverse(Wire.read());
  currentDateTime.Month  = binaryCodedDecimalReverse(Wire.read());
  currentDateTime.Year   = binaryCodedDecimalReverse(Wire.read());
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // инициализируем последовательный интерфейс
  Wire.begin(); // инициализируем I2C интерфейс
  Disp.clearDisp(); // очистка дисплея
  readTimeFromDS1307(); // читаем время для правильной настройки яркости
  SetBright(Disp); // установка яркости
  pinMode(BTN_PIN, INPUT_PULLUP); // настраиваем пин на вход и подтягиваем его внутренним резистором ардуино (10кОм) к +5В 
  pinMode(BUZZER_PIN, OUTPUT); // настраиваем пин на выход

  Disp.hello(); // приветствие 
  delay(2000);
  Disp.clearDisp(); // очистка дисплея
}

void loop() {
  // put your main code here, to run repeatedly:
    SetBright(Disp); // установка яркости
    readTimeFromDS1307(); // обновление времени
    temperature = Temperature(); // обновление температуры
    btnProcessing(BTN_PIN); // обработка нажатия кнопки

  // будильник
  if (ENABLE_ALARM) {
    if (currentDateTime.Day >= 1 and currentDateTime.Day <= 5) {
      if (currentDateTime.Hour == hourAlarmWeekday and currentDateTime.Minute == minuteAlarmWeekday and currentDateTime.Second == 0) {
      Disp.setBright(7);
      Alarm(ALARM_MELODY); 
      Disp.succes(currentDateTime.Hour, currentDateTime.Minute);
      }
    }
    else if (currentDateTime.Hour == hourAlarmWeekend and currentDateTime.Minute == minuteAlarmWeekend and currentDateTime.Second == 0) {
      Disp.setBright(7);
      Alarm(ALARM_MELODY); 
      Disp.succes(currentDateTime.Hour, currentDateTime.Minute);
    }
  }
  
  // смена режимов индикации в соответствии с заданным временем отображения каждого режима
  if (indicationMode == 0 and ((millis() - workTime) > timeClock)) {
    workTime = millis();
    changeMode();
  }
  if (indicationMode == 1 and ((millis() - workTime) > timeDate)) {
    workTime = millis();
    changeMode();
  }
    if (indicationMode == 2 and ((millis() - workTime) > timeTemp)) {
    workTime = millis();
    changeMode();
  }
  
  switch(indicationMode) {
    case(0):
      Disp.dispTime(currentDateTime.Hour, currentDateTime.Minute);
      break;
    case(1):
      Disp.dispDate(currentDateTime.Date, currentDateTime.Month);
      break; 
     case(2):
      Disp.dispTemp(temperature); 
      break; 
  }
}

// установка яркости
void SetBright(TM1637 Disp){
    if (currentDateTime.Hour >= hourNight or currentDateTime.Hour < hourDay) {
      Disp.setBright(brightNight); // устанавливаем яркость дисплея
      Bright = brightNight;
    }
    else {
      Disp.setBright(brightDay);
      Bright = brightDay;
    }
}

// смена режима индикации 
void changeMode() {
  Disp.clearDisp();
  if (indicationMode == 2) {
    indicationMode = 0;
  }
  else  indicationMode ++ ;
}

// получение температуры от DS18B20
float Temperature(){
  if (millis() - lastUpdateTemperature > UPDATE_TEMPERATURE_TIME) {
    lastUpdateTemperature = millis();
    byte data[2]; // Массив для хранения данных от DS18B20 
    DsTemp.reset(); // Начинаем взаимодействие со сброса всех предыдущих команд и параметров
    DsTemp.write(0xCC); // Даем датчику DS18b20 команду пропустить поиск по адресу. В нашем случае только одно устрйоство
    DsTemp.write(0x44); // Даем датчику DS18b20 команду измерить температуру. Само значение температуры мы еще не получаем - датчик его положит во внутреннюю память

    DsTemp.reset(); // Теперь готовимся получить значение измеренной температуры
    DsTemp.write(0xCC); 
    DsTemp.write(0xBE); // Просим передать нам содержание регистров со значением температуры
  
    // Получаем и считываем ответ
    data[0] = DsTemp.read(); // Читаем младший байт значения температуры
    data[1] = DsTemp.read(); // А теперь старший
  
    // Формируем итоговое значение: 
    //    - сперва "склеиваем" значение, 
    //    - затем умножаем его на коэффициент, соответсвующий разрешающей способности (для 12 бит по умолчанию - это 0,0625)
    float temperature = ((data[1] << 8) | data[0]) * 0.0625;
    if (temperature != 85.0) {
      return (temperature);
    }
    else return(25.0);
    }
  }

void Alarm(int8_t melody){
  switch(melody){
    case (0):
      Play_Pirates();
      break;
    case (1):
      Play_CrazyFrog();
      break;
    case (2):
      Play_MarioUW();
      break;
    case (3):
      Play_Titanic();
      break;    
  }
}

// обработка нажатия кнопки для запуска настроеки времени
void btnProcessing(byte btn_pin) {
  btnState = !digitalRead(btn_pin);
  // Button press
  if (btnState and !btnFlag) {
    btnFlag = true;
    btnTimer = millis();
  }
  // Button retention
  if (btnState and btnFlag and millis() - btnTimer > btnRetention) {
    btnTimer = millis();
    Disp.succes(currentDateTime.Hour, currentDateTime.Minute);
    Disp.dispTime(currentDateTime.Hour, currentDateTime.Minute);
    currentDateTime.setDateTimeFromSerial();
    setTimeToDS1307(currentDateTime);
  }  
  // Button released
  if (!btnState and btnFlag) {
    btnFlag = false;
    btnTimer = millis();
  }
} 


// мелодии будильника
void Play_Pirates()
{ 
  for (int thisNote = 0; thisNote < (sizeof(Pirates_note)/sizeof(int)); thisNote++) {
    // моргание дисплея
    if (thisNote % 2 == 0) Disp.dispTime(currentDateTime.Hour, currentDateTime.Minute);   
    if (thisNote % 2 == 1) Disp.clearDisp();
    
    int noteDuration = 1000 / Pirates_duration[thisNote];//convert duration to time delay
    tone(BUZZER_PIN, Pirates_note[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.05; //Here 1.05 is tempo, increase to play it slower
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN); //stop music 
    }
}

void Play_CrazyFrog()
{
  for (int thisNote = 0; thisNote < (sizeof(CrazyFrog_note)/sizeof(int)); thisNote++) {
    // моргание дисплея
    if (thisNote % 2 == 0)Disp.dispTime(currentDateTime.Hour, currentDateTime.Minute);    
    if (thisNote % 2 == 1)Disp.clearDisp();
    
    int noteDuration = 1000 / CrazyFrog_duration[thisNote]; //convert duration to time delay
    tone(BUZZER_PIN, CrazyFrog_note[thisNote], noteDuration); 
    int pauseBetweenNotes = noteDuration * 1.30;//Here 1.30 is tempo, decrease to play it faster
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN); //stop music 
    }
}

void Play_MarioUW()
{
    for (int thisNote = 0; thisNote < (sizeof(MarioUW_note)/sizeof(int)); thisNote++) {
    // моргание дисплея
    if (thisNote % 2 == 0) Disp.dispTime(currentDateTime.Hour, currentDateTime.Minute);
    if (thisNote % 2 == 1) Disp.clearDisp();
    
    int noteDuration = 1000 / MarioUW_duration[thisNote];//convert duration to time delay
    tone(BUZZER_PIN, MarioUW_note[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.80;
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN); //stop music 
    }
}

void Play_Titanic()
{
    for (int thisNote = 0; thisNote < (sizeof(Titanic_note)/sizeof(int)); thisNote++) {
    // моргание дисплея
    if (thisNote % 2 == 0) Disp.dispTime(currentDateTime.Hour, currentDateTime.Minute);
    if (thisNote % 2 == 1) Disp.clearDisp();
    
    int noteDuration = 1000 / Titanic_duration[thisNote];//convert duration to time delay
    tone(BUZZER_PIN, Titanic_note[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 2.70;
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN); //stop music
    }
}
