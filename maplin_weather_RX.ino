/*
sketch : maplin_weather_RX
Author : Peter Matthews (CurlyWurly)   May 2017
Desc   : Decodes the 433 Mhz signals sent from a Maplin Weather station N25FR 

INFORMATION
  N.B. THIS VERSION IS TRIGGERED WHEN A "HIGH" STATE EXCEEDS "minStartHi"
       i.e. THIS VERSION IS TUNED TO TRAP THE FIRST MESSAGE BECAUSE THE MAPLIN "N25FR" TRANSMITTER
            ALTERNATES BETWEEN SENDING ONE AND THEN A PAIR OF MESSAGES SEPARATED BY 48 SECONDS 
            IN THE 48 SECONDS, THE 433 Mhz RECIEVER THEN RAMPS UP THE AGC TRYING TO FIND A SIGNAL 
            AND ENDS UP AMPYFYING BACKGROUND NOIDE(UNTIL RANDOM NOISE IS SEEN AGAIN)  
  N.B. Connect pin 2 (rxpin) to the RX module output
  N.B. This sniffer sketch deciphers the following from the outside sensor:
         - Outside Humidity    in %RH
         - Outside Temperature in C 
         - Outside Wind(Gust) in mph (since last reading)
         - Outside Wind(Avg)  in mph (since last reading)
         - Outside Rainfall Total - since batteries were inserted into the sensor
  N.B. Be aware that the rainfall measurement value is the total volume since sensor switch-on. 
       i.e. It is the receiver that does all the time sliced grouping (e.g.how much per hour/day/month)  
  N.B. Be aware that the receiving station handles the following internally (i.e. the below are not seen in 433 Mhz comms)
         - Time
         - Inside Humidity
         - Inside Temperature
         - Barometric Pressure
  N.B. The sensor I bought (May 2017) does the following on the 433 comms  
        1 - sends one message only (79 pulses) 
        2 - waits 48 seconds
        3 - sends 2 identical messages with a brief pause inbetween. (both messages are supposed to be 79 pulses) 
        4 - waits 48 seconds and then repeats the cycle
  N.B. Information on the web says the initial preamble is supposed to be 8 "spike" Hi pulses. However, I found 
       that my Maplin sensor only sent 7 "spike" Hi pulses, with the "Low" phase of the first pulse regularly 
       contaminated with a tiny 3us "Hi" spike. 
  N.B. This sketch is designed to tolerate a varying number of preamble "Hi" pulses - anything from 4 to 8 "Hi" pulses is OK 
       This is done to cater for the problem where initial preamble spikes are missed as desribed above (perhaps because of noise?).
       If preamble spikes are missed, the total number of pulses will less than they should (e.g. 77 or 78). So, to make 
       this sketch more reliable and flexible, an "adj" adjustment value is applied to the array index with a value calculated 
       by looking at the total number of pulses, By doing this, this sketch always looks at the correct "bits" of the message 
       to construct the values e.g. Temperature and Humidity, even if up to 4 preamble spikes are missed!!!. 
  N.B. This sketch does not use the CRC to check the validity of the message - this could be done in a future version     
*/

#define rxPin            2   // Input from receiver
#define maxState       200   // Maximum States (Halve for pulses, will detect "pulse trains" up to 100 pulses long)
#define minStartHi     300   // Min start Hi pulse   
#define minBreakLow  10000   // Min break Low pause   

bool          trig;               // recording started
bool          pinstate;           // State of input pin - Hi(1) or Low(0)
int           stateCount;         // State change counter
int           pulses;             // Number of pulses (half of stateCount) 
int           i;                  // Array counter
int           adj;                // Adjustment for preamble 
int           lastTime;           // 
double        temp;               // Outside Temperature
int           hum;                // Outside Humidity 
double        windGust;           // Wind Gust 
double        windAvg;            // Wind Avg 
double        rainfall;           // Rainfall 

unsigned long T_array[maxState];  // Array of HiLo pulse lengths (starts with Hi)
bool          B_array[(maxState/2)];  // Array of HiLo pulse lengths (starts with Hi)
unsigned long minlengthHI;        // Minimum Hi pulse length in us
unsigned long minlengthLOW;       // Minimum Lo pulse length in us
unsigned long pulseLen;           // pulse length in us
unsigned long startTime;          // Start Time of pulse in us
unsigned long endTime;            // End   Time of pulse in us
unsigned long unitlen;            // lowest AVG pulse length in us (assumed L.C.Denominator)

//*********************************************************************************************
void setup() {
//*********************************************************************************************
  pinMode(rxPin, INPUT);      // initialize input pin used for input signal
  Serial.begin(115200);       // Set up Serial baud rate 
  Serial.println("Scanning for 433Mhz data from a Maplin N25FR Weather Station");
  startTime     = micros();   // Store first "Start time"
  pinstate      = false;      // prepare stored version of pin state to start as "low"
  trig          = false;      // initialise 
}

//*********************************************************************************************
void loop() {
//*********************************************************************************************
// If a state change, then calculate pulselen
  if (pinstate != digitalRead(rxPin)) {
    endTime    = micros();                // Record the end time of the read period.
    pulseLen   = endTime - startTime;     // Calculate Pulse length in us
    startTime  = endTime;                 // Remember start time for next state change
    pinstate   = !pinstate;

// If  previous pulse Length is less than 300 Us or greater than 2 second, reset 
    if ( ( pulseLen < 100 ) || ( pulseLen > 2000000 ) ) {
      trig          = false; 
    } else {
      if ( ( trig == false ) &&  ( pulseLen > minStartHi)  &&  ( pinstate == false ) ) {
        trig          = true;       // initialise 
        stateCount    = 0;          // initialise 
      };
    };

// Return if not triggered 
    if ( trig == false ) { 
      return;      
    };

// Store previous state time length (N.B. even indexes are for high, odd index is for low)
    T_array[stateCount] = pulseLen;            // Store Pulse Length 

// If previous state time length is less than minStartLow, then store pulse length
// and return to keep recording state 
    if ( ( pulseLen < minBreakLow ) && ( stateCount < maxState ) ) {
      stateCount++;    
      return;
    }

//********************************************************************************
// If you are here, then we have reached a pause *should be a Low state), so 
// interpret the array data to decipher the "code" to serial before trying again      
//********************************************************************************


// Calculate Min lengths
    minlengthHI   = 9999;
    minlengthLOW  = 9999;
// Don;t count last low!!
    for(i=0; i<stateCount; i=i+1){
      if ( ( i % 2) == 0 ) {
        if (T_array[i] < minlengthHI)  { minlengthHI  = T_array[i]; }
      } else {
        if (T_array[i] < minlengthLOW) { minlengthLOW = T_array[i]; };
      };
    };

// Calculate Avg unit length of pulses
    unitlen = 0;
    if ( minlengthLOW < ( minlengthHI  / 2 )) {
      unitlen = minlengthLOW ;
    } else {
      unitlen = ( ( minlengthHI + minlengthLOW ) / 2 );
    };    


// Interpret message to Binary string
// This is determined by the length of the "High" state (Even indexes)
//    if long,  then this is a "0"
//    if short, then this is a "1"
// N.B.   B_array[] will always have half as many indexes as T_array[] 
    for(i=0; i<=stateCount; i=i+2){
        if ( ( (T_array[i] * 10) + (5 * minlengthHI)  ) / (10 * minlengthHI) == 1 ) {
          B_array[(i/2)] = 1;
        } else {
          B_array[(i/2)] = 0;
      };
    };
//    Serial.println(" ");

// Calculate pulses
    pulses = ( (stateCount + 1) / 2);

// Check if a valid message ( unitlen = 660, pulses 77 - 80 ) 
    if (    (unitlen <  600)
         || (unitlen >  700) 
         || ( (pulses < 76) || (pulses > 80) ) 
         || ( ( B_array[1] != 1) || ( B_array[2] != 1) ) ) {
      trig = false;
      return;
    };

    adj = 0;
    if ( pulses == 80) {
      adj = 0;
    }
    if ( pulses == 79) {
      adj = 1;
    }
    if ( pulses == 78) {
      adj = 2;
    }
    if ( pulses == 77) {
      adj = 3;
    }
    if ( pulses == 76) {
      adj = 4;
    }

// Output deciphered data as HnLn
//    Serial.print("Pulses = ");
//    Serial.print( pulses );
//    Serial.print(",   Unit length = ");
//    Serial.print(unitlen);
//    Serial.print(",   Code = ");
//    for(i=0; i<=stateCount; i=i+1){
//      if ( ( i  % 2) == 0 ) {
//        Serial.print("H");
//        Serial.print( ( (T_array[i] * 10) + (5 * unitlen)  ) / (10 * unitlen) );
//      } else {
//        Serial.print("L");
//        Serial.print( ( (T_array[i] * 10) + (5 * unitlen)  ) / (10 * unitlen) );
//        Serial.print(", ");
//      };
//    };
//    Serial.println(" ");
     
//    for(i=0; i<=pulses; i=i+1){
//      Serial.print(B_array[i]);
//    };
//    Serial.println(" ");    

// Outside Humidity
    hum   =  (B_array[32-adj] * 128  ) + (B_array[33-adj] * 64   ) + (B_array[34-adj] * 32  ) + (B_array[35-adj] * 16  )
           + (B_array[36-adj] * 8    ) + (B_array[37-adj] * 4    ) + (B_array[38-adj] * 2   ) + (B_array[39-adj] * 1   ) ;
    Serial.print("Humidity = ");
    Serial.print(hum);          

// Outside Temperatue
    temp  =    ( ( (B_array[20-adj] * 2048 ) + (B_array[21-adj] * 1024 ) + (B_array[22-adj] * 512 ) + (B_array[23-adj] * 256 )
                 + (B_array[24-adj] * 128  ) + (B_array[25-adj] * 64   ) + (B_array[26-adj] * 32  ) + (B_array[27-adj] * 16  )
                 + (B_array[28-adj] * 8    ) + (B_array[29-adj] * 4    ) + (B_array[30-adj] * 2   ) + (B_array[31-adj] * 1   ) ) - 400 );
    temp = temp / 10;
    Serial.print(",  Temp(C) = ");
    Serial.print(temp,1); 

// Outside Wind(Avg)
    windAvg =     (B_array[40-adj] * 128  ) + (B_array[41-adj] * 64   ) + (B_array[42-adj] * 32  ) + (B_array[43-adj] * 16  )
                + (B_array[44-adj] * 8    ) + (B_array[45-adj] * 4    ) + (B_array[46-adj] * 2   ) + (B_array[47-adj] * 1   ) ;
    windAvg = round(windAvg * 3.4);
    windAvg = round(windAvg * 2.237);
    windAvg = windAvg/10;
    Serial.print(",  Wind(Avg mph) = ");
    Serial.print(windAvg,1);  

// Outside Wind(Gust)
    windGust =    (B_array[48-adj] * 128  ) + (B_array[49-adj] * 64   ) + (B_array[50-adj] * 32  ) + (B_array[51-adj] * 16  )
                + (B_array[52-adj] * 8    ) + (B_array[53-adj] * 4    ) + (B_array[54-adj] * 2   ) + (B_array[55-adj] * 1   ) ;
    windGust = round(windGust * 3.4);
    windGust = round(windGust * 2.237);
    windGust = windGust/10;
    Serial.print(",  Wind(Gust mph) = ");
    Serial.print(windGust,1);  

// Outside Rainfall
    rainfall =    (B_array[64-adj] * 128  ) + (B_array[65-adj] * 64   ) + (B_array[66-adj] * 32  ) + (B_array[67-adj] * 16  )
                + (B_array[68-adj] * 8    ) + (B_array[69-adj] * 4    ) + (B_array[70-adj] * 2   ) + (B_array[71-adj] * 1   ) ;
    rainfall = rainfall * 0.3;
    Serial.print(",  Total Rainfall(mm)= ");
    Serial.print(rainfall,1);   

// Elapsed Time
    if ( lastTime != 0 ) {
      Serial.print(",  Elapsed Time(s)= ");
      lastTime = round(millis()/1000 - lastTime);
      Serial.println(lastTime); 
    } else {
      Serial.println(" "); 
    };
    lastTime = millis()/1000;
    trig     = false;       // initialise 
  };
}
   
