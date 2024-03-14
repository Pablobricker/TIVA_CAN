volatile uint32_t FallingEdges = 0;

void DisableInterrupts(void){
    __asm(  "  CPSID I ");
}

void EnableInterrupts(void){
    __asm( "  CPSIE I  ");
}


void EdgeCounteR_Init (void) {
    // Puerto J0 - SW1 , y puerto N, PN1 - Led 1
    SYSCTL_RCGCGPIO_R |= 0x1100;// (a) Activa reloj para puertos J y N
    FallingEdges = 0;       // (b) inicializa contador
    GPIO_PORTJ_AHB_DIR_R   &=0x00 ; // (c) terminal PJ0 de entrada (boton integrado SW1)
    GPIO_PORTJ_AHB_DEN_R   |= 0x01; // (d)Habilita modo digital PJ0
    //GPIO_PORTJ_AHB_PUR_R   |= 0x01; // (e)Habilita pull-up en PJ0
    GPIO_PORTJ_AHB_IS_R    &= 0x00;// (f) PJ0 sensible en flanco
    GPIO_PORTJ_AHB_IBE_R    = 0x00; // (g) PJ0 is not both edges
    GPIO_PORTJ_AHB_IEV_R   &= 0x00; // PJ0 falling edge event
    GPIO_PORTJ_AHB_ICR_R    = 0x01; // (e) clear flag1
    GPIO_PORTJ_AHB_IM_R    |=0x01 ; // (f) arm interrupt on PJ0

    NVIC_PRI12_R   = (NVIC_PRI12_R & 0x00FFFFFF) | 0xA0000000; // (g) priority 5 1010 0000000
    NVIC_EN1_R    =  NVIC_EN1_R | ( 1 << 19); // (h) enable interrupt 51 in NVIC Desplaza 19 lugares


        //  Se tiene que habilitar la interrupcion (bit) 51 del conjunto de registros NVIC_EN0_R y NVIC_EN1_R
        //  En el registro NVIC_EN0_R se tienen los bits 0-31
        //  En el registro NVIC_EN1_R se tienen los bits 32-63
        //  51-32 = 19 -> el bit 19 del siguiente registro (NVIC_EN1_R)

        // NVIC_EN1_R = NVIC_EN1_R | ( 1 << 19) 19 es el bit asignado a la interrupcion 51 se obtiene con la resta del número de interupcion menos el inicio.
        // NVIC_EN1_R = NVIC_EN1_R | ( 1 << (51-32) );




//// Configuracion de LED en PN1-LED1
//    GPIO_PORTN_DIR_R   = 0x02;  // salida bit 1 PN1
//    GPIO_PORTN_DEN_R   = 0x02;

    EnableInterrupts();     //
}
int pinJ;
//int interrNum=0;
//void GPIOPORTJ_Handler (void){
//    interrNum++;
//    if (interrNum == 4){
//        interrNum = 0;
//    }
//
//    pinJ = GPIO_PORTJ_AHB_DATA_R & 0x0F;
//    //Swithc para saber que pin del puerto J es el presionado
//
//    GPIO_PORTJ_AHB_ICR_R =  0x01; // limpia acknowledge flag
//    FallingEdges = FallingEdges + 1 ;
//    int i,j;

//    switch(interrNum){
//    case 1:
//        for(i=0; i<10; i++){
//            CAN_Memoria_Dato(0x0001,0x2);
//            CAN_Tx(0x2);      //Enviar una trama de datos que controla el generador de funciones de ciro
//                                //Y otra que pida su valor del sensor mandando una trama remota
//
//            SysCtlDelay(1000000);
//        }
//
//        break;
//        case 2:
//            for(i=0; i<10; i++){
//                CAN_Memoria_Dato(0x0002,0x2);
//                CAN_Tx(0x2);      //Enviar una trama de datos que controla el generador de funciones de ciro
//                                    //Y otra que pida su valor del sensor mandando una trama remota
//
//                SysCtlDelay(1000000);
//            }
//
//            break;
//        case 3:
//            for(i=0; i<10; i++){
//                CAN_Memoria_Dato(0x0003,0x2);
//                CAN_Tx(0x2);      //Enviar una trama de datos que controla el generador de funciones de ciro
//                                    //Y otra que pida su valor del sensor mandando una trama remota
//
//                SysCtlDelay(1000000);
//            }
//
//            break;
//    default:
//
//                            CAN_Tx(0x2);      //Enviar una trama de datos que controla el generador de funciones de ciro
//                                                //Y otra que pida su valor del sensor mandando una trama remota
//
//                            SysCtlDelay(1000000);
//
//        break;
//}

//}


