# Maplin-N25FR-weather-station-scanner

Use this sketch on an Arduino Uno with a cheap 433MHZ receiver to read the data coming from a Maplin weather station N25FR

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

