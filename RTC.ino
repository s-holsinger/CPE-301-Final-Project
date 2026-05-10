#include <RTClib.h>
#include <Wire.h>
#include <stdlib.h>
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
"Friday", "Saturday"};

//UART functions-------------
//intialize 9600
void UART_init(unsigned int ubrr){

  UBRR0H = (unsigned char)(ubrr >> 8);
  UBRR0L = (unsigned char)ubrr;

  UCSR0B = (1 << TXEN0); //EN transmitter
  UCSR0C = (1<< UCSZ01)|(1<<UCSZ00); //8-bit data
}

//This should send one character 
void UART_sendChar(char data){
  while (!(UCSR0A & (1 << UDRE0)));

  UDR0 = data;
}

//Should send string 
void UART_sendString(const char *str){
  while (*str){
    UART_sendChar(*str);
    str++;
  }
}

void setup() {
  
UART_init(103);


rtc.begin();
  //*updates RTC to real time----
//rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  
UART_sendString("DS1307RTC Read Test \r\n");
UART_sendString("-------------------\r\n");
}

unsigned long previousTime = 0;
const unsigned long interval = 60000;

void loop() {

  char buffer[12];

  if (millis() - previousTime >= interval){
    previousTime = millis();

    DateTime now = rtc.now();
     UART_sendString("TEST\r\n");
    
  //for the YEAR
  itoa(now.year(), buffer, 10);
  UART_sendString(buffer);
  UART_sendChar('/');
    
  //for the MONTH
  itoa(now.month(), buffer, 10);
  UART_sendString(buffer);
  UART_sendChar('/');
    
  //for the DAY
  itoa(now.day(), buffer, 10);
  UART_sendString(buffer);
  UART_sendChar('/');
    
  //day of the week
  UART_sendString("(");
  UART_sendString(daysOfTheWeek[now.dayOfTheWeek()]);
  UART_sendString(")");
    
  //for the HOUR
  UART_sendString(" ");
  itoa(now.hour(), buffer, 10);
  UART_sendString(buffer);
  UART_sendChar(':');
  
  //for the Minute
  itoa(now.minute(), buffer, 10);
  UART_sendString(buffer);
  UART_sendChar(':');
  
  //for the Second
  itoa(now.second(), buffer, 10);
  UART_sendString(buffer);
    
  UART_sendString("\r\n");
  //delay(60000);
  }
}
}
