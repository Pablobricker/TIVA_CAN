#include <stdbool.h>
#include <stdint.h>
#include "inc/tm4c1294ncpdt.h"
#include "IEEE_CAN.h"                           //Drivers para comunicacion CAN
#include "driverlib/sysctl.h"
#include "VL53_I2C_dirvers.h"                   //Drivers para comunicacion I2C
#include "GPIOF_Interrupt.h"                    //Configuracion de interrupcion por boton
#include "SPI_POT_DRIVERS.h"                    //Drivers para comunicacion SPI

uint64_t Rx[4];                                 //Vector auxiliar para almacenar datos de control y sondeo

//--------------------------------------------------------------------
//%%%%%%        IDENTIFICADORES ASOCIADOS A LOS NODOS DE LA RED %%%%%%
//
//--------------------------------------------------------------------
uint64_t JAIME = 0x444;
uint64_t PABLO = 0x555;
uint64_t JOSHUA = 0x222;
uint64_t CIRO = 0x111;


//--------------------------------------------------------------------
//%%%%%%    INICIALIZACIÓN DE PUERTOS ASOCIADOS AL CAN0    %%%%%%%%%%%
//                 CAN0Rx: PA0    CAN0Tx: PA1
//--------------------------------------------------------------------
void Config_Puertos(void){                       //(TM4C1294NCPDT)
    SYSCTL_RCGCGPIO_R|=0x1;                      //Reloj Puerto A
    while((SYSCTL_PRGPIO_R&0x1)==0){}
    GPIO_PORTA_AHB_AFSEL_R|=0x3;                 //PA0 y PA1 función alterna
    GPIO_PORTA_AHB_PCTL_R|=0x77;                 //Función CAN a los pines PA0-PA1
    GPIO_PORTA_AHB_DEN_R|=0x3;                   //Hab función digital PA0 y PA1
}

//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INICIALIZACIÓN CAN0     %%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Config_CAN(void){
    SYSCTL_RCGCCAN_R=0x1;                       //Reloj modulo 0 CAN
    while((SYSCTL_PRCAN_R&0x1)==0){}
                                                //Bit Rate= 1 Mbps      CAN clock=16 [Mhz]
    CAN0_CTL_R=0x41;                            //Deshab. modo prueba, Hab. cambios en la config. y hab. inicializacion
    CAN0_BIT_R=0x4900;                          //TSEG2=4   TSEG1=9    SJW=0    BRP=0
                                                //Lenght Bit time=[TSEG2+TSEG1+3]*tq
                                                //               =[(Phase2-1)+(Prop+Phase1-1)+3]*tq
    CAN0_CTL_R&=~0x41;                          //Hab. cambios en la config. y deshab. inicializacion
    CAN0_CTL_R|=0x2;                            //Hab de interrupción en el módulo CAN
    NVIC_EN1_R|=((1<<(38-32)) & 0xFFFFFFFF);    //(TM4C1294NCPDT)
    NVIC_PRI9_R = 0x00100000; // Prioridad 1 (p.152) Interrupcion 38
}

void CAN_Error(void){
    static int ent=0;
    if(CAN0_STS_R&0x80){
        if(ent){
            NVIC_APINT_R|=0x4;                      //Reinicio de todo el sistema
        }else{
            CAN0_CTL_R=0x41;                        //Hab. cambios en la config. y hab. inicializacion
            CAN0_CTL_R|=0x80;                       //Hab. modo prueba
            CAN0_TST_R|=0x4;                        //Hab. Modo silencio
            CAN0_CTL_R&=~0x41;                      //Hab. cambios en la config. y deshab. inicializacion
            SysCtlDelay(333333);
            CAN0_CTL_R=0x41;                        //Hab. cambios en la config. y hab. inicializacion
            CAN0_TST_R&=~0x4;                       //Deshab. Modo silencio
            CAN0_CTL_R&=~0x41;                      //Hab. cambios en la config. y deshab. inicializacion
            ent++;
        }
    }
}

//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%    INTERRUPCIÓN DEL CAN0    %%%%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Inter_CAN0(void){
    uint8_t NoInt;
    NoInt=CAN0_INT_R;                               //Lectura del apuntador de interrupciones
    CAN0_STS_R&=~0x10;                              //Limpieza del bit de recepcion

    if(NoInt==0x1){
        Rx[0]=CAN_Rx(NoInt);                        //Recepción de datos para el control del SPI
        int opcion = Rx[0] & 0x0F;
        int valor = (Rx[0] & 0xFF00) >>8;

        switch(opcion){
        case 0x00:
            Shap = valor;
        break;
        case 0x01:
            Coef = valor;
            break;
        case 0x02:
            Delay = valor;
            break;

        }

    }
    if(NoInt==0x3){
        Rx[1]=CAN_Rx(NoInt);                //Recepción de datos remotos del sensor de ciro
                                            //La recepcion de la repsuesta a la trama remota se configura en la misma trama remota mensaje onjeto 3 //linea 98
    }
    if(NoInt==0x4){
            Rx[2]=CAN_Rx(NoInt);                //Recepción de datos RESPUESTA

        }
    if(NoInt==0x6){
                Rx[3]=CAN_Rx(NoInt);                //Recepción de datos RESPUESTA
            }
    //    CAN_Error();
}

//------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    PROGRAMA PRINCIPAL    %%%%%%%%%%%%%%%%%%%%
//------------------------------------------------------------------
void main(void){

    I2C_Init();
    VL53_Init();
    PUERTOF_Init();       //para modo recepcion comenta: inicializacion, interrupcion en startup, y mensaje objeto de transmision
    Config_Puertos();
    Config_CAN();
    SSI0_init();   // Función que habilita el SPI
    SSI0_trigger();

    //Localidad 1 Rx con Msk
    CAN_Memoria_Arb(CIRO,false,0x1);                   //ID, TxRx, Localidad
    CAN_Memoria_CtrlMsk(0xFF,2,false,true,false,0x1);  //Mask, DLC, TxIE, RxIE, Remote, Localidad
    //Localidad 4 Rx con Msk
    CAN_Memoria_Arb(JAIME,false,0x4);                   //ID, TxRx, Localidad
    CAN_Memoria_CtrlMsk(0xFFF,2,false,true,false,0x4);  //Mask, DLC, TxIE, RxIE, Remote, Localidad
    //Localidad 2 Tx
    CAN_Memoria_Arb(PABLO,true,0x2);                   //ID, TxRx, Localidad
    CAN_Memoria_CtrlMsk(0,3,false,false,false,0x2);     //Mask, DLC, TxIE, RxIE, Remote, Localidad
    //CAN_Memoria_Dato(0x000001,0x2);                     //Dato, Localidad
    //Localidad 3 Trama remota Rx para CIRO
    CAN_Memoria_Arb(CIRO,false,0x3);                   //ID, TxRx, Localidad
    CAN_Memoria_CtrlMsk(0xFFF,2,false,true,true,0x3);  //Mask, DLC, TxIE, RxIE, Remote, Localidad

    //Si yo pregunto  por la inclinacion de ciro debo preguntar con el identificador CIRO[Posicion]
    //Tanto yo tengo que preguntar por el identificador de CIRO[Posicion]
    //Tanto el me tiene que responder con el identificador CIRO[Posicion] que configuro como RESPUESTA A TRAMA REMOTA<


    //RESPUESTA A TRAMA REMOTA TX

    //Localidad 5 Tx
        CAN_Memoria_Arb(PABLO,true,0x5);                    //ID, TxRx, Localidad
        CAN_Memoria_CtrlMsk(0xFFF,2,false,false,true,0x5);
        CAN_Memoria_Dato(tmpuint16, 0x5);



    while(1){
        SysCtlDelay(333);                //Retraso 11 [us] n=fsys*tdes/3

        //CAN_Tx(0x2);
        //CAN_Memoria_Dato(tmpuint16,0x2);     //Esto es solo para transmitir trama de Datos al chilazo
        //SysCtlDelay(1000000);
        //SysCtlDelay(333);                //Retraso 11 [us] n=fsys*tdes/3
        CAN_Tx(0x5);

        CAN_Memoria_Dato(tmpuint16, 0x5); //Actualización del dato para enviar como respuesta a la trama remota


    }
}




