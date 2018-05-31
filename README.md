# Net server for Oberon workstations

This is an RPI based server application, connecting through *nRF24L01* wireless module. It requires specific wiring for use with GPIO pin connector port on RPI side. Each FPGA Oberon station needs *nRF24L01* plugged in.
	| PIN | NRF24L01 | RPI |   GPIO      |
	|-----|----------|-----|-------------|
	|  1  |   GND    |  6  | GND         |
	|  2  |   VCC    |  1  | 3.3V        |
	|  3  |   CE     | 22  | (25)        |
	|  4  |   CSN    | 24  | (8)         |
	|  5  |   SCK    | 23  | SPI_CLK(11) |
	|  6  |   MOSI   | 19  | SPI_MOSI(10)|
	|  7  |   MISO   | 21  | SPI_MISO(9) |
	|  8  |   IRQ    |  -  |  -          |

![WiFi](RPI-wifi.jpg?raw=true "RPI-wifi")

**How to build**
	$ sudo make install

**How to run**
	$ cd <to-your-prefered-dir>
	$ net

**Prerequisite**
	Build modified RF24+ lib, from its own repository here

# Oberon station Toolbox
	Net.StartServer  Net.Unprotect  Net.WProtect
	Net.StopServer
	Net.SendMsg RPI hello there ~
	Net.Reset
	Net.SendFiles RPI My.Mod ~
	Net.ReceiveFiles RPI Your.Mod ~
	Net.GetTime RPI ~

RPI Net server broadcasts time update every 16 seconds, to receive time updates start & unprotect the Oberon Net server

# Notes
The connection was tested with RPI model B(-). It required some changes in timimg. The short range works best with low or minimum radio power. Due to some noise, retry count was raised, to allow for more retrasmits.

**Module SCC**
	CONST Wait = 50; Wait0 = 5; Wait1 = 1000;

	PROCEDURE SubSnd (*use Wait0*)
	PROCEDURE SendPacket (*use Wait*)
	PROCEDURE ReceiveHead (*use Wait1*)

	PROCEDURE Start (*added*)
	WriteReg1(4, 0*11H + 15); (*SETUP_RETR delay + retry count*)
	WriteReg1(6, 03H); (*RF_SETUP <= 1Mb rate, 0dBm wide range, -12dBm short rea
