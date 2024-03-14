#include <stdbool.h>
#include <stdint.h>
#include "inc/tm4c1294ncpdt.h"
#include "IEEE_CAN.h"
#include "driverlib/sysctl.h"
#include "VL53_I2C_drivers.h"
#include "GPIOF_Interrupt.h"
#include "SPI_POT_DRIVERS.h"

uint64_t Rx[3];
uint64_t JAIME = 0x444;     //reloj en tiempo real
uint64_t PABLO = 0x555;     //Sensor de distancia
uint64_t JOSHUA = 0x222;
uint64_t CIRO = 0x111;      //Sensor de inclinación
uint64_t ERICK = 0x333;
uint64_t dato = 0x159137;



//--------------------------------------------------------------------
//%%%%%%    INICIALIZACIÓN DE PUERTOS ASOCIADOS AL CAN0    %%%%%%%%%%%
//                 CAN0Rx: PA0    CAN0Tx: PA1
//--------------------------------------------------------------------
void Config_Puertos(void){                      //(TM4C1294NCPDT)
    SYSCTL_RCGCGPIO_R|=0x1;                     //Reloj Puerto A
    while((SYSCTL_PRGPIO_R&0x1)==0){}
    GPIO_PORTA_AHB_AFSEL_R|=0x3;                 //PA0 y PA1 función alterna
    GPIO_PORTA_AHB_PCTL_R|=0x77;                 //Función CAN a los pines PA0-PA1
    GPIO_PORTA_AHB_DEN_R|=0x3;                   //Hab función digital PA0 y PA1
}

//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    INICIALIZACIÓN CAN0     %%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Config_CAN(void){
    SYSCTL_RCGCCAN_R=0x1;                               //Reloj modulo 0 CAN
    while((SYSCTL_PRCAN_R&0x1)==0){}
                                                        //Bit Rate= 1 Mbps      CAN clock=16 [Mhz]
    CAN0_CTL_R=0x41;                                    //Deshab. modo prueba, Hab. cambios en la config. y hab. inicializacion
    CAN0_BIT_R=0x4900;                                  //TSEG2=4   TSEG1=9    SJW=0    BRP=0
                                                        //Lenght Bit time=[TSEG2+TSEG1+3]*tq
                                                        //               =[(Phase2-1)+(Prop+Phase1-1)+3]*tq
    CAN0_CTL_R&=~0x41;                                  //Hab. cambios en la config. y deshab. inicializacion
    CAN0_CTL_R|=0x2;                                    //Hab de interrupción en el módulo CAN
    NVIC_EN1_R|=((1<<(38-32)) & 0xFFFFFFFF);            //(TM4C1294NCPDT)
    NVIC_PRI9_R = (NVIC_PRI2_R&0xFF00FFFF)|0x00010000; // Prioridad 1 (p.152) Interrupcion 8
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
uint32_t segundos;

void setSPI_parameters(uint64_t RxPar){
    int opcion = RxPar & 0x0F;
    uint8_t valor = (RxPar & 0xFF00) >>8;

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
//--------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%    INTERRUPCIÓN DEL CAN0    %%%%%%%%%%%%%%%%%%%%%%
//--------------------------------------------------------------------
void Inter_CAN0(void){
    uint8_t NoInt;
    NoInt=CAN0_INT_R;                                //Lectura del apuntador de interrupciones
    CAN0_STS_R&=~0x10;                               //Limpieza del bit de recepcion

    if(NoInt==0x1){
        Rx[0]=CAN_Rx(NoInt);                         //Recepción de datos
        if ((Rx[0]&0xFF0000)==0){
                    inclinacion = Rx[0]&0xFF;
                }
        else{
        setSPI_parameters(Rx[0]);
        }
    }

    if(NoInt==0x3){
        Rx[1]=CAN_Rx(NoInt);                         //Recepción de datos
        if ((Rx[1]&0xFF0000)==0){
            segundos = Rx[1]&0xFF;
        }
        else{
            setSPI_parameters(Rx[1]);
        }

    }
    if(NoInt==0x4){
            Rx[2]=CAN_Rx(NoInt);                //Recepción de datos RESPUESTA
        }
    //    CAN_Error();
}



//------------------------------------------------------------------
//%%%%%%%%%%%%%%%%%%%%    PROGRAMA PRINCIPAL    %%%%%%%%%%%%%%%%%%%%
//------------------------------------------------------------------
void main(void){

    I2C_Init();
    VL53_Init();
    PUERTOF_Init();
    Config_Puertos();
    Config_CAN();
    SSI0_init();
    SSI0_trigger();
    //Localidad 1 Rx con Msk
    CAN_Memoria_Arb(CIRO,false,0x1);                   //ID, TxRx, Localidad
    CAN_Memoria_CtrlMsk(0xFF,3,false,true,false,0x1);  //Mask, DLC, TxIE, RxIE, Remote, Localidad


    //Localidad 3 Rx con Msk
    CAN_Memoria_Arb(JAIME,false,0x3);                   //ID, TxRx, Localidad
    CAN_Memoria_CtrlMsk(0xFF,3,false,true,false,0x3);  //Mask, DLC, TxIE, RxIE, Remote, Localidad


uint16_t DatoSensor;

//-------------------------Localidad 2 Tx--------------------------
//%%%%%-------Sirve para enviar datos de control al SPI de los demas
//%%%%--------Ell envio es mediante interrupciones
//------------------------------------------------------------------

        CAN_Memoria_Arb(PABLO,true,0x2);                            //ID, TxRx, Localidad
        CAN_Memoria_CtrlMsk(0,3,false,false,false,0x2);             //Mask, DLC, TxIE, RxIE, Remote, Localidad

//--------------------------Localidad 4 Tx------------------------
//%%%%%%------Sirve para enviar datos del propio sensor ----------
//%%%%%%--Y que los demas puedan monitorear siempre en el while--

        CAN_Memoria_Arb(PABLO,true,0x4);                            //ID, TxRx, Localidad
        CAN_Memoria_CtrlMsk(0,3,false,false,false,0x4);             //Mask, DLC, TxIE, RxIE, Remote, Localidad
                             //Dato, Localidad
    while(1){
        DatoSensor = tmpuint16;
        SysCtlDelay(333);                //Retraso 11 [us] n=fsys*tdes/3
        CAN_Memoria_Dato(DatoSensor,0x4);
        CAN_Tx(0x4);

    }
}





