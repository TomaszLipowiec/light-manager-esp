#include <ESP8266WiFi.h>

const char* ssid = "";                  //wifi name
const char* password = "";              //wifi passwd
const char* host = "";                  //application ip
const int serverPort = 35000;

WiFiServer server(80);

WiFiClient manager;
WiFiClient client;
char servMsg;
char msgBuffor[7];
const char* name = "lm-xxx";           //lamp label
int mode = 0;

bool paired = false;
bool commandRec = false;
int iterator = 0;

int relay = 0;
int moveSens = 1;
int syncTime = 0;
bool prevMoveState = false;
bool moveState = false; 
bool espState = false;

void connectToManager();
void makePairWithManager(char servMsg);
void proccedCommandsFromManager();

void setup() {
  // put your setup code here, to run once:
  delay(1000);

  WiFi.begin(ssid, password);
  
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  connectToManager();

  pinMode(moveSens, INPUT_PULLUP);
  pinMode(relay, OUTPUT);

  digitalWrite(relay, LOW);
}

void loop() {

  //Pairing with manager
  while(client.available()){
    delay(100);
    servMsg = client.read();
    makePairWithManager(servMsg);
  }

  //Waiting and procceding command from manager
  if(paired){
    if(mode == 0){
      client.stop();
      delay(500);
      server.begin();
      delay(500);
      mode = 1;
    }
    proccedCommandFromManager();
  }


  //Motion detector
  moveState = digitalRead(moveSens);
  if(moveState != prevMoveState){
    if(moveState == true && espState == false){
      digitalWrite(relay, LOW);
    }

    if(moveState == false && espState == false){
      digitalWrite(relay, HIGH);
    }
  }

  prevMoveState = moveState;
}


void connectToManager()
{
  //Trying to conn with manager
  while(!client.connect(host, serverPort)){
    
    delay(5000);
  }

  //Connected with manager!
}

void makePairWithManager(char servMsg){
    if((servMsg == 'l' || msgBuffor[0] == 'l' || (msgBuffor[0] == 'l' && msgBuffor[1] == 'm')) && servMsg != '\n' && iterator < 7)
    {
      msgBuffor[iterator] = servMsg;
      iterator++;
    }

    if((msgBuffor[0] == 'l' && msgBuffor[1] == 'm') && servMsg == '\n'){
        String buffor = "0123456";
        for(int i=0; i<7; i++){
          buffor[i] = msgBuffor[i];
        }

        if(buffor == "lm-conn" && !paired)
        {
          client.write(name);
          paired = true;
          iterator = 0;
        }
    }
}

void proccedCommandFromManager()
{
    manager = server.available();
    if(manager)
    {
      int managerMode = 0; // 1 - command, 2 - ping
      while(manager.connected())
      {
        bool messageRec = false;
        if(manager.available())
        {
          if(manager.find("GET /")){

            while(manager.findUntil("lm-comm", "\r\n"))
            {
              
              char ceq = manager.read();
              char val = manager.read();

              if(ceq == '=' && val == '0')
              {
                //turning off light!
                managerMode = 1;
                digitalWrite(relay, LOW);
                espState = true;
                messageRec = true;
              }

              if(ceq == '=' && val == '1'){
                //turning on light!
                managerMode = 1;
                digitalWrite(relay, HIGH);
                espState = false;
                messageRec = true;
              }

              if(ceq == '=' && val == '2'){
                managerMode = 2;
                messageRec = true;

                ceq = manager.read();

                String buffor = "012345";
                for(int i=0; i<6; i++){
                  buffor[i] = manager.read();
                }

                if(ceq == '=' && buffor == name)
                {
                  messageRec = true;
                }
              }
            }
          }
        }

        if((messageRec && managerMode == 1) || (messageRec && managerMode == 2)){
          manager.println("HTTP/1.1 200 OK");
          manager.println();
        } else {
          manager.println("HTTP/1.1 400 Bad Request");
          manager.println();
        }
        break;
      }
    }

    delay(100);
    manager.stop();
}

