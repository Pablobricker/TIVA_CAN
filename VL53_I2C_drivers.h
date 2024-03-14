
#define I2C_MCS_ACK 0x00000008 //Transmmitter Acknowledge Enable
#define I2C_MCS_DATACK 0x00000008 // Data Acknowledge Enable
#define I2C_MCS_ADRACK 0x00000004 // Acknowledge Address
#define I2C_MCS_STOP 0x00000004 // Generate STOP
#define I2C_MCS_START 0x00000002 // Generate START
#define I2C_MCS_ERROR 0x00000002 // Error
#define I2C_MCS_RUN 0x00000001 // I2C Master Enable
//#define I2C_MSA_RS 0x01 //Sentido del esclavo
#define MAXRETRIES 5 // number of receive attempts before giving up
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MDR_DATA_M          0x000000FF  // This byte contains the data
#define I2C_MSA_SA_M            0x000000FE  // I2C Slave Address

#define GPIO_PORTB_AFSEL_R      (*((volatile uint32_t *)0x40059420))
#define GPIO_PORTB_ODR_R        (*((volatile uint32_t *)0x4005950C))
#define GPIO_PORTB_DIR_R        (*((volatile uint32_t *)0x40059400))
#define GPIO_PORTB_DEN_R        (*((volatile uint32_t *)0x4005951C))
#define GPIO_PORTB_PCTL_R       (*((volatile uint32_t *)0x4005952C))

/*El clculo del Time Period Register
 (TPR) se especifica en la pgina 1284
 Asumiendo un reloj de 16 MHz y un modo de operacin estndar (100 kbps):
*/
int TPR = 1;

//**Direcciones del VL53L0X
int VL53L0xAdd =0x29;///Direccn del VL53L0x
int SubRegIdx= 0x0014;
int AdreMin=0x01;




// Variables para manejar los valores del sensor
uint8_t error;
uint32_t i;
uint8_t data[10];
int Interrupcion=0;
uint8_t range[2];
uint8_t ranges[2];
uint16_t tmpuint16;

#define VL53L0X_MAKEUINT16(lsb, msb) (uint16_t)((((uint16_t)msb)<<8) + \
        (uint16_t)lsb)

//para las funciones de poleo
int failr=0;
int fail=0;

uint8_t lidar_id;

uint8_t stop_variable=0;

typedef enum
{
    CALIBRATION_TYPE_VHV,
    CALIBRATION_TYPE_PHASE
} calibration_type_t;

uint8_t cntr = 0x00;

//*** Funcique inicializa los relojes, el GPIO y el I2C0 ***
void I2C_Init(void){
    //RELOJ
    SYSCTL_RCGCI2C_R |= 0x0001;
    SYSCTL_RCGCGPIO_R |= 0x0002;
    while((SYSCTL_PRGPIO_R&0x0002) == 0){};
    //GPIO
    GPIO_PORTB_AFSEL_R |= 0x0C;
    GPIO_PORTB_ODR_R |= 0x08;
    GPIO_PORTB_DIR_R |= 0x0C;   //Activo al PB2 y al PB3 como OUTPUT
    GPIO_PORTB_DEN_R |= 0x0C;   //Activo la funcide PB3 y PB2
    GPIO_PORTB_PCTL_R|=0x00002200;

    //CONFIGURAC DEL MODULO I2C0
    I2C0_MCR_R = 0x00000010; // Habilitar funci para el I2C0
    I2C0_MTPR_R = TPR; // Se establece una velocidad estndar de 100kbps

}


void I2C_direct_read(uint8_t deviceAddress, uint8_t targetRegister) {

    for(i=0; i<175; i++);
    while (I2C0_MCS_R & I2C_MCS_BUSY) {};                            // wait for transmission done

    I2C0_MSA_R = (deviceAddress << 1) & I2C_MSA_SA_M;                // MSA[7:1] is slave address
    I2C0_MSA_R &= ~I2C_MSA_RS;                                      // MSA[0] is 0 for send

    I2C0_MDR_R = targetRegister & I2C_MDR_DATA_M;                    // prepare targetRegister

    I2C0_MCS_R = (I2C_MCS_START  |        // generate start/restart
                  I2C_MCS_RUN |        // generate enable
                  I2C_MCS_STOP);          // master stop

    for(i=0; i<175; i++);
    while (I2C0_MCS_R & I2C_MCS_BUSY) {};                            // wait for transmission done
}

void I2C_direct_write(uint8_t deviceAddress, uint8_t targetRegister) {

    for(i=0; i<175; i++);
    while (I2C0_MCS_R & I2C_MCS_BUSY) {};                            // wait for transmission done

    I2C0_MSA_R = (deviceAddress << 1) & I2C_MSA_SA_M;                // MSA[7:1] is slave address
    I2C0_MSA_R &= ~I2C_MSA_RS;                                      // MSA[0] is 0 for send

    I2C0_MDR_R = targetRegister & I2C_MDR_DATA_M;                    // prepare targetRegister

    I2C0_MCS_R = (I2C_MCS_START  |        // generate start/restart
                  I2C_MCS_RUN  );          // generate enable


    for(i=0; i<175; i++);
    while (I2C0_MCS_R & I2C_MCS_BUSY) {};                            // wait for transmission done
}


void I2C_read(uint8_t deviceAddress, uint8_t targetRegister, uint8_t *ReaData){


    I2C_direct_read(deviceAddress, targetRegister);
        // check error bits
        if((I2C0_MCS_R & (I2C_MCS_DATACK|I2C_MCS_ADRACK|I2C_MCS_ERROR)) != 0){
            failr=1;
        }
        do {
                        for(i=0; i<175; i++);
                        while (I2C0_MCS_R & I2C_MCS_BUSY) {};                // wait for I2C ready
                        I2C0_MSA_R = (deviceAddress << 1) & I2C_MSA_SA_M;    // MSA[7:1] is slave address
                        I2C0_MSA_R |= I2C_MSA_RS;                            // MSA[0] is 1 for receive

                        I2C0_MCS_R = (I2C_MCS_STOP  |                        // generate stop
                                      I2C_MCS_START |                        // generate start/restart
                                      I2C_MCS_RUN);                          // master enable
                        for(i=0; i<175; i++);
                        while (I2C0_MCS_R & I2C_MCS_BUSY) {};                // wait for transmission done

                    }                                                        // repeat if error
                    while (((I2C0_MCS_R & (I2C_MCS_ADRACK|I2C_MCS_ERROR)) != 0));
                    ReaData[0] = (I2C0_MDR_R & I2C_MDR_DATA_M);
}



void I2C_write(uint8_t deviceAddress, uint8_t targetRegister, uint8_t data){


    I2C_direct_write(deviceAddress, targetRegister);
        // check error bits
        if((I2C0_MCS_R & (I2C_MCS_DATACK|I2C_MCS_ADRACK|I2C_MCS_ERROR)) != 0){
            I2C0_MCS_R = I2C_MCS_STOP;                                   // stop transmission
            // return error bits if nonzero
            fail=1;
        }

        I2C0_MDR_R = data & I2C_MDR_DATA_M;                       // prepare data byte
                I2C0_MCS_R = (I2C_MCS_STOP |                                 // generate stop
                              I2C_MCS_RUN);                                  // master enable
                for(i=0;i<175;i++);
                while (I2C0_MCS_R & I2C_MCS_BUSY) {};                        // wait for transmission done

}

void VL53_WHOAMI(){

    I2C_read(0x29, 0xC0,&lidar_id);
}

void data_init(){

    uint8_t vhv_config_scl_sda=0;

    //Set 2v8 mode of power supply
    I2C_read(0x29,0x89,&vhv_config_scl_sda);
    vhv_config_scl_sda |=0x01;
    I2C_write(0x29,0x89,vhv_config_scl_sda);


    //Set i2c estandar mode
    I2C_write(0x29,0x88,0x00);
    I2C_write(0x29,0x80,0x01);
    I2C_write(0x29,0xFF,0x01);
    I2C_write(0x29,0x00,0x00);

    I2C_read(0x29,0x91,&stop_variable);

    I2C_write(0x29,0x00,0x01);
    I2C_write(0x29,0xFF,0x00);
    I2C_write(0x29,0x80,0x00);

}


void static_init(){

    //Load default tuning settings
    I2C_write(0x29,0xFF, 0x01);
    I2C_write(0x29,0x00, 0x00);
    I2C_write(0x29,0xFF, 0x00);
    I2C_write(0x29,0x09, 0x00);
    I2C_write(0x29,0x10, 0x00);
    I2C_write(0x29,0x11, 0x00);
    I2C_write(0x29,0x24, 0x01);
    I2C_write(0x29,0x25, 0xFF);
    I2C_write(0x29,0x75, 0x00);
    I2C_write(0x29,0xFF, 0x01);
    I2C_write(0x29,0x4E, 0x2C);
    I2C_write(0x29,0x48, 0x00);
    I2C_write(0x29,0x30, 0x20);
    I2C_write(0x29,0xFF, 0x00);
    I2C_write(0x29,0x30, 0x09);
    I2C_write(0x29,0x54, 0x00);
    I2C_write(0x29,0x31, 0x04);
    I2C_write(0x29,0x32, 0x03);
    I2C_write(0x29,0x40, 0x83);
    I2C_write(0x29,0x46, 0x25);
    I2C_write(0x29,0x60, 0x00);
    I2C_write(0x29,0x27, 0x00);
    I2C_write(0x29,0x50, 0x06);
    I2C_write(0x29,0x51, 0x00);
    I2C_write(0x29,0x52, 0x96);
    I2C_write(0x29,0x56, 0x08);
    I2C_write(0x29,0x57, 0x30);
    I2C_write(0x29,0x61, 0x00);
    I2C_write(0x29,0x62, 0x00);
    I2C_write(0x29,0x64, 0x00);
    I2C_write(0x29,0x65, 0x00);
    I2C_write(0x29,0x66, 0xA0);
    I2C_write(0x29,0xFF, 0x01);
    I2C_write(0x29,0x22, 0x32);
    I2C_write(0x29,0x47, 0x14);
    I2C_write(0x29,0x49, 0xFF);
    I2C_write(0x29,0x4A, 0x00);
    I2C_write(0x29,0xFF, 0x00);
    I2C_write(0x29,0x7A, 0x0A);
    I2C_write(0x29,0x7B, 0x00);
    I2C_write(0x29,0x78, 0x21);
    I2C_write(0x29,0xFF, 0x01);
    I2C_write(0x29,0x23, 0x34);
    I2C_write(0x29,0x42, 0x00);
    I2C_write(0x29,0x44, 0xFF);
    I2C_write(0x29,0x45, 0x26);
    I2C_write(0x29,0x46, 0x05);
    I2C_write(0x29,0x40, 0x40);
    I2C_write(0x29,0x0E, 0x06);
    I2C_write(0x29,0x20, 0x1A);
    I2C_write(0x29,0x43, 0x40);
    I2C_write(0x29,0xFF, 0x00);
    I2C_write(0x29,0x34, 0x03);
    I2C_write(0x29,0x35, 0x44);
    I2C_write(0x29,0xFF, 0x01);
    I2C_write(0x29,0x31, 0x04);
    I2C_write(0x29,0x4B, 0x09);
    I2C_write(0x29,0x4C, 0x05);
    I2C_write(0x29,0x4D, 0x04);
    I2C_write(0x29,0xFF, 0x00);
    I2C_write(0x29,0x44, 0x00);
    I2C_write(0x29,0x45, 0x20);
    I2C_write(0x29,0x47, 0x08);
    I2C_write(0x29,0x48, 0x28);
    I2C_write(0x29,0x67, 0x00);
    I2C_write(0x29,0x70, 0x04);
    I2C_write(0x29,0x71, 0x01);
    I2C_write(0x29,0x72, 0xFE);
    I2C_write(0x29,0x76, 0x00);
    I2C_write(0x29,0x77, 0x00);
    I2C_write(0x29,0xFF, 0x01);
    I2C_write(0x29,0x0D, 0x01);
    I2C_write(0x29,0xFF, 0x00);
    I2C_write(0x29,0x80, 0x01);
    I2C_write(0x29,0x01, 0xF8);
    I2C_write(0x29,0xFF, 0x01);
    I2C_write(0x29,0x8E, 0x01);
    I2C_write(0x29,0x00, 0x01);
    I2C_write(0x29,0xFF, 0x00);
    I2C_write(0x29,0x80, 0x00);

    //Configure Interrupt
    I2C_write(0x29,0x0A,0x04);
    //Jalar el pin de gpio a un nivel alto porque no lo hac por default

    uint8_t gpio_hv_mux_active_high = 0;
    I2C_read(0x29,0x84,&gpio_hv_mux_active_high);
    gpio_hv_mux_active_high &= ~0x01;
    I2C_write(0x29, 0x84,gpio_hv_mux_active_high);

    //Clear interrupt
    I2C_write(0x29,0x0B,0x01);

    //secuencia especfica de pasos

    I2C_write(0x29, 0x01,0x28 + 0x40 + 0x80);

}


void single_ref_calibration(calibration_type_t calib_type){
    uint8_t sysrange_start = 0;
        uint8_t sequence_config = 0;
        switch (calib_type)
        {
        case CALIBRATION_TYPE_VHV:
            sequence_config = 0x01;
            sysrange_start = 0x01 | 0x40;
            break;
        case CALIBRATION_TYPE_PHASE:
            sequence_config = 0x02;
            sysrange_start = 0x01 | 0x00;
            break;
        }

        I2C_write(0x29,0x01,sequence_config);

        I2C_write(0x29,0x00,sysrange_start);

        uint8_t interrupt_status = 0;

        do{
            I2C_read(0x29,0x13,&interrupt_status);
        }while ((interrupt_status & 0x07) == 0);

        I2C_write(0x29, 0x0B, 0x01);

        I2C_write(0x29,0x00, 0x00);

}

void ref_calibration(){
    single_ref_calibration(CALIBRATION_TYPE_VHV);
    single_ref_calibration(CALIBRATION_TYPE_PHASE);

    I2C_write(0x29,0x01,0x28 + 0x40 + 0x80);
}


