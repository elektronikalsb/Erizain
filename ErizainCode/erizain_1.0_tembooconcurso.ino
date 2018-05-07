#include <Process.h>          // Librería que obtiene la hora utilizando internet
#include <Bridge.h>           // Librería que hace de puente de comunicación entre ATmega32U4 y Linux en Arduino Yun
#include <Temboo.h>           // Librería de Temboo
#include "TembooAccount.h"    // Libreria que contiene información de la cuenta de Temboo
#include <SoftwareSerial.h>   // Librería para abilitar puerto serie via sofware
#include <DFMiniMp3.h>        // Librería del reproductor mp3
#include <Servo.h>            // Librería que controla los servos

Servo servo1;                 ////
Servo servo2;                 // Declaración de servomotores
Servo servo3;                 ////

#define boton 7               // Boton para silenciar alarma asignado al pin 7

int hours, minutes, seconds;                          // Variables de hora actual
int lastHours, lastMinutes, lastSecond = -1;          // Variables de hora para comparaciones
int hora_evento, minutos_evento, pastilla_numero;     // Variables hora y pastilla de eventos de google calendar
int calls = 1, maxCalls = 50;                         // Fija el numero de peticiones de choreo en 1 y el numero maximo de peticiones posibles
String string="";                                     // String vacio
boolean json_var = 0;                                 // Variable de estado del codigo json


class Mp3Notify                                       //////
{                                                     //
public:                                               //
  static void OnError(uint16_t errorCode) {           //    
    Serial.println();                                 //
    Serial.print(F("Com Error "));                    //
    Serial.println(errorCode);                        //
  }                                                   //                                                      
  static void OnPlayFinished(uint16_t globalTrack) {  //
    Serial.println();                                 //
    Serial.print(F("Play finished for #"));           //
    Serial.println(globalTrack);                      //
  }                                                   //    
  static void OnCardOnline(uint16_t code) {           //    
    Serial.println();                                 //   Class para el DFPlayer que muestra notificaciones sobre su estado y sobre errores
    Serial.print(F("Card online "));                  //
    Serial.println(code);                             //
  }                                                   //
  static void OnCardInserted(uint16_t code) {         //
    Serial.println();                                 //
    Serial.print(F("Card inserted "));                //
    Serial.println(code);                             //
  }                                                   //
  static void OnCardRemoved(uint16_t code)  {         //
    Serial.println();                                 //
    Serial.print(F("Card removed "));                 //
    Serial.println(code);                             //
  }                                                   //
};                                                    //////

//DFMiniMp3<HardwareSerial, Mp3Notify> mp3(Serial1);  // descomentar esta linea para comunicar el DFPlayer por hardware serial
                                                      // la libreria process.h que hemos utilizado para obtener la hora no es compatible con el hardware serial
                                              
SoftwareSerial DFplayer(5, 4); // RX, TX              // DFplayer comunicado por software serial con los pines 4 y 5
SoftwareSerial nextion(2, 3); // RX, TX               // Pantalla Nextiom comunicada por software serial con los pines 4 y 5 // problemas para enviar datos
DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(DFplayer);   // linea de codigo para el mp3

Process date;                 // Se usa para obtener la hora


////////////////////////////////////////SETUP/////////////////////////////////////////////

void setup() {
  Bridge.begin();                   // Inicializa Bridge
  Serial.begin(9600);               // Inicializa puerto serie
  Serial.flush();                   // Limpia los datos del puerto serie
  DFplayer.begin(9600);             // Inicializa DFPlayer
  DFplayer.flush();                 // Limpia los datos de DFPlayer
  nextion.begin(9600);              // Inicializa Nextion
  nextion.flush();                  // Limpia los datos de Nextion
  string.reserve(200);              // Reserva 200 bits de memoria para guardar string 
  
  pinMode (boton, INPUT_PULLUP);    // Asigna el boton al pin 7 como entrada
  servo1.attach(9);                 // Inicializa servo 1 y le asigna el pin 9 (PWM)
  servo1.write(0);                  // Servo 1 se pone en posición inicial
  servo2.attach(10);                // Inicializa servo 1 y le asigna el pin 10 (PWM)
  servo2.write(0);                  // Servo 1 se pone en posición inicial
  servo3.attach(11);                // Inicializa servo 1 y le asigna el pin 11 (PWM)
  servo3.write(0);                  // Servo 1 se pone en posición inicial
  
  mp3.begin();                      // Inicializa reproductor mp3
  mp3config();                      // Llama a funcion de configuracion del reproductor mp3 (volumen, ecualizador...)

  delay(1000);                      ////
//  while(!Serial);                   // desactivar/activar linea para debugging/modo normal
  delay(1000);                      ///

  Serial.println(F("Time Check"));  // Serial print para debugging
  horaactual();                     // Llama a la funcion para obtener la hora por primera vez
  minutes=lastMinutes;              // Iguala las variables de minuto para futuras comparaciones
  hours=lastHours;                  // Iguala las variables de hora para futuras comparaciones

  if (!date.running()) {            ////
    date.begin("date");             // Fija la hora inicial usando la libreria process.h
    date.addParameter("+%T");       // Devuelve hh:mm:ss
    date.run();                     ///
  }
}

//////////////////////////////////////LOOP////////////////////////////////////////////

void loop() 
{
horaactual();                        // Esta funcion sincroniza la hora via internet y la actualiza cada segundo
if (minutes!=lastMinutes) {          // Comprueba si el minutero de la hora a cambiado
  lastMinutes=minutes;               // Iguala minutos y minutos anterior para una futura comparacion
  if (calls <= maxCalls) {
    Serial.println("Running GetNextEvent - Run #" + String(calls++));
    
    TembooChoreo GetNextEventChoreo;

    // Invoke the Temboo client
    GetNextEventChoreo.begin();

    // Set Temboo account credentials
    GetNextEventChoreo.setAccountName(TEMBOO_ACCOUNT);
    GetNextEventChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
    GetNextEventChoreo.setAppKey(TEMBOO_APP_KEY);
    
    // Set profile to use for execution
    GetNextEventChoreo.setProfile("erizain4getnextevent");
    
    // Identify the Choreo to run
    GetNextEventChoreo.setChoreo("/Library/Google/Calendar/GetNextEvent");
    
    // Run the Choreo; when results are available, print them to serial
    GetNextEventChoreo.run();
    
    while(GetNextEventChoreo.available()) {
      char c = GetNextEventChoreo.read();
      Serial.print(c);
       
       if ((c=='{')||(json_var==1)){                                      //////  
          json_var=1;                                                     //
          if(c!='\n'){                                                    // 
             if(c=='"')                                                   // El texto que recibimos cuando se lee un evento es un json,
                string+='\"';                                             // que contiene toda la informacion sobre el evento, converimos
             else                                                         // todos los caracteres del json recibido en una cadena de caracteres
               string+=c;                                                 // tipo string para poder acceder a la informacion que nos interesa del
          }                                                               // evento.
        else                                                              //
          json_var==0;                                                    //
     }                                                                    //
    }                                                                     //
    json_var=0;                                                           //////
    
    quepastilla();                                                        // Obtiene el numero de pastilla desde el string del evento
    quehora();                                                            // Obtiene la hora programada para el evento desde el string
    
    horaenpantalla(); //////////////////////cambiar de sitio              // Imprime la hora actual en la pantalla
    
      if((hora_evento == hours)&&(minutos_evento==minutes+1))  {          // Comprueba si la hora actual y la hora evento conciden
        Serial.println(F("hay pastilla"));                                // Serial print para debugging
        Serial.println(pastilla_numero);                                  // Imprime el numero de pastilla del evento en curso                      
        hora_evento=25;                                                   // Cambiamos el valor de hora a 25 para que no coincida hasta que cambie ///////////// creo que no hace falta
        
        tomapastilla1();                                                  //////     
        tomapastilla2();                                                  // dependiendo de la pastilla que toque se pone en marcha la funcion correspondiente, servo, pantalla, alarma...    
        tomapastilla3();                                                  //////
      }
      else  {
        Serial.println(F("no hay pastilla"));                             // Si no coincide hora actual y hora evento no hay pastilla
      }
      string="";                                                          // Borra los datos del string para poder guardar de nuevo la siguiente vez
      
      GetNextEventChoreo.close();                                         // Finaliza la peticion de Temboo
    }
  }
}

/////////////////////////////////////FUNCIONES/////////////////////////////////////////

void quepastilla() {
    int n=string.indexOf("description");
    String pastilla=string.substring(n+14,n+23);
    String n_pastilla=string.substring(n+22,n+23);
    Serial.println (pastilla);
    Serial.println (n_pastilla);
    pastilla_numero = n_pastilla.toInt();
}

void quehora() {
    int m=string.indexOf("start");
    String horaevento=string.substring(m+31,m+36);
    String hora=string.substring(m+31,m+33);
    String minutos=string.substring(m+34,m+36);
    hora_evento = hora.toInt();
    minutos_evento = minutos.toInt();
    Serial.println (horaevento);
    Serial.println (hora_evento);  
    Serial.println (minutos_evento);
    Serial.println (hours);
    Serial.println (minutes);  
}

void horaactual() {  
   while (date.available() > 0) {
    // get the result of the date process (should be hh:mm:ss):
    String timeString = date.readString();

    // find the colons:
    int firstColon = timeString.indexOf(":");
    int secondColon = timeString.lastIndexOf(":");

    // get the substrings for hour, minute second:
    String hourString = timeString.substring(0, firstColon);
    String minString = timeString.substring(firstColon + 1, secondColon);
    String secString = timeString.substring(secondColon + 1);
    
    // convert to ints,saving the previous second:
    
    hours = hourString.toInt();
    minutes = minString.toInt();
    lastSecond = seconds;          // save to do a time comparison
    seconds = secString.toInt();
  }

  if (lastSecond != seconds) { // if a second has passed
    // print the time:
    if (hours <= 9) {
      Serial.print("0");  // adjust for 0-9
    }
    Serial.print(hours);
    Serial.print(":");
    if (minutes <= 9) {
      Serial.print("0");  // adjust for 0-9
    }
    Serial.print(minutes);
    Serial.print(":");
    if (seconds <= 9) {
      Serial.print("0");  // adjust for 0-9
    }
    Serial.println(seconds);

    // restart the date process:
    if (!date.running()) {
      date.begin("date");
      date.addParameter("+%T");
      date.run();
    }
    Serial.println();
  }
  //if there's a result from the date process, parse it: 
}

void horaenpantalla() {  
    if (hours <= 9) {
      nextion.print("t_hora.txt=");
      nextion.write(0x22);
      nextion.print("0");
      nextion.print(hours);
      nextion.write(0x22);
      nextion.write(0xff);
      nextion.write(0xff);
      nextion.write(0xff);
    }
    else  {
      nextion.print("t_hora.txt=");
      nextion.write(0x22);
      nextion.print(hours);
      nextion.write(0x22);
      nextion.write(0xff);
      nextion.write(0xff);
      nextion.write(0xff);  
    }
      if (minutes <= 9) {    
      nextion.print("t_minuto.txt=");
      nextion.write(0x22);
      nextion.print("0");
      nextion.print(minutes);
      nextion.write(0x22);
      nextion.write(0xff);
      nextion.write(0xff);
      nextion.write(0xff);
    }
    else  {
      nextion.print("t_minuto.txt=");
      nextion.write(0x22);
      nextion.print(minutes);
      nextion.write(0x22);
      nextion.write(0xff);
      nextion.write(0xff);
      nextion.write(0xff);      
    }
}

void tomapastilla1() {
        if(pastilla_numero==1)  {
          Serial.println(F("cae la pastilla1"));
          mp3.playMp3FolderTrack(1);  // sd:/mp3/0001.mp3      
        
          nextion.print("t0.txt=");
          nextion.write(0x22);
          nextion.println("Hora de la pastilla 1");
          nextion.println("Pulsar boton para silenciar");
          nextion.write(0x22);
          nextion.write(0xff);
          nextion.write(0xff);
          nextion.write(0xff);  

          servo1.write(175);
          delay(2000);
          servo1.write(25);
          
          while((digitalRead(boton)==HIGH) && (minutes==lastMinutes)){
            horaactual();         
          }
          mp3.stop();
          nextion.print("t0.txt=");
          nextion.write(0x22);
          nextion.println("");
          nextion.println("");
          nextion.write(0x22);
          nextion.write(0xff);
          nextion.write(0xff);
          nextion.write(0xff);             
          }
}

void tomapastilla2() { 
        if(pastilla_numero==2)  {
          Serial.println(F("cae la pastilla2"));
          mp3.playMp3FolderTrack(1);  // sd:/mp3/0001.mp3
        
          nextion.print("t0.txt=");
          nextion.write(0x22);
          nextion.println("Hora de la pastilla 2");
          nextion.println("Pulsar boton para silenciar");
          nextion.write(0x22);
          nextion.write(0xff);
          nextion.write(0xff);
          nextion.write(0xff);
         
          servo2.write(180);
          delay(2000);
          servo2.write(25);

          while((digitalRead(boton)==HIGH) && (minutes==lastMinutes)){
            horaactual();
            }
          mp3.stop();
          nextion.print("t0.txt=");
          nextion.write(0x22);
          nextion.println("");
          nextion.println("");
          nextion.write(0x22);
          nextion.write(0xff);
          nextion.write(0xff);
          nextion.write(0xff);
        }
}

void tomapastilla3() {
          if(pastilla_numero==3)  {
          Serial.println(F("cae la pastilla3"));
          mp3.playMp3FolderTrack(1);  // sd:/mp3/0001.mp3

          nextion.print("t0.txt=");
          nextion.write(0x22);
          nextion.println("Hora de la pastilla 3");
          nextion.println("Pulsar boton para silenciar");
          nextion.write(0x22);
          nextion.write(0xff);
          nextion.write(0xff);
          nextion.write(0xff); 
        
          servo3.write(180);
          delay(2000);
          servo3.write(25);
          while((digitalRead(boton)==HIGH) && (minutes==lastMinutes)){
            horaactual();
            }
          mp3.stop();
          nextion.print("t0.txt=");
          nextion.write(0x22);
          nextion.println("");
          nextion.println("");
          nextion.write(0x22);
          nextion.write(0xff);
          nextion.write(0xff);
          nextion.write(0xff);
          }
}

void mp3config()  {
  uint16_t volume = mp3.getVolume();
  Serial.print(F("volume "));
  Serial.println(volume);
  mp3.setVolume(15);
  
  uint16_t count = mp3.getTotalTrackCount();
  Serial.print(F("files "));
  Serial.println(count);
  
  Serial.println(F("starting..."));
}

void waitMilliseconds(uint16_t msWait)  {
  uint32_t start = millis();
  
  while ((millis() - start) < msWait)
  {
    // calling mp3.loop() periodically allows for notifications 
    // to be handled without interrupts
    mp3.loop(); 
    delay(1);
  }
}
