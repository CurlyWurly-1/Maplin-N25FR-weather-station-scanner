# Maplin-N25FR-weather-station-scanner

Use this sketch on an Arduino Uno with a cheap 433MHZ receiver to read the data coming from a Maplin weather station N25FR

- MESSAGE PROTOCOL

The RF comms message seems to be 79 pulses long and split into 20 nibbles as follows:

N.B. the first nibble only has 3 pulses - this why you see 79 pulses and not 80

1  111   - Preamble

2  1111  - Preamble

3  0101  - ID

4  1010  - ID

5  1111  - ID

6  0010  - Temp      (MSB)

7  0000  - Temp

8  0101  - Temp

9  0100  - Hum       (MSB)

10 1011  - Hum

11 0000  - Avg Wind  (MSB)

12 0011  - Avg Wind

13 0000  - Gust Wind (MSB)

14 0100  - Gust Wind

15 0000  - Unknown

16 0010  - Rain Hi

17 0110  - Rain      (MSB)

18 0100  - Rain

19 1011  - Checksum

20 1110  - Checksum

- HOW TO CONVERT THE BINARY VALUES

Humidity - read directly from bits e.g. 00100001 = 33 %RH

Temperature in C - Use the 12 bits (from above map), subtract 400 (0x190) and divide the decimal by 10. e.g. a reading of 0x2oe minus 0x190 = 0x7e, which is 126. This represents 12.6 degrees C.

Wind speed in m/s - Multiply 8 bits by 0.34 and round the result to one decimal place. So, value 0x04 is round(4 * 0.34) = 1.4 m/s. If you want mph instead of m/s, use another multiplication of 2.237 on the m/s result and round to one decimal place.

Rainfall in mm - Multiply the 8 bits (should it be 12?) by 0.3 to get the total rainfall since the since the sensor was switched on. To get the "real" rainfall value - you have to work out the delta from a previously transmitted rainfall value with respect to the time between the two messages

- USEFUL LINKS

http://www.susa.net/wordpress/2012/08/raspberry-pi-reading-wh1081-weather-sensors-using-an-rfm01-and-rfm12b/
https://github.com/cawhitworth/weatherduino
