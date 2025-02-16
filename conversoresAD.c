#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "inc/ssd1306.h"

// Definições dos pinos e constantes
#define I2C_INTERFACE i2c1
#define SDA_PIN 14
#define SCL_PIN 15
#define OLED_ADDR 0x3C
#define LED_VERDE 11
#define LED_AZUL 12
#define LED_VERMELHO 13
#define BOTAO_A 5
#define BOTAO_B 6
#define JOY_X 27 // Pino GP27 para o eixo X do joystick (Canal ADC1)
#define JOY_Y 26 // Pino GP26 para o eixo Y do joystick (Canal ADC0)
#define BTN_JOYSTICK 22  // Pino GP22 para o botão do joystick
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define VALOR_MAX_ADC 4096
#define VALOR_METADE_ADC (VALOR_MAX_ADC / 2)
#define SQUARE_SIZE 8
#define NEUTRAL 200 // Zona neutra para evitar pequenos movimentos
#define PWM_WRAP_VALUE 255 // Valor máximo do PWM (8 bits)

// Variáveis globais
static volatile bool sw_value = true;
static volatile uint32_t last_time_btn_a = 0; // Tempo do último evento do botão A
static volatile uint32_t last_time_btn_sw = 0; // Tempo do último evento do botão do joystick
static volatile uint cor_led_verde;
static volatile uint cor_led_azul;
static volatile uint cor_led_vermelho;
static volatile bool ativar_leds = true;
static volatile bool ligar_led_verde = false;
static volatile uint8_t border_style = 0; // Estilo da borda (0: sem borda, 1: borda simples, 2: borda nos 4 cantos)
static int prev_display_x = OLED_WIDTH / 2 - SQUARE_SIZE / 2; // Posição anterior do quadrado (X)
static int prev_display_y = OLED_HEIGHT / 2 - SQUARE_SIZE / 2; // Posição anterior do quadrado (Y)

// Protótipos das funções
uint CONFIGURE_PMW(uint8_t led_pin);
void CONFIGURE_BTN(uint8_t btn_pin);
void CONFIGURE_JOY();
void CONFIGURE_i2c();
void CONFIGURE_DISPLAY(ssd1306_t *ssd);
void READ_JOYSTICK(uint16_t *x_value, uint16_t *y_value);
void PROCESS_JOYSTICK(uint16_t x_value_raw, uint16_t y_value_raw, float *x_value, float *y_value);
void HANDLER_BTN_irq(uint gpio, uint32_t events);
void update_leds(uint16_t x_value_raw, uint16_t y_value_raw);
void update_display(float x_value, float y_value, ssd1306_t *ssd);
void draw_border(ssd1306_t *ssd, uint8_t style);

// Função principal
int main() {
    stdio_init_all(); // Inicializa a comunicação serial (para depuração)

    // Inicializa os periféricos
    cor_led_verde = CONFIGURE_PMW(LED_VERDE);
    cor_led_azul = CONFIGURE_PMW(LED_AZUL);
    cor_led_vermelho = CONFIGURE_PMW(LED_VERMELHO);

    CONFIGURE_BTN(BOTAO_A);
    CONFIGURE_BTN(BTN_JOYSTICK);

    CONFIGURE_JOY();
    CONFIGURE_i2c();

    ssd1306_t display;
    CONFIGURE_DISPLAY(&display);

    // Posição inicial do quadrado (centro do display)
    float x_value = 0.0f, y_value = 0.0f;
    update_display(x_value, y_value, &display); // Desenha o quadrado no centro

    while (true) {
        uint16_t x_value_raw, y_value_raw;

        // Lê e processa os valores do joystick
        READ_JOYSTICK(&x_value_raw, &y_value_raw);
        PROCESS_JOYSTICK(x_value_raw, y_value_raw, &x_value, &y_value);

        // Atualiza os LEDs e o display
        update_leds(x_value_raw, y_value_raw);
        update_display(x_value, y_value, &display);

        sleep_ms(50); // Aguarda um pouco antes da próxima leitura
    }
}

// Inicializa o PWM para um LED
uint CONFIGURE_PMW(uint8_t led_pin) {
    gpio_set_function(led_pin, GPIO_FUNC_PWM); // Configura o pino como PWM
    uint slice_num = pwm_gpio_to_slice_num(led_pin); // Obtém o número do slice PWM
    pwm_config config = pwm_get_default_config(); // Obtém a configuração padrão do PWM
    pwm_config_set_clkdiv(&config, 4.f); // Define o divisor de clock
    pwm_init(slice_num, &config, true); // Inicializa o PWM
    pwm_set_gpio_level(led_pin, 0); // Inicializa o LED desligado
    return slice_num; // Retorna o número do slice PWM
}

// Inicializa um botão
void CONFIGURE_BTN(uint8_t btn_pin) {
    gpio_init(btn_pin); // Inicializa o pino do botão
    gpio_set_dir(btn_pin, GPIO_IN); // Define como entrada
    gpio_pull_up(btn_pin); // Habilita pull-up interno
    gpio_set_irq_enabled_with_callback(btn_pin, GPIO_IRQ_EDGE_FALL, true, &HANDLER_BTN_irq); // Configura interrupção
}

// Inicializa o joystick
void CONFIGURE_JOY() {
    adc_init(); // Inicializa o ADC
    adc_gpio_init(JOY_X); // Configura o pino do eixo X
    adc_gpio_init(JOY_Y); // Configura o pino do eixo Y
}

// Inicializa o I2C
void CONFIGURE_i2c() {
    i2c_init(I2C_INTERFACE, 400 * 1000); // Inicializa o I2C com frequência de 400 kHz
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C); // Configura o pino SDA
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C); // Configura o pino SCL
    gpio_pull_up(SDA_PIN); // Habilita pull-up no pino SDA
    gpio_pull_up(SCL_PIN); // Habilita pull-up no pino SCL
}

// Inicializa o display SSD1306
void CONFIGURE_DISPLAY(ssd1306_t *ssd) {
    ssd1306_init(ssd, OLED_WIDTH, OLED_HEIGHT, false, OLED_ADDR, I2C_INTERFACE); // Inicializa o display
    ssd1306_config(ssd); // Configura o display
}

// Lê os valores do joystick
void READ_JOYSTICK(uint16_t *x_value, uint16_t *y_value) {
    adc_select_input(0); // Seleciona o canal ADC 0 (eixo Y)
    *y_value = adc_read(); // Lê o valor do eixo Y
    adc_select_input(1); // Seleciona o canal ADC 1 (eixo X)
    *x_value = adc_read(); // Lê o valor do eixo X
}

// Processa os valores do joystick
void PROCESS_JOYSTICK(uint16_t x_value_raw, uint16_t y_value_raw, float *x_value, float *y_value) {
    *x_value = (float)(x_value_raw - VALOR_METADE_ADC) / VALOR_METADE_ADC; // Normaliza o valor do eixo X (-1 a 1)
    *y_value = (float)(VALOR_METADE_ADC - y_value_raw) / VALOR_METADE_ADC; // Inverte o eixo Y para corrigir a direção
}

// Atualiza os LEDs com base nos valores do joystick
void update_leds(uint16_t x_value_raw, uint16_t y_value_raw) {
    if (ativar_leds) {
        // Controle do LED Vermelho (eixo X)
        if (abs((int)x_value_raw - VALOR_METADE_ADC) > NEUTRAL) {
            // Mapeia o valor do eixo X para a intensidade do LED vermelho (0 a 255)
            uint32_t pwm_level_red = ((uint32_t)abs((int)x_value_raw - VALOR_METADE_ADC) * PWM_WRAP_VALUE) / (VALOR_METADE_ADC - NEUTRAL);
            pwm_level_red = (pwm_level_red > PWM_WRAP_VALUE) ? PWM_WRAP_VALUE : pwm_level_red; // Limita ao valor máximo
            pwm_set_gpio_level(LED_VERMELHO, pwm_level_red); // Acende o LED vermelho
            pwm_set_gpio_level(LED_AZUL, 0); // Desliga o LED azul
        } else {
            pwm_set_gpio_level(LED_VERMELHO, 0); // Apaga o LED vermelho se na zona morta
        }

        // Controle do LED Azul (eixo Y)
        if (abs((int)y_value_raw - VALOR_METADE_ADC) > NEUTRAL) {
            // Mapeia o valor do eixo Y para a intensidade do LED azul (0 a 255)
            uint32_t pwm_level_blue = ((uint32_t)abs((int)y_value_raw - VALOR_METADE_ADC) * PWM_WRAP_VALUE) / (VALOR_METADE_ADC - NEUTRAL);
            pwm_level_blue = (pwm_level_blue > PWM_WRAP_VALUE) ? PWM_WRAP_VALUE : pwm_level_blue; // Limita ao valor máximo
            pwm_set_gpio_level(LED_AZUL, pwm_level_blue); // Acende o LED azul
            pwm_set_gpio_level(LED_VERMELHO, 0); // Desliga o LED vermelho
        } else {
            pwm_set_gpio_level(LED_AZUL, 0); // Apaga o LED azul se na zona morta
        }
    } else {
        // Desliga ambos os LEDs se o PWM estiver desativado
        pwm_set_gpio_level(LED_VERMELHO, 0);
        pwm_set_gpio_level(LED_AZUL, 0);
    }

    // Controle do LED Verde (botão do joystick)
    pwm_set_gpio_level(LED_VERDE, ligar_led_verde ? PWM_WRAP_VALUE : 0);
}

// Atualiza o display com base nos valores do joystick
void update_display(float x_value, float y_value, ssd1306_t *ssd) {
    // Limpa o display inteiro antes de desenhar
    ssd1306_clear(ssd);

    // Calcula a nova posição do quadrado
    int display_x = (int)((x_value + 1) * (OLED_WIDTH - SQUARE_SIZE) / 2); // Eixo X
    int display_y = (int)((y_value + 1) * (OLED_HEIGHT - SQUARE_SIZE) / 2); // Eixo Y

    // Limita a posição do quadrado para não ultrapassar os limites do display
    display_x = (display_x < 0) ? 0 : (display_x > OLED_WIDTH - SQUARE_SIZE) ? OLED_WIDTH - SQUARE_SIZE : display_x;
    display_y = (display_y < 0) ? 0 : (display_y > OLED_HEIGHT - SQUARE_SIZE) ? OLED_HEIGHT - SQUARE_SIZE : display_y;

    // Desenha o quadrado na nova posição
    ssd1306_rect(ssd, display_y, display_x, SQUARE_SIZE, SQUARE_SIZE, true, true);

    // Desenha a borda de acordo com o estilo atual
    draw_border(ssd, border_style);

    // Envia os dados para o display
    ssd1306_send_data(ssd);
}

// Desenha a borda do display de acordo com o estilo
void draw_border(ssd1306_t *ssd, uint8_t style) {
    switch (style) {
        case 0: // Sem borda
            break;
        case 1: // Borda simples
            ssd1306_rect(ssd, 0, 0, OLED_WIDTH, OLED_HEIGHT, true, false);
            break;
        case 2: // Borda nos 4 cantos
            // Canto superior esquerdo
            ssd1306_rect(ssd, 0, 0, 10, 10, true, false);
            // Canto superior direito
            ssd1306_rect(ssd, 0, OLED_WIDTH - 10, 10, 10, true, false);
            // Canto inferior esquerdo
            ssd1306_rect(ssd, OLED_HEIGHT - 10, 0, 10, 10, true, false);
            // Canto inferior direito
            ssd1306_rect(ssd, OLED_HEIGHT - 10, OLED_WIDTH - 10, 10, 10, true, false);
            break;
    }
}

// Trata as interrupções dos botões
void HANDLER_BTN_irq(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time()); // Obtém o tempo atual

    if (gpio == BOTAO_A && (current_time - last_time_btn_a > 200000)) { // Debouncing para o botão A
        ativar_leds = !ativar_leds; // Alterna o estado do PWM
        last_time_btn_a = current_time; // Atualiza o tempo do último evento
    }

    if (gpio == BTN_JOYSTICK && (current_time - last_time_btn_sw > 200000)) { // Debouncing para o botão do joystick
        ligar_led_verde = !ligar_led_verde; // Alterna o estado do LED verde

        // Alterna entre os estilos de borda (0: sem borda, 1: borda simples, 2: borda nos 4 cantos)
        border_style = (border_style + 1) % 3; // Incrementa e volta a 0 após 2

        last_time_btn_sw = current_time; // Atualiza o tempo do último evento
    }
}