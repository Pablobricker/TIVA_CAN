#include <stdbool.h>
#include <stdint.h>
#include "inc/tm4c1294ncpdt.h"
#include "IEEE_CAN.h"
#include "driverlib/sysctl.h"
#include "VL53_I2C_drivers.h"
//#include "GPIO_Interrupt.h"
//#include "SPI_POT_DRIVERS.h"

uint64_t Rx[3];
uint64_t JAIME = 0x444;     //reloj en tiempo real
uint64_t PABLO = 0x555;     //Sensor de distancia
uint64_t JOSHUA = 0x222;
uint64_t CIRO = 0x111;      //Sensor de inclinaci�n
uint64_t ERICK = 0x333;
uint64_t dato = 0x159137;


int interrNum=0;
int interrAmp=0;
int interrFrq=0;


//--------------------------------------------------------------------
//%%%%%%    INICIALIZACI�N DE PUERTOS ASOCIADOS AL CAN0    %%%%%%%%%%%
//                 CAN0Rx: PA0    CAN0Tx: PA1
//--------------------------------------------------------------------
void Config_Puertos(void){                      //(TM4C1294NCPDT)
    SYSCTL_RCGCGPIO_R|=0x1;                     //Reloj Puerto A
    while((SYSCTL_PRGPIO_R&0x1)==0){}
    GPIO_PORTA_AHB_AFSEL_R|=0x3;                 //PA0 y PA1 funci�n alterna
    GPIO_PORTA_AHB_PCTL_R|=0x77;                 //Funci�n CAN a los pines PA0-PA1
    GPIO_PORTA_AHB_DEN_R|=0x3;                   //Hab funci�n digital PA0 y PA1
}

//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INICIALIZACI�N CAN0     %%%%%%%%%%%%%%%%%%%%
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
    CAN0_CTL_R|=0x2;                            //Hab de interrupci�n en el m�dulo CAN
    NVIC_EN1_R|=((1<<(38-32)) & 0xFFFFFFFF);    //(TM4C1294NCPDT)
    NVIC_PRI9_R = (NVIC_PRI2_R&0xFF00FFFF)|0x00000000; // Prioridad 7 (p.152) Interrupcion 8
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
uint32_t inclinacion;
//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%    INTERRUPCI�N DEL CAN0    %%%%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Inter_CAN0(void){
    uint8_t NoInt;
    NoInt=CAN0_INT_R;                           //Lectura del apuntador de interrupciones
    CAN0_STS_R&=~0x10;                          //Limpieza del bit de recepcion

    if(NoInt==0x1){
        Rx[0]=CAN_Rx(NoInt);                //Recepci�n de datos
        //Shap = Rx[0];
    }
    if(NoInt==0x3){
        Rx[1]=CAN_Rx(NoInt);                //Recepci�n de datos remotos de ciro
                                            //La recepcion de la repsuesta a la trama remota se configura en la misma trama remota mensaje onjeto 3 //linea 98
    }
    if(NoInt==0x4){
            Rx[2]=CAN_Rx(NoInt);                //Recepci�n de datos RESPUESTA
        }
    //    CAN_Error();
}

//------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    PROGRAMA PRINCIPAL    %%%%%%%%%%%%%%%%%%%%
//------------------------------------------------------------------
void main(void){

    //I2C_Init();
    //VL53_Init();
    //SSI0_init();   // Funci�n que habilita el SPI
    Config_Puertos();
    Config_CAN();

    //Localidad 1 Rx con Msk
    //CAN_Memoria_Arb(CIRO,false,0x1);                   //ID, TxRx, Localidad
    //CAN_Memoria_CtrlMsk(0xFF,2,false,true,false,0x1);  //Mask, DLC, TxIE, RxIE, Remote, Localidad


    //Localidad 2 Tx
    CAN_Memoria_Arb(PABLO,true,0x2);                    //ID, TxRx, Localidad
    CAN_Memoria_CtrlMsk(0,2,false,false,false,0x2);     //Mask, DLC, TxIE, RxIE, Remote, Localidad
    CAN_Memoria_Dato(0x0001,0x2);                     //Dato, Localidad

//    //Localidad 3 Trama remota Rx para CIRO
//    CAN_Memoria_Arb(CIRO,false,0x3);                   //ID, TxRx, Localidad
//    CAN_Memoria_CtrlMsk(0xFFF,2,false,true,true,0x3);  //Mask, DLC, TxIE, RxIE, Remote, Localidad
//
//
//
//    //RESPUESTA A TRAMA REMOTA TX
//
//    //Localidad 5 Tx
//        CAN_Memoria_Arb(PABLO,true,0x5);                    //ID, TxRx, Localidad
//        CAN_Memoria_CtrlMsk(0xFFF,2,false,false,true,0x5);
//        CAN_Memoria_Dato(tmpuint16, 0x5);

    //pot_setVal(0x00);









    while(1){
        SysCtlDelay(333);                //Retraso 11 [us] n=fsys*tdes/3

        CAN_Tx(0x2);
        CAN_Memoria_Dato(0xFF,0x2);     //Esto es solo para transmitir trama de Datos al chilazo

        //CAN_Tx(0x3);
        //CAN_Tx(0x5);
        //CAN_Memoria_Dato(tmpuint16, 0x5); //Actualizaci�n del dato para enviar como respuesta a la trama remota


    }
}





