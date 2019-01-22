enum DIR {
  FORWARD,
  BACKWARD,
  OFF
};
typedef struct MotorConfig {
  private: int power;
  public:
  int freq = 30000;
  int pwmChannel;
  int resolution = 8;
  int pin1;
  int pin2;
  int pinEnable;
  DIR dir = OFF;
  MotorConfig(int pn1, int pn2, int pE, int pChan) : pin1(pn1), pin2(pn2), pinEnable(pE), pwmChannel(pChan) {}
  void setup() {
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    pinMode(pinEnable, OUTPUT);
    ledcSetup(pwmChannel, freq, resolution);
    ledcAttachPin(pinEnable, pwmChannel);
  }

  void update() {
    if (dir == FORWARD) { //forwards
      ledcWrite(pwmChannel, power);
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, HIGH);
    }
    else if (dir == BACKWARD) { //backwards
      ledcWrite(pwmChannel, power);
      digitalWrite(pin1, HIGH);
      digitalWrite(pin2, LOW);
    }
    else { //stop
      ledcWrite(pwmChannel, 0);
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, LOW);
    }
  }

  void setPower(int nPower){
    power = map(nPower, 0, 255, 150, 255);
  }

  void setDirection(DIR direct){
    dir = direct;  
  }
};
