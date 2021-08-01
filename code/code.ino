#include <SD.h>               
#include <LiquidCrystal.h>    
#include <RTClib.h>           
 

LiquidCrystal lcd(2, 3, 4, 5, 6, 7);
 

RTC_DS3231 rtc;
DateTime   now;
 

#define button1       A1   
#define button2       A2   
#define DS18B20_PIN   A3
 
File dataLog;
boolean sd_ok = 0;
 
void setup()
{
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
 
  rtc.begin();          
  lcd.begin(20, 4);     
  lcd.setCursor(0, 3);  
  lcd.print("Temp:");
 
  
  Serial.begin(9600);
  Serial.print("Initializing SD card...");
 
  
  if ( !SD.begin() )
    Serial.println("initialization failed!");  
 
  else {   
    sd_ok = 1;
    Serial.println("initialization done.");
    if( SD.exists("Log.txt") == 0 )   
    {  
      Serial.print("\r\nCreate 'Log.txt' file ... ");
      dataLog = SD.open("Log.txt", FILE_WRITE);   
      if(dataLog) {                               
        Serial.println("OK");
        dataLog.println("    DATE    |    TIME  | TEMPERATURE");
        dataLog.println("(dd-mm-yyyy)|(hh:mm:ss)|");
        dataLog.close();   
      }
      else
        Serial.println("error creating file.");
    }
  }
 
  Serial.println("\r\n    DATE    |    TIME  | TEMPERATURE");
  Serial.println("(dd-mm-yyyy)|(hh:mm:ss)|");
}
 



void loop()
{
  now = rtc.now();  
  RTC_display();    
 
  if( !digitalRead(button1) )  
  if( debounce() )             
  {
    while( debounce() );  
 
    byte hour   = edit( now.hour() );         
    byte minute = edit( now.minute() );       
    byte day    = edit( now.day() );          
    byte month  = edit( now.month() );        
    byte year   = edit( now.year() - 2000 );  
 
    
    rtc.adjust(DateTime(2000 + year, month, day, hour, minute, 0));
 
    while(debounce());  
  }
 
  static byte p_second;
  if( (now.second() % 2 == 0) && (p_second != now.second()) )
  {   
    unsigned int ds18b20_temp;
    char buffer1[12], buffer2[26];
    bool sensor_ok = 0;
    p_second = now.second();
    if( ds18b20_read(&ds18b20_temp) )
    {
      sensor_ok = 1;
      if (ds18b20_temp & 0x8000)           
      {
        ds18b20_temp = ~ds18b20_temp + 1;  
        sprintf(buffer1, "-%02u.%04u%cC", (ds18b20_temp/16) % 100, (ds18b20_temp & 0x0F) * 625, 223);
      }
      else
      {      
        if (ds18b20_temp/16 >= 100)          
          sprintf(buffer1, "%03u.%04u%cC", ds18b20_temp/16, (ds18b20_temp & 0x0F) * 625, 223);
        else      
          sprintf(buffer1, " %02u.%04u%cC", ds18b20_temp/16, (ds18b20_temp & 0x0F) * 625, 223);
      }
    }
    else
      sprintf(buffer1, "   ERROR  ");
 
    lcd.setCursor(5, 3);
    lcd.print(buffer1);
 
    sprintf( buffer2, " %02u-%02u-%04u | %02u:%02u:%02u | ", now.day(), now.month(), now.year(),
                      now.hour(), now.minute(), now.second() );
    if(sensor_ok) {
      buffer1[8] = 194;   
      buffer1[9] = 176;
      buffer1[10] = 'C';  
      buffer1[11] = '\0'; 
    }
    
    Serial.print(buffer2);
    Serial.println(buffer1);
    
    if(sd_ok)
    {  
      dataLog = SD.open("Log.txt", FILE_WRITE);
      dataLog.print( buffer2 );
      dataLog.println( buffer1 );
      dataLog.close();   
    }
 
  }
 
  delay(100);   
}
 



void RTC_display()
{
  char _buffer[17];
  char dow_matrix[7][10] = {" SUNDAY  ", " MONDAY  ", " TUESDAY ", "WEDNESDAY",
                             "THURSDAY ", " FRIDAY  ", "SATURDAY "};
  lcd.setCursor(4, 0);
  lcd.print( dow_matrix[now.dayOfTheWeek()] );
 
  
  sprintf( _buffer, "TIME: %02u:%02u:%02u", now.hour(), now.minute(), now.second() );
  lcd.setCursor(0, 1);
  lcd.print(_buffer);
  
  sprintf( _buffer, "DATE: %02u-%02u-%04u", now.day(), now.month(), now.year() );
  lcd.setCursor(0, 2);
  lcd.print(_buffer);
}
 
byte edit(byte parameter)
{
  static byte i = 0, y_pos,
              x_pos[5] = {6, 9, 6, 9, 14};
  char text[3];
  sprintf(text,"%02u", parameter);
 
  if(i < 2)
    y_pos = 1;
  else
    y_pos = 2;
 
  while( debounce() );   
 
  while(true) {
    while( !digitalRead(button2) ) {  
      parameter++;
      if(i == 0 && parameter > 23)    
        parameter = 0;
      if(i == 1 && parameter > 59)    
        parameter = 0;
      if(i == 2 && parameter > 31)    
        parameter = 1;
      if(i == 3 && parameter > 12)    
        parameter = 1;
      if(i == 4 && parameter > 99)    
        parameter = 0;
 
      sprintf(text,"%02u", parameter);
      lcd.setCursor(x_pos[i], y_pos);
      lcd.print(text);
      delay(200);       
    }
 
    lcd.setCursor(x_pos[i], y_pos);
    lcd.print("  ");
    unsigned long previous_m = millis();
    while( (millis() - previous_m < 250) && digitalRead(button1) && digitalRead(button2) ) ;
    lcd.setCursor(x_pos[i], y_pos);
    lcd.print(text);
    previous_m = millis();
    while( (millis() - previous_m < 250) && digitalRead(button1) && digitalRead(button2) ) ;
 
    if(!digitalRead(button1))
    {                     
      i = (i + 1) % 5;    
      return parameter;   
    }
  }
}
 

bool debounce ()
{
  byte count = 0;
  for(byte i = 0; i < 5; i++)
  {
    if ( !digitalRead(button1) )
      count++;
    delay(10);
  }
 
  if(count > 2)  return 1;
  else           return 0;
}

 

bool ds18b20_start()
{
  bool ret = 0;
  digitalWrite(DS18B20_PIN, LOW); 
  pinMode(DS18B20_PIN, OUTPUT);
  delayMicroseconds(500);      
  pinMode(DS18B20_PIN, INPUT);
  delayMicroseconds(100);      
  if (!digitalRead(DS18B20_PIN))
  {
    ret = 1;                  
    delayMicroseconds(400);   
  }
  return(ret);
}
 
void ds18b20_write_bit(bool value)
{
  digitalWrite(DS18B20_PIN, LOW);
  pinMode(DS18B20_PIN, OUTPUT);
  delayMicroseconds(2);
  digitalWrite(DS18B20_PIN, value);
  delayMicroseconds(80);
  pinMode(DS18B20_PIN, INPUT);
  delayMicroseconds(2);
}
 
void ds18b20_write_byte(byte value)
{
  byte i;
  for(i = 0; i < 8; i++)
    ds18b20_write_bit(bitRead(value, i));
}
 
bool ds18b20_read_bit(void)
{
  bool value;
  digitalWrite(DS18B20_PIN, LOW);
  pinMode(DS18B20_PIN, OUTPUT);
  delayMicroseconds(2);
  pinMode(DS18B20_PIN, INPUT);
  delayMicroseconds(5);
  value = digitalRead(DS18B20_PIN);
  delayMicroseconds(100);
  return value;
}
 
byte ds18b20_read_byte(void)
{
  byte i, value;
  for(i = 0; i < 8; i++)
    bitWrite(value, i, ds18b20_read_bit());
  return value;
}
 
bool ds18b20_read(int *raw_temp_value)
{
  if (!ds18b20_start())
    return(0);
  ds18b20_write_byte(0xCC);
  ds18b20_write_byte(0x44);   
  while(ds18b20_read_byte() == 0);  
  if (!ds18b20_start())             
    return(0);                      
  ds18b20_write_byte(0xCC);         
  ds18b20_write_byte(0xBE);         
 
  
  *raw_temp_value = ds18b20_read_byte();
  
  *raw_temp_value |= (unsigned int)(ds18b20_read_byte() << 8);
 
  return(1);  
}
