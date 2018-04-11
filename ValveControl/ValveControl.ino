#include <SoftwareSerial.h>
#include <Stepper.h>
#include <LiquidCrystal.h>

#define DEBUG true

boolean testWithoutWiFi = true; // added for quick testing without wifi
boolean inTestMode = true;

// const vars for valve states

int const STATE_VALVE_OPEN = 1000;
int const STATE_VALVE_CLOSED = 2000;
int const STATE_VALVE_HALF = -1000;

int currentValveState = STATE_VALVE_HALF;

int flashEveryAmount = 50;
int ledState = 0;

//---( Number of steps per revolution of INTERNAL motor in 4-step mode ) NOT USED
#define STEPS_PER_MOTOR_REVOLUTION 8   

//---( Steps per OUTPUT SHAFT of gear reduction )---
#define STEPS_PER_OUTPUT_REVOLUTION -2 * 64  //2048 


 
SoftwareSerial ESP8266(10, 11); // RX, TX

//String wifiNetwork = "MOHAESP"; // Garder les guillemets
//String Password = "password"; // Garder les guillemets

String wifiNetwork = "wolfradiolan"; // Garder les guillemets
String Password = "12345678"; // Garder les guillemets

boolean valveMoving = false;

boolean startup = true; 

//#define COMMON_ANODE

int redPin = 13;
int greenPin = 12;
int bluePin = 9;



// sensor pins
int openSensorPin = 8;
int closeSensorPin = 7;
int liquidSesnorPin = 6;



// Initialize Stepper Motor
//Stepper small_stepper(STEPS_PER_MOTOR_REVOLUTION, 2, 4, 3, 5);
Stepper small_stepper(200, 2, 3, 4, 5);
int  Steps2Take;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(A0, A1, A5, A4, A3, A2);

int delayTime2 = 350; // Delay between shifts (used by Scroll function)


/****************************************************************/
/*                       INITIALISATION                         */
/****************************************************************/
void setup()
{  
  // Analog Pin to connect with LCD
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print(" INITIALISATION ");

  pinMode(bluePin,OUTPUT);  
  pinMode(greenPin,OUTPUT);
  pinMode(redPin,OUTPUT);
  
  pinMode(openSensorPin,INPUT);
  pinMode(closeSensorPin,INPUT);
  pinMode(liquidSesnorPin,INPUT);
  
  
  setColor(255, 0, 0); // Turn the RGB Led Red (Valve closed)

  // Initialise the serial com
  Serial.begin(9600);  

   if (!testWithoutWiFi) // added so quick testing can take place......
  {
  ESP8266.begin(115200);
  sendToESP8266("AT+CIOBAUD=9600");
  receiveFromESP8266(4000);
  ESP8266.begin(9600);  

 
  InitESP8266();
  }
  // Display info on LCD
  lcd.print(" INITIALISATION ");
  lcd.setCursor(0, 1);
  lcd.print("      DONE      ");
}

void loop()
{

// ------------------------------- Checks to see if there is liquid in the chamber  ------------------------------ //

//----------------- POSSIBLE BUG BELOW WILL NEED TO BE CHANGED TO ACCOUNT FOR NORMAL LIQUID FLOW IN FINAL VERSION --------------------- 

  if (liquidSesnorPin==HIGH) // checks to see if float switch is triggered - ie there is liquid in the chamber
    {
//    currentValveState = STATE_VALVE_CLOSED; // by pass untill sensor fitted
    //  SendData("full"); // sends message to website saying that the chamber is full 
    }
//------------------------------------------------  BUG ABOVE -------------------------------------------------------------


  // ------------------------------- Startup check gets the state of the valve ------------------------------ //
       

  if (currentValveState==STATE_VALVE_HALF) // on first loop the current valve state will not be open or closed
  {
  int currentState = checkValveState(); // gets the current state of valve  
        Serial.println("Current Valve State: ");
        Serial.print(currentState);

  switch(currentState)
  {
    case STATE_VALVE_OPEN:
    {
       
      currentValveState = STATE_VALVE_OPEN;
      setColor(0, 255, 0); // sets led to green
      break;
    }
    case STATE_VALVE_CLOSED:
    {
      currentValveState = STATE_VALVE_CLOSED;
      setColor(255, 0, 0); // sets the led to red
      break;
    }
    case STATE_VALVE_HALF:
    {
      Serial.println("Closing Valve");
       
      closeValve(); // closes valve
      
    
    //  SendData("Valve closed"); // sents message to website saying that the valve is closed 
      break;
    }
    
  }
  }
  else
  {
  mainLoop(); // runs the main loop
 
  }
 
}



void mainLoop()
{
  
  // Display info on LCD

  //print the number of seconds since reset:
  //lcd.print(millis() / 1000);
  
  // lcd.clear();
  // set the cursor to column 0, line 0 (line 1 is the second row, since counting begins with 0) 
  lcd.setCursor(0, 0);
  lcd.print(" LIQUID FLOWING  ");
  lcd.setCursor(0, 1);
  lcd.print(" \177 \177 \177 \177 \177 \177 \177 \177"); // Display arrow to the LCD screen (Octal)
  delay(500);
  lcd.setCursor(0, 1);
  lcd.print("\177 \177 \177 \177 \177 \177 \177 \177 "); // Shift the arrows
  delay(300);


if(inTestMode) // open and close Test
{
 setColor(255, 0, 0);  // red
  delay(1000);
  setColor(0, 255, 0);  // green
  delay(1000);
  setColor(0, 0, 255);  // blue
  delay(1000);
  setColor(255, 255, 50);  // yellow
  delay(1000);  
 
openValve();
delay(2000);
closeValve();
inTestMode = false;
}

  if(ESP8266.available()) // check if the esp is sending a message 
  {
    if(ESP8266.find("+IPD,")) // If network data received from a single connection
    {
      delay(500); // wait for the serial buffer to fill up (read all the serial data)
      // get the connection id so that we can then disconnect
      int connectionId = ESP8266.read()-48; // subtract 48 because the read() function returns the ASCII decimal value
            
      ESP8266.find("pin="); // advance cursor to "pin="
      Serial.println("new client"); // Display info serial console
      int pinNumber = (ESP8266.read()-48); // get first number
      Serial.print("new pinNumber : "); // Display info serial console
      Serial.println(pinNumber, DEC);

      // If order 1 received: Open the valve
      if(pinNumber == 1)
      {      
        setColor(255, 255, 50); // Turn the Led Amber
        valveMoving = true;

        // Display info on LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Contamination!");
        lcd.setCursor(0, 1);
        lcd.print("Valve Opening");
        
        openValve();  // Open Valve

        // Display info on LCD
        lcd.print("Valve Open!  ");
      }

      // If order 2 received: Close the valve
      if(pinNumber == 2)
      {
        setColor(255, 255, 50); // Turn the Led Amber
        valveMoving = true;

        // Display info on LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("No Contamination");
        lcd.setCursor(0, 1);
        lcd.print("Valve closing");
        
        closeValve(); // Close Valve

        // Display info on LCD
        lcd.print("Valve close!");
      }     

      // If order 3 received: send data to the server
      if(pinNumber == 3)
      {
        ConnectToWebsite();  // Connect to the website
        SendData("Hello");  // Send data
      }      
    }
  }   
}

/* Function to initialise the ESP8266 */
void InitESP8266()
{  
  Serial.println("********************** INITIALISATION *********************"); 
  sendToESP8266("AT");
  receiveFromESP8266(2000);
  Serial.println("***********************************************************");
  sendToESP8266("AT+CWMODE=3"); //Wifi mode - softAP + station mode
  receiveFromESP8266(5000);
  Serial.println("***********************************************************");

  // Display info on LCD
  lcd.setCursor(0, 0);
  lcd.print("   CONNECTING   ");
  lcd.setCursor(0, 1);
  lcd.print("      WIFI      ");


  sendToESP8266("AT+CWJAP=\""+ wifiNetwork + "\",\"" + Password +"\""); //connect to wifi network
  receiveFromESP8266(15000);


  // Display info on LCD
  lcd.setCursor(0, 0);
  lcd.print("      WIFI      ");
  lcd.setCursor(0, 1);
  lcd.print("   CONNECTED!   ");
  
  Serial.println("***********************************************************");
  sendToESP8266("AT+CIFSR"); //Display the IPs adress (client + server)
  receiveFromESP8266(15000);
  Serial.println("***********************************************************");
  sendToESP8266("AT+CIPMUX=1");  //set multiple connections 
  receiveFromESP8266(5000);
  Serial.println("***********************************************************"); 
  /* configures the module as the server. It will then enable external clients to connect to the module.
  The port number is set for listening.*/
  sendToESP8266("AT+CIPSERVER=1,80");
  receiveFromESP8266(5000);
  Serial.println("******************* INITIALISATION DONE *******************");
}

/* Function to connect to the uni server */
void ConnectToWebsite()
{
  Serial.println(".................. CONNECTION TO SERVER ....................");
  sendToESP8266("AT+CIPSTART=1,\"TCP\",\"mi-linux.wlv.ac.uk\",80"); //connect to website
  receiveFromESP8266(10000);
  Serial.println("***************** CONNECTION TO SERVER: OK ****************");
}

// Function to send data by GET request
void SendData(String data)
{
  sendToESP8266("AT+CIPSEND=1,109"); //send message to connection 1, 109 bytes (87 bytes before char "?")
  receiveFromESP8266(10000);

  String httpreq = "GET /~1613741/valve.php?will=" + data + " HTTP/1.1";
  
  // Make a HTTP request:
  sendToESP8266(httpreq);
  receiveFromESP8266(10000);
  sendToESP8266("Host: mi-linux.wlv.ac.uk");
  sendToESP8266("Connection: keep-alive");
  sendToESP8266("");   
  receiveFromESP8266(10000);
  Serial.println("\r\n");
}

/* Function that send commands to the ESP8266 */
void sendToESP8266(String commande)
{  
  ESP8266.println(commande);
}

/*Function that read and display messages sent by ESP8266 (with timeout)*/
void receiveFromESP8266(const int timeout)
{
  String reponse = "";
  long int time = millis();
  while( (time+timeout) > millis())
  {
    while(ESP8266.available())
    {
      char c = ESP8266.read();
      reponse+=c;
    }
  }
  Serial.print(reponse);   
}






// A function to check if the valve has hit the limit switches, if not the stepper is moved forward once, if  limit swicth reached stepper is stopped.




boolean loopSteps(int numberOfSteps,boolean opening)
{
  int flashCount = 0;
   setColor(0, 0, 0);
   Serial.println("Closing");
       
  for(int i  = 0;i>numberOfSteps;i++) // loops thro steps
    {
      if (opening) // if valve is opening move CW
        {
      
           Serial.println("Opening 1");
         if(digitalRead(openSensorPin)) // checks open limit switch
           {
           if(flashCount==flashEveryAmount)
          {
          if (ledState==0)
          {
            ledState = 1;
            
          }
          else
          {
            ledState = 0;
          }
          flashCount=0;
          }
         
            if (ledState==0)
          {
          setColor(255, 255, 50); // Turn the Led Amber
          }
          else
          {
             setColor(0, 0, 0);
          }
     flashCount++;
        
            
            //  Serial.println("Closing 2");
            small_stepper.step(1);
           }
         else
           {
           currentValveState = STATE_VALVE_OPEN;
           return false;
          }
       }
     else   // if valve is closed move CCW
       {
         Serial.println("Closing 1");

         
        if(digitalRead(closeSensorPin)) // checks closed limit switch
         {
            if(flashCount==flashEveryAmount)
          {
          if (ledState==0)
          {
            ledState = 1;
            
          }
          else
          {
            ledState = 0;
          }
          flashCount=0;
          }
         
            if (ledState==0)
          {
          setColor(255, 255, 50); // Turn the Led Amber
          }
          else
          {
             setColor(0, 0, 0);
          }
     flashCount++;

           
           Serial.println("Closing 2");
           small_stepper.step(-1);
         }
        else
         {
          currentValveState = STATE_VALVE_CLOSED;
          return false;
        }
    }
  }
}

// Function to open the valve
void openValve(){
  for (int i = 0; i<8; i++){
    setColor(0, 0, 0); // Turn Off the Led
    Steps2Take  =  STEPS_PER_OUTPUT_REVOLUTION ;  // Rotate CW 1 turn
    small_stepper.setSpeed(1000);   
    if(!loopSteps(Steps2Take, true)){break;} // runs funtion to move stepper motor step bu step so that limit switch can be used - saves over run
    //delay(10);
   
    Steps2Take  =  STEPS_PER_OUTPUT_REVOLUTION;  // Rotate CW 1 turn  
    small_stepper.setSpeed(1000);  // 700 a good max speed??
    if(!loopSteps(Steps2Take, true)){break;}
  }  
  
  setColor(0, 255, 0); // Turn the Led Green
}

// Function to close the valve
void closeValve(){
  for (int i = 0; i<8; i++){
    setColor(0, 0, 0); // Turn Off the Led
    Steps2Take  =  STEPS_PER_OUTPUT_REVOLUTION ;  // Rotate CCW 1 turn
    small_stepper.setSpeed(1000);   
    if(!loopSteps(Steps2Take, false)){break;} // runs funtion to move stepper motor step bu step so that limit switch can be used - saves over run
    //delay(10);
    
    Steps2Take  =  STEPS_PER_OUTPUT_REVOLUTION;  // Rotate CCW 1 turn  
    small_stepper.setSpeed(1000);  
    if(!loopSteps(Steps2Take, false)){break;}
  }
  
  setColor(255, 0, 0); // Led turn red
}




// Function to set the RGB Led color
void setColor(int red, int green, int blue)
{
  #ifdef COMMON_ANODE
    red = 255 - red;
    green = 255 - green;
    blue = 255 - blue;
  #endif


  
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);  
}

// Function to scroll text on LCD (ex. of use: scrollInFromRight(1, "Line2 From Right");
void scrollInFromRight (int line, char str1[]) {

  int i = strlen(str1);

  for (int j = 16; j >= 0; j--) {
    lcd.setCursor(0, line);
      
    for (int k = 0; k <= 15; k++) {
      lcd.print(" "); // Clear line
    }

    lcd.setCursor(j, line);
    lcd.print(str1);
    delay(delayTime2);
  }
}



// function to check open and closed sensors to get current state of valve (Open or Closed)
int checkValveState()
{
  boolean openCheck = false;
  
  if (!digitalRead(openSensorPin))
  {
   return STATE_VALVE_OPEN;
  }
  
  if (!digitalRead(closeSensorPin))
  {
    return STATE_VALVE_CLOSED;
  }
  else
  {
    return STATE_VALVE_HALF;
  }
  
}


