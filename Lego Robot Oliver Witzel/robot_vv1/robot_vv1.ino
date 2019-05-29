#include <Servo.h>// Ett servo bibliotek som gör det enklare att kontrolera servot
#define sf 4// sf går till den integrerade H bryggan A i esp kortet och snurrar framåt 
#define sba 0//sba går till samma hbrygga men vrider motorn backåt
#define rpb 5// rpm bestämmer plusen på PWN som styr varv talet på roboten
#define YLi 14// går till den gula LED:en
#define RLi 2// går till den Röda LED:en
#define GLi 12// går till den gröna LED:en 
#define Button 16//input för knappens signal
#include <Wire.h>//
#include "EspMQTTClient.h"// ett biblotek som gör det enklare att skicka/hantera mqtt packet
#include <ArduinoJson.h>// ett biblotek som  gör det enklare att skapa Json filer

Servo My_servo;//MY_servo är namnet för det servot som styr svängningen
Servo look;// Look är det servot som svänger sensron vilket ger sensorn en 140graders synfält

int Loop =0;// en int som kommer hålla koll på hur många gånger stop caset körs på rad
int proxList[140];// en array som fylls med det avstånd som sensorn uppfattar per grad
int grad = 0;// graden som look servot har snurrat
int Pot;// pot är den som läses av av sensorn
int distance = 0;//Avstånds variable som ändras av sensorn 
int velocity = 0;// den totala hastigheten på roboten
bool Press = false;//Tryck bool som ändras när man trycker på knappen
int Speed = 400;// standard farten för roboten
int diri = 1;// riktnigen som den åker åt 1= fram 0= bak
bool last_turn = false;// den senaste svängen som roboten tog

unsigned long previousMillis = 0;
unsigned long currentMillis;

typedef enum Direction{// alla cases
  Standby,//standby gör inget väntar på att bli startad
  Forward,//forward åker framåt beroende på sensor värden
  Stop,// stop stannar och backar 
  };
Direction di;// förkortning för directions

void onConnectionEstablished();//förklarar att onConnectionEstablished existera

EspMQTTClient client(//skapar en client som används för att connecta till hemsidan
  "ABBIndgymIoT_2.4GHz",                 // Wifi ssid
  "ValkommenHit!",                 // Wifi password
  "192.168.0.116",                   // MQTT broker ip
  1883,                   // MQTT broker port
  "oliner",              // MQTT username
  "apa",             // MQTT password
  "microdator",                 // Client name
  onConnectionEstablished,// Connection established callback
  true,                   // Enable web updater
  true                    // Enable debug messages
);

void setup() {
  di = Standby;// det förta caset blir standby
  pinMode(sf,OUTPUT);//förklarar sf som en output pin
  pinMode(sba,OUTPUT);//sba blir output pin
  pinMode(rpb,OUTPUT); //rpb -||-
  pinMode(RLi,OUTPUT);//RLi-||-
  pinMode(YLi,OUTPUT);//YLi-||-
  pinMode(GLi,OUTPUT); //GLi-||-
  pinMode(Button,INPUT);//button blir input eftersom den ska registrera ett knapp tryck
  look.attach(13);//declarerar vilken pin som look servot strys från
  My_servo.attach(15);//declarerar vilken pin som styr servot körs från
  Serial.begin(115200);//frekvensen som Serial printas den måste stämma överens med mqqt så att man kan få korekta error medelande
  look.write(0);//startar sevor på grad 0
  My_servo.write(70);// startar servot på mitten då den totala graden blir 140
}

void Send(String dis, String angle){// ensend funktion som skapar ett json objekt som skickas vi mqqt till hemsidan
  StaticJsonBuffer<150> jsonBuffer; //Skapar en buffer, det vill säga så mycket minne som vårt blivande jsonobjekt får använda.
  JsonObject& root = jsonBuffer.createObject(); //Skapar ett jsonobjekt som vi kallar root
  root["dis"] = dis; //Skapar parameterna dis och ger den avståndet till väggen
  root["pos"]= String(angle);//pos vilket blir positionen i en array på hemsidan
  String buffer; //Skapar en string som vi kallar buffer
  root.printTo(buffer); //Lägger över och konverterar vårt jsonobjekt till en string och sparar det i buffer variabeln.
  Serial.println(buffer);// skriver ut det den ska skicka så man kan se att inget gick snet
  client.publish("json",buffer);//publiserar vårat medelande via clienten som vi declarerade ovan
}
void onConnectionEstablished(){}

void accerlation (int maxspeed,int dir,int duration){// en function som accelererar robotet framåt
  float delaytime = duration/ (maxspeed-100);// räknar ut delay time så man får den durationen som man vill ha
  if (velocity!=maxspeed){//om den nya farten skiljer sig från den gamla så ska det sku en accerlation
    velocity= maxspeed;// sätter robotens totala fart till den nya
    if (dir == 1){//kollar om man vill accelerera framåt
      
      digitalWrite(sf, HIGH);
      digitalWrite(sba,LOW);
      for(int v = 100; v<maxspeed; v+=1){//för varje cykel så ökar hastig heten till ett tills man har nått den slutliga hastigheten
        analogWrite(rpb,v);
        delay(delaytime);}}
        
    else{// körs om man vill baka
      digitalWrite(sba, HIGH);
      digitalWrite(sf,LOW);
      for(int v = 100; v<maxspeed; v+=1){//samma som ovan
        analogWrite(rpb,v);
        delay(delaytime);}}}}

void retard(int endspeed, int duration){// en funktion för att retardera (bromsa in) med duration och den slutliga hastigheten som man vill ha
  if(velocity =! endspeed){//inget körs om inte den slutliga hastigheten skiljer sig från den totala   
    float delaytime = duration/(velocity-endspeed);//räknar ut paus tiden om if satsen ovan inte hade existerat så skulle man dividerat med 0 och fått en oändlig lång paus tid
    for(int s=velocity;s>endspeed; s-= 1){//för varje cykel så minskar den totala hastigheten med ett tills man har nått den valda hastigheten
      analogWrite(rpb,s);
      delay(delaytime);
      }
    velocity = endspeed;// updaterar robotens totala fart
    }}

void turn(int dir){// en sväng funktion
    if (dir == 1){// svänger Höger om dir =1
      My_servo.write(140);
      delay(100);
      Serial.println("RIGHT");
    }else{//annars svänger den höger
      My_servo.write(0);
      delay(100);
      Serial.println("LEFT");//skriver vart den svänger för att göra det enklare att felsöka
    }  
      Serial.println(diri);
      if(diri==1){//OM ROBOTENS RIKTNING är frammåt så gasar den i kurvan
        accerlation(1000,diri,250);
        delay(400);
        accerlation(Speed,diri,250);// bromsar in igen
      }else if(diri == 0){//annars så backar den
         Serial.println("reverse");// visar att den backar vilket gör det enklare att felsöka
         accerlation(1000,diri,250);
         delay(800);
         retard(0,250);// sen stannar den
      }
      My_servo.write(70);// svänger upp hjulen igen
    }  

void prox(){
  for(int x = 0;x<140; x+=1){//för varje grad och tar av ståndet 
    look.write(x);//snurrar till den grad som körs
    delay(3);//väntar tre milli sekunder för att kompensera för den mekaniska delayen
    distance = analogRead(Pot);// tar in det rå värdet från sensorn
    int cm = pow(3027.4/distance, 1.2134); //konverterar distansen(cm)
    proxList[x] = cm;// lägger in det i arrayen
    if(x>30 and x<110 and diri== 1 and cm <7){// kollar så att det inte är en vägg i vägen
      di= Stop;
      diri = 0;
      retard(0,100);// isåfall stannar den och förbered
    };
    if(di==Standby){
      Send(String(cm),String(x));// den skickar bara till hemsidan uder standby eftersom det skapas lagg när den kör i labyrinten så den ska endast demonstrera att det funkar
    }};
  look.write(0);//ställer tillbaka servot 
  Serial.println(proxList[0]);// skriver ut vad den såg sist och först för att enklare felsöka
  Serial.println(proxList[139]);
  Serial.println("Prox");// indekerar att den körde prox
  delay(300);
  }

void loop() {//den loop som kör hela programmet
  switch(di){// här nedan förklaras vad alla cases gör
    case Standby:
      digitalWrite(YLi,HIGH);// tänder den gula lampan
      Press = digitalRead(Button);//läser av efter knapp tryck
      if (Press == true){// om knappen trycks så släcks den gula lampan
        Press = false;
        digitalWrite(YLi,LOW);
        diri = 1;// förklarar att den ska börja åka frammåt
        accerlation(Speed,diri,300);// acceelererar framåt
        di = Forward;// byter case till forward
        break;
        };
      prox();// kör prox funktionen
      break;
    case Forward:
      digitalWrite(RLi,LOW);// om den röda lampan va på så släcks den
      digitalWrite(GLi,HIGH);// den gröna lampan tänds 
      prox();//kör prox funktionen
      if(di == Stop){// om di ändras i prox så avbryts caset direkt
        break;
      }
      if(proxList[0]>proxList[139]-1 and proxList[0]<proxList[139]+1 and proxList[70]<7){
        // om av avstånden på båda sidorna är samma så fortsätter den framåt        
        break; 
      }else if(proxList[0]>proxList[139]+2){//om en utav sidorna är längre ifrån än den andra så svänger den i från den och sätter last_turn till det motsatta eftersom man när man backar så svänger man åt måtsatt håll
        turn(1);
        last_turn = false;
      }else if(proxList[0]<proxList[139]-2){
        turn(0);
        last_turn = true;
      }
      break;
    case Stop:
      digitalWrite(GLi,LOW);//släcker den gröna lampan
      digitalWrite(RLi,HIGH);// tänder den röda
      prox();// gör en prox igen för att se var allting är
      if((proxList[0]>proxList[139]-2 and proxList[0]<proxList[139]+2) or Loop ==3){//om avståndet är lika så baackar den rakt bakåt eller om den har kört igenom caset 3 gånger utan att ha gjort något
        Loop=0;// resetar loop utifall den skulle bugga sig igen
        accerlation(Speed,diri,250);//accelererar bakåt
        digitalWrite(RLi,LOW);//blinkar den röda lampan
        delay(500);
        digitalWrite(RLi,HIGH);
        delay(500);
        break; 
      }else if(proxList[0]>proxList[139]+2){//om ena avståndet till vägen är större än det andra så svänger den
        Loop=0;//loop resetas
        turn(last_turn);//svänger enligt last turn
        retard(0,250);// stannar
        prox();// kör en prox igen
        diri = 1;// ställer in roboten på att åka frammåt
        turn(1);// svänger iväg från väggen
        last_turn = false;// sääter last turn på motsatt
        delay(400);// en delay så den kan köra lite framåt
        di = Forward;//bytter till case forward
      }else if(proxList[0]<proxList[139]-2){// samma som ovan fast inverterade svängar
        Loop=0;
        turn(last_turn);
        My_servo.write(70);
        prox();
        diri=1;
        turn(0);
        last_turn = true;
        delay(400);
        di = Forward;
      }
      Loop+=1;// lägger till 

      Serial.println("STOP");// indikerar att den stannade
     
      break;
  
}
  if (di== Standby){//om caset är på standby så updateras client så att man inte får lagg när roboten kör
    client.loop();
  }

}
