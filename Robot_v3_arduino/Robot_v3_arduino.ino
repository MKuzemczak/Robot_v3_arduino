/*
 Name:		Robot_v3_arduino.ino
 Author:	Michał Kuzemczak
*/

#include <StandardCplusplus.h>
#include <vector>
#include <Adafruit_PWMServoDriver.h>

//#define DEBUG
//#define DEBUG1


Adafruit_PWMServoDriver driver = Adafruit_PWMServoDriver();

#define SRV0  0
#define SRV1  1
#define SRV2  2
#define SRV3  3
#define SRV4  4
#define SRV5  5

#define LISTSIZE 50

#define FLAG_OFFSET									'A'
#define FLAG_CONNECT								FLAG_OFFSET+0x0C
#define FLAG_MOV_FIN								FLAG_OFFSET+0x01
#define FLAG_RCV_CONF								FLAG_OFFSET+0x01
#define FLAG_RCV_CONF_FIN						FLAG_OFFSET+0x02
#define FLAG_RCV_SINGLE_NUM					FLAG_OFFSET+0x03
#define FLAG_RCV_SINGLE_SIGNAL			FLAG_OFFSET+0x04
#define FLAG_RCV_SPEED							FLAG_OFFSET+0x05
#define FLAG_END_OF_NUM		'\n'


std::vector<int> received,			// servo configuration received through UART, waiting to be set
currentlySet(16);	// current configuration of servos

char val; // byte received through UART

String liczba;	// string in which received bytes are saved, until '\n' occurs

bool _receivingConfiguration = false, // flag signalising that received strings should be saved to 'received' vector.
_receivingDOF = false,
_allOff = false,
_receivingSingleNumber = false,
_receivingSingleSignal = false,
_receivingSpeed = false;

int biggestChange; // contains the value of the biggest change in servo signals, need it to slowly set all the servos
int singleServoCurrent,
singleServoNumber;
int speed = 1;

void start();																		// this function sets all servos to initial position
void establishContact();												// this function establishes contact with stm32
void roznica();																	// finds the biggest change among received servo signals
void ustaw();																		// slowly moves all servos to new positions
void setSingleServo(int servo, int signal);			// slowly sets one chosen servo
void checkForTurnOffs();												// checks received data for 4096 - turn off signal and turns those servos off
void signal();
bool equal(std::vector<int> v0, std::vector<int> v1);

// the setup function runs once when you press reset or power the board
void setup()
{
	Serial.begin(115200); // otwarcie portu szeregowego UART

#ifndef DEBUG
	driver.begin();
	driver.setPWMFreq(60);
#endif // !DEBUG


	yield();

	pinMode(LED_BUILTIN, OUTPUT);

	start();
	establishContact(); // ustanowienie kontaktu


	singleServoCurrent = 300;
}


// the loop function runs over and over again until power down or reset
void loop()
{
	while (Serial.available() > 0)
	{
		val = Serial.read(); // czytanie kazdego znaku ASCII po kolei

												 //Serial.print("val == ");
												 //Serial.println(val);

		if (isDigit(val) || (val == '.') || (val == '-'))
		{
			liczba += (char)val;
		}
		else if (val == FLAG_RCV_CONF)
		{
			_receivingConfiguration = true;
			received.clear();
		}
		else if (val == FLAG_RCV_CONF_FIN)
		{
			_receivingConfiguration = false;

			checkForTurnOffs();

			if (!equal(received, currentlySet))
			{
				if (!_allOff)
					roznica();

				ustaw();
			}
			else
			{
				Serial.write(FLAG_MOV_FIN);
			}
		}
		else if (val == FLAG_RCV_SINGLE_NUM)
		{
			_receivingSingleNumber = true;
		}
		else if (val == FLAG_RCV_SINGLE_SIGNAL)
		{
			_receivingSingleSignal = true;
		}
		else if (val == FLAG_RCV_SPEED)
		{
			_receivingSpeed = true;
		}
		else if (val == FLAG_END_OF_NUM)
		{
			if (_receivingConfiguration)
			{
				received.push_back(liczba.toInt());
			}
			else if (_receivingSingleNumber)
			{
				singleServoNumber = liczba.toInt();
				_receivingSingleNumber = false;
			}
			else if (_receivingSingleSignal)
			{
				setSingleServo(singleServoNumber, liczba.toInt());
				_receivingSingleSignal = false;
			}
			else if (_receivingSpeed)
			{
				speed = liczba.toInt();
				_receivingSpeed = false;
			}

			liczba = "";

#ifdef DEBUG
			Serial.println("received.push_back(liczba.toInt());");
#endif // DEBUG

		}

	}
}


void start()
{
	for (int i = 0; i < 16; i++)
	{
#ifndef DEBUG
		driver.setPWM(i, 0, 4096);
#endif // !DEBUG

		currentlySet[i] = 4096;

#ifdef DEBUG
		Serial.print(4096);
		Serial.print(" ");
#endif // DEBUG

	}
#ifdef DEBUG
	Serial.println("");
#endif // DEBUG



	_allOff = true;
}


void establishContact()
{
	while (Serial.available() <= 0)
	{
		Serial.write(FLAG_CONNECT);
		delay(300);
	}
	signal();
}

// dla kazdego wiersza okreslenie najwiekszej roznicy w stosunku do poprzedniego wiersza
void roznica()
{
#ifdef DEBUG
	Serial.println("roznica");
#endif // DEBUG

	biggestChange = std::max(abs(received[0] - currentlySet[0]), abs(received[1] - currentlySet[1]));
	for (int i = 2; i < received.size(); i++)
	{
		biggestChange = std::max(biggestChange, abs(received[i] - currentlySet[i]));
	}

#ifdef DEBUG
	Serial.print("biggestChange == ");
	Serial.println(biggestChange);
#endif // DEBUG

}

// ustawia serwa od punktu do punktu, lagodnie
void ustaw()
{
	if (_allOff)
	{
		//Serial.println("ustaw, allOff");

		for (int i = 0; i < received.size(); i++)
		{
#ifndef DEBUG
			driver.setPWM(i, 0, received[i]);
#endif // !DEBUG

#ifdef DEBUG
			Serial.print(received[i]);
			Serial.print(" ");
#endif // DEBUG

		}
#ifdef DEBUG
		Serial.println("");
#endif // DEBUG

		_allOff = false;
	}
	else
	{
#ifdef DEBUG
		Serial.println("ustaw");
#endif // DEBUG

		for (int i = 0; i <= biggestChange; i++)
		{
			for (int j = 0; j < received.size(); j++)
			{
#ifndef DEBUG
				driver.setPWM(j, 0, currentlySet[j] + (int)((((double)received[j] - (double)currentlySet[j]) / (double)biggestChange) * (double)i));
#endif // !DEBUG

#ifdef DEBUG
				Serial.print(currentlySet[j] + (int)((((double)received[j] - (double)currentlySet[j]) / (double)biggestChange) * (double)i));
				Serial.print(' ');
#endif // DEBUG

			}

#ifdef DEBUG
			Serial.println("");
#endif // DEBUG

			delay(speed);
		}

	}

	for (int i = 0; i < received.size(); i++)
		currentlySet[i] = received[i];

	Serial.write(FLAG_MOV_FIN);
}

void setSingleServo(int servo, int signal)
{
	if (signal == 4096)
		driver.setPWM(servo, 0, 4096);
	else if (currentlySet[servo] == 4096)
	{
		driver.setPWM(servo, 0, signal);
		currentlySet[servo] = signal;
	}
	else if (signal == currentlySet[servo])
	{

	}
	else
	{
		for (int i = currentlySet[servo]; i != signal; i += (signal - currentlySet[servo]) / abs(signal - currentlySet[servo]))
		{

#ifdef DEBUG1
			Serial.print("arduino: loop in setsingleservo, servo ==");
			Serial.print(servo);
			Serial.print(", i == ");
			Serial.println(i);
#endif // DEBUG1


			driver.setPWM(servo, 0, i);
			delay(speed);
		}

		currentlySet[servo] = signal;

#ifdef DEBUG1
		Serial.print("arduino: currentlyset[");
		Serial.print(servo);
		Serial.print("] == ");
		Serial.println(currentlySet[servo]);
#endif // DEBUG1
	}


	Serial.write(FLAG_MOV_FIN);


}

void checkForTurnOffs()
{
	for (int i = 0; i < received.size(); i++)
	{
		if (received[i] == 4096)
		{
			received[i] = currentlySet[i];

			driver.setPWM(i, 0, 4096);
		}


		// do zmiany, tu powinno wysyłać raport o błędzie
		if (received[i] < 150 || received[i] > 600)
			received[i] = currentlySet[i];
	}
}

void signal()
{
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);
	delay(500);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);
	delay(500);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);

	Serial.write(FLAG_MOV_FIN);
}

// funkcja map, ale w typie danych double
double mapa(double f, double in_min, double in_max, double out_min, double out_max)
{
	return (f - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

bool equal(std::vector<int> v0, std::vector<int> v1)
{
	for (int i = 0; i < v0.size(); i++)
	{
		if (v0[i] != v1[i])
			return false;
	}

	return true;
}