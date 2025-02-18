## Autor: Cleber Santos Pinto Júnior - https://github.com/cleberspjr

## CONVERSORES A/D
O código fornecido implementa um projeto que utiliza o microcontrolador RP2040 para interagir com um joystick, LEDs RGB, e um display OLED SSD1306.

# OBJETIVOS
Compreender o funcionamento do conversor analógico-digital (ADC) no RP2040.
Utilizar o PWM para controlar a intensidade de dois LEDs RGB com base nos valores do joystick.
Representar a posição do joystick no display SSD1306 por meio de um quadrado móvel.
Aplicar o protocolo de comunicação I2C na integração com o display.

## Componentes Utilizados

• LED RGB, com os pinos conectados às GPIOs (11, 12 e 13).
• Botão do Joystick conectado à GPIO 22.
• Joystick conectado aos GPIOs 26 e 27.
• Botão A conectado à GPIO 5.
• Display SSD1306 conectado via I2C (GPIO 14 e GPIO15).

## SOBRE A TAREFA
No display SSD1306 há um quadrado de 8x8 pixels, inicialmente centralizado, que se move proporcionalmente aos valores capturados pelo joystick.
Ao mover o joystick no eixo x, o LED Vermelho é acionado em um brilho mínimo e a intensidade vai aumentando cada vez que chega nas extremidades 
Ao mover o joystick no eixo y, o LED AZUL é acionado em um brilho mínimo e a intensidade vai aumentando cada vez que chega nas extremidades 
Ao clicar no botão A, o estado dos leds são alterados. 1 clique: o led não é mais acionado a medida que mexemos o joystick, 2 cliques, o led volta a ser acionado.
A cada clique no joystick, alterna o estado do led verde (inicialmente desligado), e alterna o estilo das bordas
As bordas por sua vez estão definidas da seguinte forma: 
   1. SEM BORDA
   2. BORDA SIMPLES
   3. BORDA NOS CANTOS 

## MODO DE EXECUÇÃO

1. Clonar o Repositório: git clone https://github.com/cleberspjr/conversoresAD

2. Configure o ambiente de desenvolvimento PICO SDK

3. Abra o projeto no VS Code e importe o projeto através da extensão Raspberry Pi 

4. Execute a simulação na placa Bitdoglab


## LINK PARA O VÍDEO DA TAREFA: 

https://drive.google.com/file/d/1GnUMHHeNMtcw6FvqmhU6u9NefJwJ6nfW/view?usp=sharing
