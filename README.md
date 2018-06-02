# Net server for Oberon workstations

This is an RPI based server application, connecting through *nRF24L01* wireless module. It requires specific wiring for use with P1 GPIO connector port on RPI side. Each FPGA Oberon station needs *nRF24L01* plugged in. It is basically `SCC.Mod`,`Net.Mod` recoded in cpp, and it appears as another Oberon station to the user.

	| PIN | NRF24L01 | RPI P1 |   GPIO      |
	|-----|----------|--------|-------------|
	|  1  |   GND    |    6   | GND         |
	|  2  |   VCC    |    1   | 3.3V        |
	|  3  |   CE     |   22   | (25)        |
	|  4  |   CSN    |   24   | CS0(8)      |
	|  5  |   SCK    |   23   | SPI_CLK(11) |
	|  6  |   MOSI   |   19   | SPI_MOSI(10)|
	|  7  |   MISO   |   21   | SPI_MISO(9) |
	|  8  |   IRQ    |    -   |  -          |

![WiFi](RPI-wifi.jpg?raw=true "RPI-wifi")

**How to build**

	$ make
	$ sudo make install

**How to run**

	$ cd <to-your-preferred-dir>
	$ net

**Prerequisite**

	Modified RF24+ lib build, from its own repository here

**Oberon Net Toolbox**

	Net.StartServer  Net.Unprotect  Net.WProtect
	Net.StopServer
	Net.SendMsg RPI hello there ~
	Net.Reset
	Net.SendFiles RPI My.Mod ~
	Net.ReceiveFiles RPI Your.Mod ~
	Net.GetTime RPI ~

RPI Net server broadcasts time update every 16 seconds, to receive time updates start & unprotect the Oberon Net server

# Notes
The connection was tested with RPI model B(-). It required some changes in timimg. The short range works best with low or minimum radio power. Due to some noise, retry count was raised, to allow for more retransmits.

**Module SCC**

	CONST Wait = 50; Wait0 = 5; Wait1 = 1000;

	PROCEDURE SubSnd (*use Wait0*)
	PROCEDURE SendPacket (*use Wait*)
	PROCEDURE ReceiveHead (*use Wait1*)

	PROCEDURE Start (*added*)
	WriteReg1(4, 0*10H + 15); (*SETUP_RETR delay + retry count*)
	WriteReg1(6, 03H); (*RF_SETUP <= 1Mb rate, 0dBm wide range, -12dBm short reach*)

**Module Net**

On ocassions file transfer may fail, *Net.Reset* is the remedy, also proved useful to add *partner[0] := 0X* to it, and move *reply(0)* right after *ReceiveData()* in *PROCEDURE ReceiveFiles*.

	PROCEDURE Reset (*added*)
	*partner[0] := 0X*

	PROCEDURE ReceiveFiles (*moved*)
	ReceiveData(F, done); *reply(0);*

For multifile transfer extra ACK packets added after *SendData()*

	PROCEDURE SendFiles (*added*)
	SendData(F); *Send(ACK, 0, dmy);*
	
	PROCEDURE Serve (*added*)
	Texts.Append(Oberon.Log, W.buf); SendData(F); *Send(ACK, 0, dmy);*

