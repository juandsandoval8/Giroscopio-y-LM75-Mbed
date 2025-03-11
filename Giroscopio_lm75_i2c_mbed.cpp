// Lectura de sensores de temperatura, Girosocopio usando I2C con LCD a 4 BITS

#include "mbed.h"
#include "TextLCD.h"

// Configura el display LCD
TextLCD lcd(PB_5, PB_4, PA_5, PA_6, PA_7, PB_6, TextLCD::LCD16x2); //RS,Enable,D4-D0,TipoLCD

// Canales de comunicación I2C
I2C i2c_1(PB_9, PB_8); //SDA,SCL

// Direcciones de cada elemento en el canal I2C
const int dir_lm75_1 = 0x90;  // Dirección LM75_1
const int dir_lm75_2 = 0x92;  // Dirección LM75_2
const int dir_DS3231 = 0xD0;  // Dirección DS3231
const int dir_mpu6050 = 0xD2; // Dirección MPU6050

// Hilos para lectura de sensores con prioridad normal
Thread hilo_temp(osPriorityNormal, 1024); // damos prioridad a la activacion del hilo
Thread hilo_reloj(osPriorityNormal, 1024); // damos la prioridad a la activacion del hilo
Thread hilo_mpu6050(osPriorityNormal, 1024); // damos la activacion a la prioridad del hilo

char cadena[500];
char d_semana[20];
char escribir_ds3231[8], leer_ds3231[8]; // Punteros de escritura y lectura DS3231
char registro_inicio[2]; // Puntero para iniciar el MPU6050
char leer_acelerometro[6], leer_giroscopio[6]; // Punteros para MPU6050

int t_e, t_f, i, l, Y, M, D, di, h, m, s, segundos, minutos, hora, dia_de_la_semana, dia, mes, año; // Variables DS3231
int temp_ent, temp_flo, temp1_ent, temp1_flo, temp2_ent, temp2_flo; // Variables para LM75
int acelx_e, acelx_f, acely_e, acely_f, acelz_e, acelz_f; // Variables para acelerómetro MPU6050
int girox_e, girox_f, giroy_e, giroy_f, giroz_e, giroz_f; // Variables para giroscopio MPU6050

float temp, temp1, temp2;
float acel_x, acel_y, acel_z, giro_x, giro_y, giro_z;

// Se crea la clase LM75 para los sensores
class LM75 {
public:
    LM75(I2C& i2c, int direccion_i2c) : i2c_(i2c), i2cAddress_(direccion_i2c) {
        char registro_configuracion[2] = {0x01, 0x00};
        i2c_.write(i2cAddress_, registro_configuracion, 2);
    }

    float leer_temperatura() {
        char registro_temperatura[1] = {0x00};
        char lectura_temperatura[2];
        i2c_.write(i2cAddress_, registro_temperatura, 1);
        i2c_.read(i2cAddress_, lectura_temperatura, 2);
        float temperatura = ((lectura_temperatura[0] << 8) | lectura_temperatura[1]) / 256.0;
        return temperatura;
    }

private:
    I2C& i2c_;
    int i2cAddress_;
};

void configuracion() {
    // Configurar la sensibilidad del acelerómetro (±2g) y del giroscopio (±250°/s)
    char configuracion_acel[] = {0x1C, 0x00}; // Registro de configuración del acelerómetro
    char configuracion_giro[] = {0x1B, 0x00}; // Registro de configuración del giroscopio
    i2c_1.write(dir_mpu6050, configuracion_acel, 2);
    i2c_1.write(dir_mpu6050, configuracion_giro, 2);

    // Enciende el MPU6050
    registro_inicio[0] = 0x6B; // Registro de energía MPU6050
    registro_inicio[1] = 0x00; // Despierta el MPU6050
    i2c_1.write(dir_mpu6050, registro_inicio, 2);

    // Configuración inicial del reloj
    Y = 2023; M = 9; D = 06; di = 4;  // Asignación inicial de fecha
    h = 11; m = 39; s = 00; // Asignación inicial de la hora
    escribir_ds3231[0] = 0; // Establece el puntero de escritura en la dirección 0 (segundos)
    escribir_ds3231[1] = ((s / 10) << 4) | (s % 10);
    escribir_ds3231[2] = ((m / 10) << 4) | (m % 10);
    escribir_ds3231[3] = ((h / 10) << 4) | (h % 10);
    escribir_ds3231[4] = ((di / 10) << 4) | (di % 10);
    escribir_ds3231[5] = ((D / 10) << 4) | (D % 10);
    escribir_ds3231[6] = ((M / 10) << 4) | (M % 10);
    escribir_ds3231[7] = (((Y - 2000) / 10) << 4) | ((Y - 2000) % 10);
    i2c_1.write(dir_DS3231, escribir_ds3231, sizeof(escribir_ds3231));
}

void leer_sensores() {
    while (1) {
        LM75 sensor1(i2c_1, dir_lm75_1);
        LM75 sensor2(i2c_1, dir_lm75_2);
        temp1 = sensor1.leer_temperatura();
        temp2 = sensor2.leer_temperatura();
        temp1_ent = static_cast<int>(temp1);
        temp1_flo = static_cast<int>(temp1 * 100) % 100;
        temp2_ent = static_cast<int>(temp2);
        temp2_flo = static_cast<int>(temp2 * 100) % 100;
        ThisThread::sleep_for(100ms);
    }
}

void leer_reloj() {
    while (1) {
        i2c_1.write(dir_DS3231, &escribir_ds3231[0], 1); // Establece el puntero de lectura en la dirección 0 del DS3231
        i2c_1.read(dir_DS3231, leer_ds3231, sizeof(leer_ds3231));
        segundos = ((leer_ds3231[0] >> 4) * 10) + (leer_ds3231[0] & 0x0F);
        minutos = ((leer_ds3231[1] >> 4) * 10) + (leer_ds3231[1] & 0x0F);
        hora = ((leer_ds3231[2] >> 4) * 10) + (leer_ds3231[2] & 0x0F);
        dia_de_la_semana = leer_ds3231[3] & 0x07;
        dia = ((leer_ds3231[4] >> 4) * 10) + (leer_ds3231[4] & 0x0F);
        mes = ((leer_ds3231[5] >> 4) * 10) + (leer_ds3231[5] & 0x0F);
        año = 2000 + ((leer_ds3231[6] >> 4) * 10) + (leer_ds3231[6] & 0x0F);

        // Leer temperatura DS3231
        char registro_temp = 0x11;
        char lecturaTemperatura[2];
        i2c_1.write(dir_DS3231, &registro_temp, 1);
        i2c_1.read(dir_DS3231, lecturaTemperatura, 2);
        int temperaturaRaw = ((lecturaTemperatura[0] << 8) | lecturaTemperatura[1]);
        temp = (temperaturaRaw >> 6) * 0.25;
        temp_ent = static_cast<int>(temp);
        temp_flo = static_cast<int>(temp * 100) % 100;
        ThisThread::sleep_for(50ms);
    }
}

void leer_mpu6050() {
    while (1) {
        char r_acel = 0x3B; // Apunta al registro del acelerómetro
        i2c_1.write(dir_mpu6050, &r_acel, 1);
        i2c_1.read(dir_mpu6050, leer_acelerometro, 6);

        char r_giro = 0x43; // Apunta al registro de giroscopio
        i2c_1.write(dir_mpu6050, &r_giro, 1);
        i2c_1.read(dir_mpu6050, leer_giroscopio, 6);

        acel_x = (((leer_acelerometro[0] << 8) | leer_acelerometro[1]) * 9.81) / 16384.0;
        acelx_e = static_cast<int>(acel_x);
        acelx_f = static_cast<int>(acel_x * 100) % 100;

        acel_y = (((leer_acelerometro[2] << 8) | leer_acelerometro[3]) * 9.81) / 16384.0;
        acely_e = static_cast<int>(acel_y);
        acely_f = static_cast<int>(acel_y * 100) % 100;

        acel_z = (((leer_acelerometro[4] << 8) | leer_acelerometro[5]) * 9.81) / 16384.0;
        acelz_e = static_cast<int>(acel_z);
        acelz_f = static_cast<int>(acel_z * 100) % 100;

        giro_x = ((leer_giroscopio[0] << 8) | leer_giroscopio[1]) / 131.0;
        girox_e = static_cast<int>(giro_x);
        girox_f = static_cast<int>(giro_x * 100) % 100;

        giro_y = ((leer_giroscopio[2] << 8) | leer_giroscopio[3]) / 131.0;
        giroy_e = static_cast<int>(giro_y);
        giroy_f = static_cast<int>(giro_y * 100) % 100;

        giro_z = ((leer_giroscopio[4] << 8) | leer_giroscopio[5]) / 131.0;
        giroz_e = static_cast<int>(giro_z);
        giroz_f = static_cast<int>(giro_z * 100) % 100;

        ThisThread::sleep_for(80ms);
    }
}

void imprimir() {
    snprintf(d_semana, sizeof(d_semana), "%s", 
        dia_de_la_semana == 1 ? "Lunes" :
        dia_de_la_semana == 2 ? "Martes" :
        dia_de_la_semana == 3 ? "Miércoles" :
        dia_de_la_semana == 4 ? "Jueves" :
        dia_de_la_semana == 5 ? "Viernes" :
        dia_de_la_semana == 6 ? "Sábado" :
        dia_de_la_semana == 7 ? "Domingo" : "Error");

    snprintf(cadena, sizeof(cadena), "%04d-%02d-%02d %s", año, mes, dia, d_semana); // Impresión fecha primer renglón LCD
    lcd.locate(0, 0);
    lcd.printf("%s", cadena);

    snprintf(cadena, sizeof(cadena), "%02d:%02d:%02d %02d,%02d C", hora, minutos, segundos, t_e, t_f); // Impresión hora y temp segundo renglón LCD
    lcd.locate(0, 1);
    lcd.printf("%s", cadena);
}

int main() {
    configuracion();
    hilo_temp.start(leer_sensores);
    hilo_reloj.start(leer_reloj);
    hilo_mpu6050.start(leer_mpu6050);
    while (1) {
        imprimir();
        ThisThread::sleep_for(1s);
    }
}
