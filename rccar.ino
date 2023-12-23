#include <SoftwareSerial.h>

#define Ir_pin A1      // Ir센서 핀
#define MOTOR_A_SPD 10 // 모터A의 속력을 결정하는 핀
#define MOTOR_B_SPD 11 // 모터B의 속력을 결정하는 핀
#define MOTOR_A_DIR 12 // 모터A의 방향을 결정하는 핀
#define MOTOR_B_DIR 13 // 모터B의 방향을 결정하는 핀
#define A_BAL 1        // 모터A 속력 균형 계수 기본값 1
#define B_BAL 0.865    // 모터B 속력 균형 계수 기본값 1
#define M_SPEED 120    // 모터 속력
#define BT_RXD 8       // 블루투스 읽기 핀
#define BT_TXD 7       // 블루투스 전송 핀

SoftwareSerial bluetooth(BT_RXD, BT_TXD); // 새로운 시리얼 입력(블루투스)

unsigned char m_a_spd = 0, m_b_spd = 0; // 모터의 속력을 결정하는 전역변수
boolean m_a_dir = 0, m_b_dir = 0;       // 모터의 방향을 결정하는 전역변수

unsigned int adc_Ir = 0; // ir 거리 센서 값
int signal;              // RF 시그널

void setup()
{
    Serial.begin(115200);  // 시리얼 통신 초기화
    bluetooth.begin(9600); // 블루투스 시리얼 통신 초기화

    pinMode(MOTOR_A_DIR, OUTPUT); // 모터A 방향 핀 출력으로 설정
    pinMode(MOTOR_B_DIR, OUTPUT); // 모터B 방향 핀 출력으로 설정
    pinMode(Ir_pin, INPUT);       // ir 센서 핀 입력 설정

    Serial.println("Hello!"); // 터미널 작동 확인용 문자열
}

void loop()
{
    char bt_cmd = 0;             // 명령어 저장용 문자형 변수
    adc_Ir = analogRead(Ir_pin); // ir 센서 입력값 저장
    if (bluetooth.available())   // 블루투스 값이 입력되었을 때
    {
        bt_cmd = bluetooth.read(); // 변수에 입력된 데이터 저장
        rc_ctrl_val(bt_cmd);       // 입력된 데이터에 따라 모터에 입력될 변수를 조정하는 함수
    }
    motor_drive(); // 모터 구동 함수
    if (Serial.available())
    {                           // 보드 간 통신 값을 받았을 때
        signal = Serial.read(); // 수신 값 저장
    }
}

void rc_ctrl_val(char cmd) // 입력된 데이터에 따라 모터에 입력될 변수를 조정하는 함수
{
    if (cmd == 'w') //'w'가 입력되었을 때, 전진
    {
        m_a_dir = 1;               // 모터A 정방향
        m_b_dir = 0;               // 모터B 정방향
        m_a_spd = M_SPEED * A_BAL; // 모터A의 M_SPEED의 PWM
        m_b_spd = M_SPEED * B_BAL; // 모터B의 M_SPEED의 PWM
    }
    else if (cmd == 'x') //'x'가 입력되었을 때, 정지
    {
        m_a_dir = 1; // 모터A 정방향
        m_b_dir = 0; // 모터B 정방향
        m_a_spd = 0; // 모터A의 정지
        m_b_spd = 0; // 모터B의 정지
    }
}

void motor_drive() // 모터를 구동하는 함수
{
    if (adc_Ir > 400) // 평상시 도로
    {
        digitalWrite(MOTOR_A_DIR, m_a_dir); // 모터A의 방향을 디지털 출력
        digitalWrite(MOTOR_B_DIR, m_b_dir); // 모터B의 방향을 디지털 출력
        analogWrite(MOTOR_A_SPD, m_a_spd);  // 모터A의 속력을 PWM 출력
        analogWrite(MOTOR_B_SPD, m_b_spd);  // 모터B의 속력을 PWM 출력
    }
    else if (adc_Ir < 400 && signal != 'g') // 정지선 감지&적(황)색 신호 시
    {
        analogWrite(MOTOR_A_SPD, 0); // 정지
        analogWrite(MOTOR_B_SPD, 0); // 정지
    }
    else if (signal == 'g')
    { // 녹색 신호 시

        digitalWrite(MOTOR_A_DIR, m_a_dir); // 모터A의 방향을 디지털 출력
        digitalWrite(MOTOR_B_DIR, m_b_dir); // 모터B의 방향을 디지털 출력
        analogWrite(MOTOR_A_SPD, m_a_spd);  // 모터A의 속력을 PWM 출력
        analogWrite(MOTOR_B_SPD, m_b_spd);  // 모터B의 속력을 PWM 출력
    }
}