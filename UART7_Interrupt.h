

#define UART7_IFLS_R (*((volatile uint32_t *)0x40013034))
// Selección de interrupión para FIFO (p.1191)
#define UART_IFLS_RX1_8  0x00000000  // RX FIFO >= 1/8 full (p. 1192)
#define UART_IFLS_TX1_8  0x00000000  // TX FIFO <= 1/8 full (p. 1192)

#define SYSCTL_PRGPIO_R0        0x00000004  // Puerto GPIO C listo

#define SYSCTL_PRUART_R         (*((volatile uint32_t *)0x400FEA18))
//Estado del UART (p.505)
#define SYSCTL_PRUART_R0        0x00000080  // UART Module 7 del UART listo


void UART7_init(void){
SYSCTL_RCGCUART_R |=  0x00000080; // activa el reloj para el UART7 (p.388)
while((SYSCTL_PRUART_R&SYSCTL_PRUART_R0) == 0){};
// Se espera a que el reloj se estabilice (p.505)

UART7_CTL_R &= ~0x00000001; // se deshabilita el UART (p.1188)
UART7_IBRD_R = 104;
// IBRD = int(16,000,000 / (16 * 9600)) = int(8.681) (p.1184)
UART7_FBRD_R = 11;
 // FBRD = round(0.16 * 64) = 11 (p. 1185) 9600 baudios == cargar 104.16

 // Palabra de 8 bits (sin bits de paridad, un bit stop, FIFOs) (p. 1186)
 UART7_LCRH_R = (UART_LCRH_WLEN_8|UART_LCRH_FEN);
// UART toma su reloj del la fuente alterna como se define por SYSCTL_ALTCLKCFG_R (p. 1213)
UART7_CC_R = (UART7_CC_R&~UART_CC_CS_M)+UART_CC_CS_PIOSC;
// La fuente de reloj alterna es el PIOSC (default)(P. 280)
SYSCTL_ALTCLKCFG_R = (SYSCTL_ALTCLKCFG_R&~SYSCTL_ALTCLKCFG_ALTCLK_M)+SYSCTL_ALTCLKCFG_ALTCLK_PIOSC; //

SYSCTL_RCGCGPIO_R |= 0x00000004;  // activa el reloj para el Puerto C (p.382)
 // Se espera a que el reloj se estabilice
 while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R0) == 0){};
 GPIO_PORTC_AFSEL_R |= 0x30; // habilita funcion alterna en PC5-4
 GPIO_PORTC_DEN_R |= 0x30;  // habilita digital I/O en PC5-4
 // configura PC5-4 como UART
 GPIO_PORTC_PCTL_R = (GPIO_PORTC_PCTL_R&0xFF00FFFF)+0x00110000;
 GPIO_PORTC_AMSEL_R &= ~0x30; // deshabilita la funcionabilidad analogica de PC

 // Alta velocidad deshabilitada;divide el reloj por 16 en lugar de 8 (default)(1188)
 UART7_CTL_R &= ~0x00000020;
 UART7_CTL_R |= 0x00000001;  // habilita el UART (p.1188)
 }


 void UART7_inter_config(void){
     //Bloque que configura la interrupcion y la desenmascara
  UART7_IFLS_R &= ~0x0000003F;   // Limpia las interrupciones de FIFO de TX y RX (p.1191)
  UART7_IFLS_R += (UART_IFLS_TX1_8|UART_IFLS_RX1_8);
  UART7_IFLS_R |= 0x08; //FIFO de la UART de recepcion a 32 bits para levantar (p.1192)

  // Habilita las interrupciones de la FIFO de TX y RX, y la interrupción por time-out de RX
  UART7_ICR_R = 0x10;
  // Limpia la bandera interrupcion de FIFO RX
  //(En este programa solo manejaremos esta interrupción)
  UART7_IM_R |= 0x00000010;
  // Desenmascara la interrupción para UART7

  // Configuración de prioridad y hablitación de interrupción en NVIC
  NVIC_PRI15_R = (NVIC_PRI15_R&0xFFFFFF00)|0x00000000; // Prioridad 0 (p.152)
  NVIC_EN1_R = 1<<(60-32); // Habilita la interrupción 60 en NVIC (p. 154)

  //Fin de configuración de interrupcion
 }
