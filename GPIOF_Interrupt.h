#include "IEEE_CAN.h"


volatile uint32_t FallingEdges = 0;

// Valores de control sobre potenciometros MCP41010 externos
// Contadores en base a las interrupciones de botones

uint8_t interrNum=0;
uint8_t interrAmp=0;
uint8_t interrFrq=0;

void DisableInterrupts(void){
    __asm(  "  CPSID I ");
}

void EnableInterrupts(void){
    __asm( "  CPSIE I  ");
}

//--------------------------------------------------------------------------
//%%% Configura el puerto F como fuente de interrupcion por flanco de bajada
//--------------------------------------------------------------------------
void PUERTOF_Init (void) {
    // Puerto F1 - SW1 ,  F2 - SW2, F3 - SW3
    SYSCTL_RCGCGPIO_R |= 0x20;                                              // Reloj para puerto F
    while((SYSCTL_PRGPIO_R&0X20)==0);
    GPIO_PORTF_AHB_DIR_R   &= ~0x0E ;                                       // (c) terminales PF como entradas
    GPIO_PORTF_AHB_DEN_R   |= 0x0E;                                         // (d)Habilita modo digital PF1,2,3
    GPIO_PORTF_AHB_IS_R    &= ~0x0E;                                        // (f) PF sensible en flanco
    GPIO_PORTF_AHB_IBE_R   &= ~0x0E;                                        // (g) PF1 is not both edges
    GPIO_PORTF_AHB_IEV_R   &= ~0x0E;                                        // PF falling edge event
    GPIO_PORTF_AHB_ICR_R    = 0x0E;                                         // (e) clear flag1
    GPIO_PORTF_AHB_IM_R    |=0x0E ;                                         // (f) arm interrupt on PF1,2,3
    //Numero de interrupcion 30
    NVIC_PRI7_R  &=  0x00000000;                                            // (g) priority 5 1010 0000000
    NVIC_EN0_R   |=  1 << (30);                                             // (h) enable interrupt 30 in NVIC Desplaza 30 lugares


    EnableInterrupts();                                                     //Directiva de activar interrupciones enmascarables
}


//-----------------------------------------------------------------
//%%% Transmision de las variables de control para el potenciometro
//%%% Segun el boton presionado se incrementa y envia el valor de--
//%%% la variable que modifica al generador de señales que reciba--
//-----------------------------------------------------------------
void GPIOPORTF_Handler (void){
        uint8_t R;
        R = GPIO_PORTF_AHB_RIS_R;
        switch(R){
        case 0x02:
            interrNum++;

            if (interrNum == 4){
                  interrNum = 1;
            }
            CAN_Memoria_Dato((interrNum<<8) | 0XFF0000,0x2);
           break;

        case 0x04:
            interrAmp+=10;

            if(interrAmp == 110){
                 interrAmp = 0;
            }
            CAN_Memoria_Dato((interrAmp<<8) | 0XFF0001,0x2);
            break;

        case 0x08:
            interrFrq+=50;

            if(interrFrq == 300){
                 interrFrq = 0;
             }
            CAN_Memoria_Dato((interrFrq<<8) | 0XFF0002,0x2);
            break;

        default:

        break;
        }

        CAN_Tx(0x2);
        FallingEdges = FallingEdges + 1 ;
        GPIO_PORTF_AHB_ICR_R =  0x0E; // limpia acknowledge flag


}




