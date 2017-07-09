/*
 * added delay to setup function to allow wifi to boot.  11/3/2016
 * 
Both Submit and calculations processes moved to functions
Bridge is setup and device webpage is working
Make sure that you have created arduino/www on your SD card and that you have the www folder in the same folder as the sketch with the files in it.

You can access the data at:
http://arduino.local/sd/WebPanel/ (replace arduino.local with the ip address of your unit.)
*/


#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Process.h>
#include <Console.h>
#include <FileIO.h>
#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>

// Listen on default port 5555, the webserver on the Yún
// will forward there all the HTTP requests for us.
BridgeServer server;
String startString;
long hits = 0;

#define DHTPIN 4   
//#define DHTTYPE DHT11 
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//Defining ports for inputs
  int sensor1Port = A0; //Thermometer
  int sensor2Port = A1; //Anemometer
  int sensor3Port = A2; //Pyranometer
  int sensor4Port = A3; //Humidity
  int sensor5Port = A4; //Barometer


// Sensor Slope Intercepts
  float slopeSensor2 = 22.37;       //Anemometer
  float interceptSensor2 = -22.37;
  float slopeSensor3 = 250;         //Pyranometer
  float interceptSensor3 = 0; 
  float slopeSensor4 = 30.43;       //Humidity
  float interceptSensor4 = -25.81;
  float slopeSensor5 = 6.825;       //Barometer
  float interceptSensor5 = 76.29375;



//Define variables for ports
  float t = 0;
  float h = 0;
  float sensor1Value = 0;
  float sensor2Value = 0;
  float sensor3Value = 0;
  float sensor4Value = 0;
  float sensor5Value = 0;
  String dataString;
  String displayString;
  String lastreport;

void setup() {
  delay(60000);  // Give time for wifi to boot
   Bridge.begin();  // Initialize Bridge
   Console.begin(); 
   FileSystem.begin(); //Initialize FileSystem
   SerialUSB.begin(9600);
   BridgeServer server;
      String startString;
      long hits = 0;
   Console.println("You're connected to the Console!!!!"); 
   Serial.begin(115200);
      
// initialize digital pin 13 as an output (light to show start of process).
     pinMode(13, OUTPUT);
  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();

  // get the time that this sketch started:
  startString = (getTimeStamp());
}

void loop() {

//Calculate and report the data to mohawk
    dataString = (" ");
    calc();  //Send to function to make calculations
    submit();  //  Submit to SD and to Mohawk

// Clear String
    displayString = (" ");
    
   int i=0; //clear incrimenter
   while ( i <= 250){
      Console.println(i);
      // Get clients coming from server
        BridgeClient client = server.accept();
      
        // There is a new client?
        if (client) {
          // read the command
          String command = client.readString();
          command.trim();        //kill whitespace
          SerialUSB.println(command);
          // is "temperature" command?
          if (command == "temperature") {
      
            calc();  //Send to function to make calculations
            client.print("<br>Current datastring: ");
            client.print(displayString);
            client.print("<br>");
            client.print("<br>");
            client.print("<br>");
            client.print("Last Report TimeStamp:   ");
            client.print(lastreport);
            client.print("<br>");
            client.print("<br>");            
            client.print("<br>Hits so far: ");
            client.print(hits);
            Console.println(displayString);
            }
            else {
              Console.println(displayString);
              delay(2000);
            }
          // Close connection and free resources.
          client.stop();
          displayString = ("");
          
          hits++;
          }
          else {
              Console.println(displayString);
              delay(2000);
            }
        
   delay(500); // Poll for a http connection every 50ms
   i++;
   } 
}

// This function return a string with the time stamp
String getTimeStamp() {
  String result;
  Process time;
  // date is a command line utility to get the date and the time 
  // in different formats depending on the additional parameter 
  time.begin("date");
  time.addParameter("+%D-%T");  // parameters: D for the complete date mm/dd/yy
                                //             T for the time hh:mm:ss    
  time.run();  // run the command

  // read the output of the command
  while(time.available()>0) {
    char c = time.read();
    if(c != '\n')
      result += c;
  }

  return result;
}

//Function to calculate the temperture of port 1
float Thermistor(int Raw) //This function calculates temperature from ADC count
{
 long Resistance; 
 float Resistor = 15000; //fixed resistor
  float Temp;  // Dual-Purpose variable to save space.
  Resistance=( Resistor*Raw /(1024-Raw)); 
  Temp = log(Resistance); // Saving the Log(resistance) so not to calculate  it 4 times later
  Temp = 1 / (0.00102119 + (0.000222468 * Temp) + (0.000000133342 * Temp * Temp * Temp));
  Temp = Temp - 273.15;                             // Convert Kelvin to Celsius                      
  return Temp;                                      // Return the Temperature
}

//This function will calculate the calibrated Sensor Value
float sensorValueCalculate(float slope, float intercept, float RawData) {
  float Voltage = RawData / 1023 * 5.0; 
  float CalibratedReading = intercept + (Voltage * slope);
  return CalibratedReading;  
}

float submit()
{
   //Write data to CSV
    File dataFile = FileSystem.open("/mnt/sda1/data.csv", FILE_APPEND);
        dataFile.println(dataString);
        dataFile.close();
        Console.println("Data written to SD Card");
    
    //Light to show start of database writing process  
      digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level) to show start


    // Last reported in
    lastreport = getTimeStamp();
    
    // Send data to Mohawk
        Console.println("Sending data to Mohawk");
           Process p;              
           p.begin("/mnt/sda1/db.php");
           p.addParameter(String(t));
           p.addParameter(String(h)); 
           p.addParameter(String(sensor1Value));
           p.addParameter(String(sensor2Value)); 
           p.addParameter(String(sensor3Value));
           p.addParameter(String(sensor4Value)); 
           p.addParameter(String(sensor5Value));
           p.run();
           Console.println(dataString);
           Console.println("Finished sending data to Mohawk");
        
          digitalWrite(13, LOW);   // turn the LED on (HIGH is the voltage level) to show start
  
}


float calc()
{
//Get the date and time to attach to the csv on the micro sd card  
  dataString += getTimeStamp();
  displayString += getTimeStamp();

//DHT22 readings
    h = dht.readHumidity();
    t = dht.readTemperature();

//Start Calculations

    //Getting reading from temp wand in port 1
      float data; //reading from the A/D converter (10-bit)
      float Temp; //the print below does the division first to avoid overflows
      data=analogRead(sensor1Port);       // read count from the A/D converter 
      sensor1Value=Thermistor(data);       // and  convert it to CelsiusSerial.print(Time/1000); //display in seconds, not milliseconds                       
    
    //Passing raw values to calculating process
        data = analogRead(sensor2Port);
          sensor2Value = sensorValueCalculate(slopeSensor2, interceptSensor2, data); 
            sensor2Value = sensor2Value - 1.14;                                       // Adjust to 0mph.
        data = analogRead(sensor3Port);
          sensor3Value = sensorValueCalculate(slopeSensor3, interceptSensor3, data);
        data = analogRead(sensor4Port);
          sensor4Value = sensorValueCalculate(slopeSensor4, interceptSensor4, data);
        data = analogRead(sensor5Port);
          sensor5Value = sensorValueCalculate(slopeSensor5, interceptSensor5, data);
            sensor5Value  = sensor5Value * .295;                                      //  Convert from kPa to Bars


//Create Datastring to show in console
      dataString = (String(t) + "," + String(h) + "," + String(sensor1Value) + "," + String(sensor2Value) + "," + String(sensor3Value) + "," + String(sensor4Value) + "," + String(sensor5Value));
      displayString += ("Int Temp->" + String(t) + "Int Humid->" + String(h) + "Temp->" + String(sensor1Value) + "Wind->" + String(sensor2Value) + "Light->" + String(sensor3Value) + "Hum->" + String(sensor4Value) + "Bar->" + String(sensor5Value));
//      Console.println(displayString);
//      Console.println(dataString);
//      dataString = (" ");
}

