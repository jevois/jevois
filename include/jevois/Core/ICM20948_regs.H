// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2018 by Laurent Itti, the University of Southern
// California (USC), and iLab at USC. See http://iLab.usc.edu and http://jevois.org for information about this project.
//
// This file is part of the JeVois Smart Embedded Machine Vision Toolkit.  This program is free software; you can
// redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
// Foundation, version 2.  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details.  You should have received a copy of the GNU General Public License along with this program;
// if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// Contact information: Laurent Itti - 3641 Watt Way, HNB-07A - Los Angeles, CA 90089-2520 - USA.
// Tel: +1 213 740 3527 - itti@pollux.usc.edu - http://iLab.usc.edu - http://jevois.org
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*! \file */

// This code inspired by:

/***************************************************************************//**
 * file ICM20648.cpp
 *******************************************************************************
 * section License
 * <b>(C) Copyright 2017 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
 
/**************************************************************************//**
* @name Error Codes
* @{
******************************************************************************/
#define ICM20948_OK                                 0x0000   /**< No errors         */
#define ICM20948_ERROR_INVALID_DEVICE_ID            0x0001   /**< Invalid device ID */
/**@}*/
 
/**************************************************************************//**
* @name ICM20948 register banks
* @{
******************************************************************************/
#define ICM20948_BANK_0                  (0 << 7)     /**< Register bank 0 */
#define ICM20948_BANK_1                  (1 << 7)     /**< Register bank 1 */
#define ICM20948_BANK_2                  (2 << 7)     /**< Register bank 2 */
#define ICM20948_BANK_3                  (3 << 7)     /**< Register bank 3 */
/**@}*/
 
/**************************************************************************//**
* @name Register and associated bit definitions
* @{
******************************************************************************/
/***********************/
/* Bank 0 register map */
/***********************/
#define ICM20948_REG_WHO_AM_I            (ICM20948_BANK_0 | 0x00)    /**< Device ID register                                     */
 
#define ICM20948_REG_USER_CTRL           (ICM20948_BANK_0 | 0x03)    /**< User control register                                  */
#define ICM20948_BIT_DMP_EN              0x80                        /**< DMP enable bit                                         */
#define ICM20948_BIT_FIFO_EN             0x40                        /**< FIFO enable bit                                        */
#define ICM20948_BIT_I2C_MST_EN          0x20                        /**< I2C master I/F enable bit                              */
#define ICM20948_BIT_I2C_IF_DIS          0x10                        /**< Disable I2C, enable SPI bit                            */
#define ICM20948_BIT_DMP_RST             0x08                        /**< DMP module reset bit                                   */
#define ICM20948_BIT_DIAMOND_DMP_RST     0x04                        /**< SRAM module reset bit                                  */
 
#define ICM20948_REG_LP_CONFIG           (ICM20948_BANK_0 | 0x05)    /**< Low Power mode config register                         */
#define ICM20948_BIT_I2C_MST_CYCLE       0x40                        /**< I2C master cycle mode enable                           */
#define ICM20948_BIT_ACCEL_CYCLE         0x20                        /**< Accelerometer cycle mode enable                        */
#define ICM20948_BIT_GYRO_CYCLE          0x10                        /**< Gyroscope cycle mode enable                            */
 
#define ICM20948_REG_PWR_MGMT_1          (ICM20948_BANK_0 | 0x06)    /**< Power Management 1 register                            */
#define ICM20948_BIT_H_RESET             0x80                        /**< Device reset bit                                       */
#define ICM20948_BIT_SLEEP               0x40                        /**< Sleep mode enable bit                                  */
#define ICM20948_BIT_LP_EN               0x20                        /**< Low Power feature enable bit                           */
#define ICM20948_BIT_TEMP_DIS            0x08                        /**< Temperature sensor disable bit                         */
#define ICM20948_BIT_CLK_PLL             0x01                        /**< Auto clock source selection setting                    */
 
#define ICM20948_REG_PWR_MGMT_2          (ICM20948_BANK_0 | 0x07)    /**< Power Management 2 register                            */
#define ICM20948_BIT_PWR_ACCEL_STBY      0x38                        /**< Disable accelerometer                                  */
#define ICM20948_BIT_PWR_GYRO_STBY       0x07                        /**< Disable gyroscope                                      */
#define ICM20948_BIT_PWR_ALL_OFF         0x7F                        /**< Disable both accel and gyro                            */
 
#define ICM20948_REG_INT_PIN_CFG         (ICM20948_BANK_0 | 0x0F)    /**< Interrupt Pin Configuration register                   */
#define ICM20948_BIT_INT_ACTL            0x80                        /**< Active low setting bit                                 */
#define ICM20948_BIT_INT_OPEN            0x40                        /**< Open collector onfiguration bit                        */
#define ICM20948_BIT_INT_LATCH_EN        0x20                        /**< Latch enable bit                                       */
 
#define ICM20948_REG_INT_ENABLE          (ICM20948_BANK_0 | 0x10)    /**< Interrupt Enable register                              */
#define ICM20948_BIT_WOM_INT_EN          0x08                        /**< Wake-up On Motion enable bit                           */
#define ICM20948_BIT_DMP_INT_EN          0x02

#define ICM20948_REG_INT_ENABLE_1        (ICM20948_BANK_0 | 0x11)    /**< Interrupt Enable 1 register                            */
#define ICM20948_BIT_RAW_DATA_0_RDY_EN   0x01                        /**< Raw data ready interrupt enable bit                    */
#define ICM20948_BIT_RAW_DATA_1_RDY_EN   0x02                        /**< Raw data ready interrupt enable bit                    */
#define ICM20948_BIT_RAW_DATA_2_RDY_EN   0x04                        /**< Raw data ready interrupt enable bit                    */
#define ICM20948_BIT_RAW_DATA_3_RDY_EN   0x08                        /**< Raw data ready interrupt enable bit                    */
 
#define ICM20948_REG_INT_ENABLE_2        (ICM20948_BANK_0 | 0x12)    /**< Interrupt Enable 2 register                            */
#define ICM20948_BIT_FIFO_OVERFLOW_EN_0  0x01                        /**< FIFO overflow interrupt enable bit                     */
 
#define ICM20948_REG_INT_ENABLE_3        (ICM20948_BANK_0 | 0x13)    /**< Interrupt Enable 3 register                            */

#define ICM20948_REG_I2C_MST_STATUS      (ICM20948_BANK_0 | 0x17)    /**< Status of I2C master                                   */
#define ICM20948_BIT_PASS_THROUGH        0x80
#define ICM20948_BIT_SLV4_DONE           0x40
#define ICM20948_BIT_LOST_ARB            0x20
#define ICM20948_BIT_SLV4_NACK           0x10
#define ICM20948_BIT_SLV3_NACK           0x08
#define ICM20948_BIT_SLV2_NACK           0x04
#define ICM20948_BIT_SLV1_NACK           0x02
#define ICM20948_BIT_SLV0_NACK           0x01

#define ICM20948_REG_DMP_INT_STATUS      (ICM20948_BANK_0 | 0x18)
#define ICM20948_BIT_WAKE_ON_MOTION_INT  0x08
#define ICM20948_BIT_MSG_DMP_INT         0x0002
#define ICM20948_BIT_MSG_DMP_INT_0       0x0100  // CI Command
#define ICM20948_BIT_MSG_DMP_INT_2       0x0200  // CIM Command - SMD
#define ICM20948_BIT_MSG_DMP_INT_3       0x0400  // CIM Command - Pedometer
#define ICM20948_BIT_MSG_DMP_INT_4       0x1000  // CIM Command - Pedometer binning
#define ICM20948_BIT_MSG_DMP_INT_5       0x2000  // CIM Command - Bring To See Gesture
#define ICM20948_BIT_MSG_DMP_INT_6       0x4000  // CIM Command - Look To See Gesture

#define ICM20948_REG_INT_STATUS          (ICM20948_BANK_0 | 0x19)    /**< Interrupt Status register                              */
#define ICM20948_BIT_WOM_INT             0x08                        /**< Wake-up on motion interrupt occured bit                */
#define ICM20948_BIT_PLL_RDY             0x04                        /**< PLL ready interrupt occured bit                        */
#define ICM20948_BIT_DMP_INT             0x02 

#define ICM20948_REG_INT_STATUS_1        (ICM20948_BANK_0 | 0x1A)    /**< Interrupt Status 1 register                            */
#define ICM20948_BIT_RAW_DATA_0_RDY_INT  0x01                        /**< Raw data ready interrupt occured bit                   */
 
#define ICM20948_REG_INT_STATUS_2        (ICM20948_BANK_0 | 0x1B)    /**< Interrupt Status 2 register                            */

#define ICM20948_REG_SINGLE_FIFO_PRIORITY_SEL        (ICM20948_BANK_0 | 0x26)	

#define ICM20948_REG_ACCEL_XOUT_H_SH     (ICM20948_BANK_0 | 0x2D)    /**< Accelerometer X-axis data high byte                    */
#define ICM20948_REG_ACCEL_XOUT_L_SH     (ICM20948_BANK_0 | 0x2E)    /**< Accelerometer X-axis data low byte                     */
#define ICM20948_REG_ACCEL_YOUT_H_SH     (ICM20948_BANK_0 | 0x2F)    /**< Accelerometer Y-axis data high byte                    */
#define ICM20948_REG_ACCEL_YOUT_L_SH     (ICM20948_BANK_0 | 0x30)    /**< Accelerometer Y-axis data low byte                     */
#define ICM20948_REG_ACCEL_ZOUT_H_SH     (ICM20948_BANK_0 | 0x31)    /**< Accelerometer Z-axis data high byte                    */
#define ICM20948_REG_ACCEL_ZOUT_L_SH     (ICM20948_BANK_0 | 0x32)    /**< Accelerometer Z-axis data low byte                     */
 
#define ICM20948_REG_GYRO_XOUT_H_SH      (ICM20948_BANK_0 | 0x33)    /**< Gyroscope X-axis data high byte                        */
#define ICM20948_REG_GYRO_XOUT_L_SH      (ICM20948_BANK_0 | 0x34)    /**< Gyroscope X-axis data low byte                         */
#define ICM20948_REG_GYRO_YOUT_H_SH      (ICM20948_BANK_0 | 0x35)    /**< Gyroscope Y-axis data high byte                        */
#define ICM20948_REG_GYRO_YOUT_L_SH      (ICM20948_BANK_0 | 0x36)    /**< Gyroscope Y-axis data low byte                         */
#define ICM20948_REG_GYRO_ZOUT_H_SH      (ICM20948_BANK_0 | 0x37)    /**< Gyroscope Z-axis data high byte                        */
#define ICM20948_REG_GYRO_ZOUT_L_SH      (ICM20948_BANK_0 | 0x38)    /**< Gyroscope Z-axis data low byte                         */
 
#define ICM20948_REG_TEMPERATURE_H       (ICM20948_BANK_0 | 0x39)    /**< Temperature data high byte                             */
#define ICM20948_REG_TEMPERATURE_L       (ICM20948_BANK_0 | 0x3A)    /**< Temperature data low byte                              */
#define ICM20948_REG_TEMP_CONFIG         (ICM20948_BANK_0 | 0x53)    /**< Temperature Configuration register                     */
 
#define ICM20948_REG_FIFO_EN_1           (ICM20948_BANK_0 | 0x66)    /**< FIFO Enable 1 register                                 */
#define ICM20948_BIT_SLV_0_FIFO_EN       0x01

#define ICM20948_REG_FIFO_EN_2           (ICM20948_BANK_0 | 0x67)    /**< FIFO Enable 2 register                                 */
#define ICM20948_BIT_ACCEL_FIFO_EN       0x10                        /**< Enable writing acceleration data to FIFO bit           */
#define ICM20948_BITS_GYRO_FIFO_EN       0x0E                        /**< Enable writing gyroscope data to FIFO bit              */
 
#define ICM20948_REG_FIFO_RST            (ICM20948_BANK_0 | 0x68)    /**< FIFO Reset register                                    */
#define ICM20948_REG_FIFO_MODE           (ICM20948_BANK_0 | 0x69)    /**< FIFO Mode register                                     */
 
#define ICM20948_REG_FIFO_COUNT_H        (ICM20948_BANK_0 | 0x70)    /**< FIFO data count high byte                              */
#define ICM20948_REG_FIFO_COUNT_L        (ICM20948_BANK_0 | 0x71)    /**< FIFO data count low byte                               */
#define ICM20948_REG_FIFO_R_W            (ICM20948_BANK_0 | 0x72)    /**< FIFO Read/Write register                               */
 
#define ICM20948_REG_DATA_RDY_STATUS     (ICM20948_BANK_0 | 0x74)    /**< Data Ready Status register                             */
#define ICM20948_BIT_RAW_DATA_0_RDY      0x01                        /**< Raw Data Ready bit                                     */
 
#define ICM20948_REG_FIFO_CFG            (ICM20948_BANK_0 | 0x76)    /**< FIFO Configuration register                            */
#define ICM20948_BIT_MULTI_FIFO_CFG      0x01                        /**< Interrupt status for each sensor is required           */
#define ICM20948_BIT_SINGLE_FIFO_CFG     0x00                        /**< Interrupt status for only a single sensor is required  */

#define ICM20948_REG_MEM_START_ADDR      (ICM20948_BANK_0 | 0x7c)    /**< MEMs start address                                     */
#define ICM20948_REG_MEM_R_W             (ICM20948_BANK_0 | 0x7d)    /**< MEMs data read/write                                   */
#define ICM20948_REG_MEM_BANK_SEL        (ICM20948_BANK_0 | 0x7e)    /**< MEMs bank select                                       */

/***********************/
/* Bank 1 register map */
/***********************/
#define ICM20948_REG_XA_OFFSET_H         (ICM20948_BANK_1 | 0x14)    /**< Acceleration sensor X-axis offset cancellation high byte  */
#define ICM20948_REG_XA_OFFSET_L         (ICM20948_BANK_1 | 0x15)    /**< Acceleration sensor X-axis offset cancellation low byte   */
#define ICM20948_REG_YA_OFFSET_H         (ICM20948_BANK_1 | 0x17)    /**< Acceleration sensor Y-axis offset cancellation high byte  */
#define ICM20948_REG_YA_OFFSET_L         (ICM20948_BANK_1 | 0x18)    /**< Acceleration sensor Y-axis offset cancellation low byte   */
#define ICM20948_REG_ZA_OFFSET_H         (ICM20948_BANK_1 | 0x1A)    /**< Acceleration sensor Z-axis offset cancellation high byte  */
#define ICM20948_REG_ZA_OFFSET_L         (ICM20948_BANK_1 | 0x1B)    /**< Acceleration sensor Z-axis offset cancellation low byte   */
 
#define ICM20948_REG_TIMEBASE_CORR_PLL   (ICM20948_BANK_1 | 0x28)    /**< PLL Timebase Correction register                          */
 
/***********************/
/* Bank 2 register map */
/***********************/
#define ICM20948_REG_GYRO_SMPLRT_DIV     (ICM20948_BANK_2 | 0x00)    /**< Gyroscope Sample Rate Divider regiser      */
 
#define ICM20948_REG_GYRO_CONFIG_1       (ICM20948_BANK_2 | 0x01)    /**< Gyroscope Configuration 1 register         */
#define ICM20948_BIT_GYRO_FCHOICE        0x01                        /**< Gyro Digital Low-Pass Filter enable bit    */
#define ICM20948_SHIFT_GYRO_FS_SEL       1                           /**< Gyro Full Scale Select bit shift           */
#define ICM20948_SHIFT_GYRO_DLPCFG       3                           /**< Gyro DLPF Config bit shift                 */
#define ICM20948_MASK_GYRO_FULLSCALE     0x06                        /**< Gyro Full Scale Select bitmask             */
#define ICM20948_MASK_GYRO_BW            0x39                        /**< Gyro Bandwidth Select bitmask              */
#define ICM20948_GYRO_FULLSCALE_250DPS   (0x00 << ICM20948_SHIFT_GYRO_FS_SEL)    /**< Gyro Full Scale = 250 deg/sec  */
#define ICM20948_GYRO_FULLSCALE_500DPS   (0x01 << ICM20948_SHIFT_GYRO_FS_SEL)    /**< Gyro Full Scale = 500 deg/sec  */
#define ICM20948_GYRO_FULLSCALE_1000DPS  (0x02 << ICM20948_SHIFT_GYRO_FS_SEL)    /**< Gyro Full Scale = 1000 deg/sec */
#define ICM20948_GYRO_FULLSCALE_2000DPS  (0x03 << ICM20948_SHIFT_GYRO_FS_SEL)    /**< Gyro Full Scale = 2000 deg/sec */
#define ICM20948_GYRO_BW_12100HZ         (0x00 << ICM20948_SHIFT_GYRO_DLPCFG)                                     /**< Gyro Bandwidth = 12100 Hz */
#define ICM20948_GYRO_BW_360HZ           ( (0x07 << ICM20948_SHIFT_GYRO_DLPCFG) | ICM20948_BIT_GYRO_FCHOICE)      /**< Gyro Bandwidth = 360 Hz   */
#define ICM20948_GYRO_BW_200HZ           ( (0x00 << ICM20948_SHIFT_GYRO_DLPCFG) | ICM20948_BIT_GYRO_FCHOICE)      /**< Gyro Bandwidth = 200 Hz   */
#define ICM20948_GYRO_BW_150HZ           ( (0x01 << ICM20948_SHIFT_GYRO_DLPCFG) | ICM20948_BIT_GYRO_FCHOICE)      /**< Gyro Bandwidth = 150 Hz   */
#define ICM20948_GYRO_BW_120HZ           ( (0x02 << ICM20948_SHIFT_GYRO_DLPCFG) | ICM20948_BIT_GYRO_FCHOICE)      /**< Gyro Bandwidth = 120 Hz   */
#define ICM20948_GYRO_BW_51HZ            ( (0x03 << ICM20948_SHIFT_GYRO_DLPCFG) | ICM20948_BIT_GYRO_FCHOICE)      /**< Gyro Bandwidth = 51 Hz    */
#define ICM20948_GYRO_BW_24HZ            ( (0x04 << ICM20948_SHIFT_GYRO_DLPCFG) | ICM20948_BIT_GYRO_FCHOICE)      /**< Gyro Bandwidth = 24 Hz    */
#define ICM20948_GYRO_BW_12HZ            ( (0x05 << ICM20948_SHIFT_GYRO_DLPCFG) | ICM20948_BIT_GYRO_FCHOICE)      /**< Gyro Bandwidth = 12 Hz    */
#define ICM20948_GYRO_BW_6HZ             ( (0x06 << ICM20948_SHIFT_GYRO_DLPCFG) | ICM20948_BIT_GYRO_FCHOICE)      /**< Gyro Bandwidth = 6 Hz     */
 
#define ICM20948_REG_GYRO_CONFIG_2       (ICM20948_BANK_2 | 0x02)    /**< Gyroscope Configuration 2 register                     */
#define ICM20948_BIT_GYRO_CTEN           0x38                        /**< Gyroscope Self-Test Enable bits                        */
 
#define ICM20948_REG_XG_OFFS_USRH        (ICM20948_BANK_2 | 0x03)    /**< Gyroscope sensor X-axis offset cancellation high byte  */
#define ICM20948_REG_XG_OFFS_USRL        (ICM20948_BANK_2 | 0x04)    /**< Gyroscope sensor X-axis offset cancellation low byte   */
#define ICM20948_REG_YG_OFFS_USRH        (ICM20948_BANK_2 | 0x05)    /**< Gyroscope sensor Y-axis offset cancellation high byte  */
#define ICM20948_REG_YG_OFFS_USRL        (ICM20948_BANK_2 | 0x06)    /**< Gyroscope sensor Y-axis offset cancellation low byte   */
#define ICM20948_REG_ZG_OFFS_USRH        (ICM20948_BANK_2 | 0x07)    /**< Gyroscope sensor Z-axis offset cancellation high byte  */
#define ICM20948_REG_ZG_OFFS_USRL        (ICM20948_BANK_2 | 0x08)    /**< Gyroscope sensor Z-axis offset cancellation low byte   */
 
#define ICM20948_REG_ODR_ALIGN_EN        (ICM20948_BANK_2 | 0x09)    /**< Output Data Rate start time alignment                  */
 
#define ICM20948_REG_ACCEL_SMPLRT_DIV_1  (ICM20948_BANK_2 | 0x10)    /**< Acceleration Sensor Sample Rate Divider 1 register     */
#define ICM20948_REG_ACCEL_SMPLRT_DIV_2  (ICM20948_BANK_2 | 0x11)    /**< Acceleration Sensor Sample Rate Divider 2 register     */
 
#define ICM20948_REG_ACCEL_INTEL_CTRL    (ICM20948_BANK_2 | 0x12)    /**< Accelerometer Hardware Intelligence Control register   */
#define ICM20948_BIT_ACCEL_INTEL_EN      0x02                        /**< Wake-up On Motion enable bit                           */
#define ICM20948_BIT_ACCEL_INTEL_MODE    0x01                        /**< WOM algorithm selection bit                            */
 
#define ICM20948_REG_ACCEL_WOM_THR       (ICM20948_BANK_2 | 0x13)    /**< Wake-up On Motion Threshold register                   */
 
#define ICM20948_REG_ACCEL_CONFIG        (ICM20948_BANK_2 | 0x14)    /**< Accelerometer Configuration register                   */
#define ICM20948_BIT_ACCEL_FCHOICE       0x01                        /**< Accel Digital Low-Pass Filter enable bit               */
#define ICM20948_SHIFT_ACCEL_FS          1                           /**< Accel Full Scale Select bit shift                      */
#define ICM20948_SHIFT_ACCEL_DLPCFG      3                           /**< Accel DLPF Config bit shift                            */
#define ICM20948_MASK_ACCEL_FULLSCALE    0x06                        /**< Accel Full Scale Select bitmask                        */
#define ICM20948_MASK_ACCEL_BW           0x39                        /**< Accel Bandwidth Select bitmask                         */
#define ICM20948_ACCEL_FULLSCALE_2G      (0x00 << ICM20948_SHIFT_ACCEL_FS)    /**< Accel Full Scale = 2 g  */
#define ICM20948_ACCEL_FULLSCALE_4G      (0x01 << ICM20948_SHIFT_ACCEL_FS)    /**< Accel Full Scale = 4 g  */
#define ICM20948_ACCEL_FULLSCALE_8G      (0x02 << ICM20948_SHIFT_ACCEL_FS)    /**< Accel Full Scale = 8 g  */
#define ICM20948_ACCEL_FULLSCALE_16G     (0x03 << ICM20948_SHIFT_ACCEL_FS)    /**< Accel Full Scale = 16 g */
#define ICM20948_ACCEL_BW_1210HZ         (0x00 << ICM20948_SHIFT_ACCEL_DLPCFG)                                    /**< Accel Bandwidth = 1210 Hz  */
#define ICM20948_ACCEL_BW_470HZ          ( (0x07 << ICM20948_SHIFT_ACCEL_DLPCFG) | ICM20948_BIT_ACCEL_FCHOICE)    /**< Accel Bandwidth = 470 Hz   */
#define ICM20948_ACCEL_BW_246HZ          ( (0x00 << ICM20948_SHIFT_ACCEL_DLPCFG) | ICM20948_BIT_ACCEL_FCHOICE)    /**< Accel Bandwidth = 246 Hz   */
#define ICM20948_ACCEL_BW_111HZ          ( (0x02 << ICM20948_SHIFT_ACCEL_DLPCFG) | ICM20948_BIT_ACCEL_FCHOICE)    /**< Accel Bandwidth = 111 Hz   */
#define ICM20948_ACCEL_BW_50HZ           ( (0x03 << ICM20948_SHIFT_ACCEL_DLPCFG) | ICM20948_BIT_ACCEL_FCHOICE)    /**< Accel Bandwidth = 50 Hz    */
#define ICM20948_ACCEL_BW_24HZ           ( (0x04 << ICM20948_SHIFT_ACCEL_DLPCFG) | ICM20948_BIT_ACCEL_FCHOICE)    /**< Accel Bandwidth = 24 Hz    */
#define ICM20948_ACCEL_BW_12HZ           ( (0x05 << ICM20948_SHIFT_ACCEL_DLPCFG) | ICM20948_BIT_ACCEL_FCHOICE)    /**< Accel Bandwidth = 12 Hz    */
#define ICM20948_ACCEL_BW_6HZ            ( (0x06 << ICM20948_SHIFT_ACCEL_DLPCFG) | ICM20948_BIT_ACCEL_FCHOICE)    /**< Accel Bandwidth = 6 Hz     */
 
#define ICM20948_REG_ACCEL_CONFIG_2      (ICM20948_BANK_2 | 0x15)    /**< Accelerometer Configuration 2 register              */
#define ICM20948_BIT_ACCEL_CTEN          0x1C                        /**< Accelerometer Self-Test Enable bits                 */

/***********************/
/* Bank 3 register map */
/***********************/
#define ICM20948_REG_I2C_MST_ODR_CONFIG  (ICM20948_BANK_3 | 0x00)    /**< I2C Master Output Data Rate Configuration register  */
 
#define ICM20948_REG_I2C_MST_CTRL        (ICM20948_BANK_3 | 0x01)    /**< I2C Master Control register                         */
#define ICM20948_BIT_I2C_MST_P_NSR       0x10                        /**< Stop between reads enabling bit                     */
 
#define ICM20948_REG_I2C_MST_DELAY_CTRL  (ICM20948_BANK_3 | 0x02)    /**< I2C Master Delay Control register                   */
#define ICM20948_BIT_SLV0_DLY_EN         0x01                        /**< I2C Slave0 Delay Enable bit                         */
#define ICM20948_BIT_SLV1_DLY_EN         0x02                        /**< I2C Slave1 Delay Enable bit                         */
#define ICM20948_BIT_SLV2_DLY_EN         0x04                        /**< I2C Slave2 Delay Enable bit                         */
#define ICM20948_BIT_SLV3_DLY_EN         0x08                        /**< I2C Slave3 Delay Enable bit                         */
 
#define ICM20948_REG_I2C_SLV0_ADDR       (ICM20948_BANK_3 | 0x03)    /**< I2C Slave0 Physical Address register                */
#define ICM20948_REG_I2C_SLV0_REG        (ICM20948_BANK_3 | 0x04)    /**< I2C Slave0 Register Address register                */
#define ICM20948_REG_I2C_SLV0_CTRL       (ICM20948_BANK_3 | 0x05)    /**< I2C Slave0 Control register                         */
#define ICM20948_REG_I2C_SLV0_DO         (ICM20948_BANK_3 | 0x06)    /**< I2C Slave0 Data Out register                        */
 
#define ICM20948_REG_I2C_SLV1_ADDR       (ICM20948_BANK_3 | 0x07)    /**< I2C Slave1 Physical Address register                */
#define ICM20948_REG_I2C_SLV1_REG        (ICM20948_BANK_3 | 0x08)    /**< I2C Slave1 Register Address register                */
#define ICM20948_REG_I2C_SLV1_CTRL       (ICM20948_BANK_3 | 0x09)    /**< I2C Slave1 Control register                         */
#define ICM20948_REG_I2C_SLV1_DO         (ICM20948_BANK_3 | 0x0A)    /**< I2C Slave1 Data Out register                        */
 
#define ICM20948_REG_I2C_SLV2_ADDR       (ICM20948_BANK_3 | 0x0B)    /**< I2C Slave2 Physical Address register                */
#define ICM20948_REG_I2C_SLV2_REG        (ICM20948_BANK_3 | 0x0C)    /**< I2C Slave2 Register Address register                */
#define ICM20948_REG_I2C_SLV2_CTRL       (ICM20948_BANK_3 | 0x0D)    /**< I2C Slave2 Control register                         */
#define ICM20948_REG_I2C_SLV2_DO         (ICM20948_BANK_3 | 0x0E)    /**< I2C Slave2 Data Out register                        */
 
#define ICM20948_REG_I2C_SLV3_ADDR       (ICM20948_BANK_3 | 0x0F)    /**< I2C Slave3 Physical Address register                */
#define ICM20948_REG_I2C_SLV3_REG        (ICM20948_BANK_3 | 0x10)    /**< I2C Slave3 Register Address register                */
#define ICM20948_REG_I2C_SLV3_CTRL       (ICM20948_BANK_3 | 0x11)    /**< I2C Slave3 Control register                         */
#define ICM20948_REG_I2C_SLV3_DO         (ICM20948_BANK_3 | 0x12)    /**< I2C Slave3 Data Out register                        */
 
#define ICM20948_REG_I2C_SLV4_ADDR       (ICM20948_BANK_3 | 0x13)    /**< I2C Slave4 Physical Address register                */
#define ICM20948_REG_I2C_SLV4_REG        (ICM20948_BANK_3 | 0x14)    /**< I2C Slave4 Register Address register                */
#define ICM20948_REG_I2C_SLV4_CTRL       (ICM20948_BANK_3 | 0x15)    /**< I2C Slave4 Control register                         */
#define ICM20948_REG_I2C_SLV4_DO         (ICM20948_BANK_3 | 0x16)    /**< I2C Slave4 Data Out register                        */
#define ICM20948_REG_I2C_SLV4_DI         (ICM20948_BANK_3 | 0x17)    /**< I2C Slave4 Data In register                         */
 
#define ICM20948_BIT_I2C_SLV4_INT_EN     0x40                        /**< I2C Slave 4 interrupt enable */
#define ICM20948_BIT_I2C_SLV4_REG_DIS    0x20                        /**< I2C Slave 4 no register address */

#define ICM20948_BIT_I2C_SLV_EN          0x80                        /**< I2C Slave Enable bit                                */
#define ICM20948_BIT_I2C_BYTE_SW         0x40                        /**< I2C Slave Byte Swap enable bit                      */
#define ICM20948_BIT_I2C_REG_DIS         0x20                        /**< I2C Slave Do Not Write Register Value bit           */
#define ICM20948_BIT_I2C_GRP             0x10                        /**< I2C Slave Group bit                                 */
#define ICM20948_BIT_I2C_READ            0x80                        /**< I2C Slave R/W bit                                   */

/* Register common for all banks */
#define ICM20948_REG_BANK_SEL            0x7F                        /**< Bank Select register                                */
 
#define ICM20648_DEVICE_ID               0xE0                        /**< ICM20948 Device ID value                            */
#define ICM20948_DEVICE_ID               0xEA                        /**< ICM20948 Device ID value                            */


// ####################################################################################################
/* AK09916 definitions */

/* AK09916 register definition */
#define REG_AK09916_WIA          0x01
#define VAL_AK09916_WIA          0x09

#define REG_AK09916_STATUS1      0x10
#define BIT_AK09916_STATUS1_DOR  0x02 /*!< Data overrun */
#define BIT_AK09916_STATUS1_DRDY 0x01 /*!< Data ready */

#define REG_AK09916_HXL          0x11
#define REG_AK09916_HXH          0x12
#define REG_AK09916_HYL          0x13
#define REG_AK09916_HYH          0x14
#define REG_AK09916_HZL          0x15
#define REG_AK09916_HZH          0x16


#define REG_AK09916_STATUS2      0x18
#define BIT_AK09916_STATUS2_HOFL 0x08 /*!< Magnetic sensor overflow */

#define REG_AK09916_CNTL2       0x31
#define VAL_AK09916_CNTL2_PD    0x00  /*!< AK power down */
#define VAL_AK09916_CNTL2_SNGL  0x01  /*!< AK single measurement */
#define VAL_AK09916_CNTL2_MOD1  0x02  /*!< AK continuous measurement mode 1 (10Hz) */
#define VAL_AK09916_CNTL2_MOD2  0x04  /*!< AK continuous measurement mode 1 (20Hz) */
#define VAL_AK09916_CNTL2_MOD3  0x06  /*!< AK continuous measurement mode 1 (50Hz) */
#define VAL_AK09916_CNTL2_MOD4  0x08  /*!< AK continuous measurement mode 1 (100Hz) */

#define REG_AK09916_CNTL3       0x32
#define BIT_AK09916_CNTL3_SRST  0x01  /*!< Soft reset */

#define REG_AK09916_TST1        0x33
#define REG_AK09916_TST2        0x34

#define DATA_AK09916_SCALE      161061273

#define COMPASS_SLAVEADDR       0x0C /*!< I2C address of the AK09916 */


/**@}*/
 
/** @endcond */

// ####################################################################################################
/* DMP definitions for dmp3a.20x48-0.4.1 */

#define DMP_CFG_FIFO_SIZE                   (4222)

// data output control
#define DMP_DATA_OUT_CTL1			(4 * 16)
#define DMP_DATA_OUT_CTL2			(4 * 16 + 2)
#define DMP_DATA_INTR_CTL			(4 * 16 + 12)
#define DMP_FIFO_WATERMARK			(31 * 16 + 14)

// motion event control
#define DMP_MOTION_EVENT_CTL		(4 * 16 + 14)

// indicates to DMP which sensors are available
/*	1: gyro samples available
2: accel samples available
8: secondary samples available	*/
#define DMP_DATA_RDY_STATUS			(8 * 16 + 10)

// batch mode
#define DMP_BM_BATCH_CNTR			(27 * 16)
#define DMP_BM_BATCH_THLD			(19 * 16 + 12)
#define DMP_BM_BATCH_MASK			(21 * 16 + 14)

// sensor output data rate
#define DMP_ODR_ACCEL				(11 * 16 + 14)
#define DMP_ODR_GYRO				(11 * 16 + 10)
#define DMP_ODR_CPASS				(11 * 16 +  6)
#define DMP_ODR_ALS					(11 * 16 +  2)
#define DMP_ODR_QUAT6				(10 * 16 + 12)
#define DMP_ODR_QUAT9				(10 * 16 +  8)
#define DMP_ODR_PQUAT6				(10 * 16 +  4)
#define DMP_ODR_GEOMAG				(10 * 16 +  0)
#define DMP_ODR_PRESSURE			(11 * 16 + 12)
#define DMP_ODR_GYRO_CALIBR			(11 * 16 +  8)
#define DMP_ODR_CPASS_CALIBR		(11 * 16 +  4)

// sensor output data rate counter
#define DMP_ODR_CNTR_ACCEL			(9 * 16 + 14)
#define DMP_ODR_CNTR_GYRO			(9 * 16 + 10)
#define DMP_ODR_CNTR_CPASS			(9 * 16 +  6)
#define DMP_ODR_CNTR_ALS			(9 * 16 +  2)
#define DMP_ODR_CNTR_QUAT6			(8 * 16 + 12)
#define DMP_ODR_CNTR_QUAT9			(8 * 16 +  8)
#define DMP_ODR_CNTR_PQUAT6			(8 * 16 +  4)
#define DMP_ODR_CNTR_GEOMAG			(8 * 16 +  0)
#define DMP_ODR_CNTR_PRESSURE		(9 * 16 + 12)
#define DMP_ODR_CNTR_GYRO_CALIBR	(9 * 16 +  8)
#define DMP_ODR_CNTR_CPASS_CALIBR	(9 * 16 +  4)

// mounting matrix
#define DMP_CPASS_MTX_00            (23 * 16)
#define DMP_CPASS_MTX_01            (23 * 16 + 4)
#define DMP_CPASS_MTX_02            (23 * 16 + 8)
#define DMP_CPASS_MTX_10            (23 * 16 + 12)
#define DMP_CPASS_MTX_11            (24 * 16)
#define DMP_CPASS_MTX_12            (24 * 16 + 4)
#define DMP_CPASS_MTX_20            (24 * 16 + 8)
#define DMP_CPASS_MTX_21            (24 * 16 + 12)
#define DMP_CPASS_MTX_22            (25 * 16)

#define DMP_GYRO_SF					(19 * 16)
#define DMP_ACCEL_FB_GAIN			(34 * 16)
#define DMP_ACCEL_ONLY_GAIN			(16 * 16 + 12)

// bias calibration
#define DMP_GYRO_BIAS_X				(139 * 16 +  4)
#define DMP_GYRO_BIAS_Y				(139 * 16 +  8)
#define DMP_GYRO_BIAS_Z				(139 * 16 + 12)
#define DMP_GYRO_ACCURACY			(138 * 16 +  2)
#define DMP_GYRO_BIAS_SET			(138 * 16 +  6)
#define DMP_GYRO_LAST_TEMPR			(134 * 16)
#define DMP_GYRO_SLOPE_X			( 78 * 16 +  4)
#define DMP_GYRO_SLOPE_Y			( 78 * 16 +  8)
#define DMP_GYRO_SLOPE_Z			( 78 * 16 + 12)

#define DMP_ACCEL_BIAS_X            (110 * 16 +  4)
#define DMP_ACCEL_BIAS_Y            (110 * 16 +  8)
#define DMP_ACCEL_BIAS_Z            (110 * 16 + 12)
#define DMP_ACCEL_ACCURACY			(97 * 16)
#define DMP_ACCEL_CAL_RESET			(77 * 16)
#define DMP_ACCEL_VARIANCE_THRESH	(93 * 16)
#define DMP_ACCEL_CAL_RATE			(94 * 16 + 4)
#define DMP_ACCEL_PRE_SENSOR_DATA	(97 * 16 + 4)
#define DMP_ACCEL_COVARIANCE		(101 * 16 + 8)
#define DMP_ACCEL_ALPHA_VAR			(91 * 16)
#define DMP_ACCEL_A_VAR				(92 * 16)
#define DMP_ACCEL_CAL_INIT			(94 * 16 + 2)
#define DMP_ACCEL_CAL_SCALE_COVQ_IN_RANGE	(194 * 16)
#define DMP_ACCEL_CAL_SCALE_COVQ_OUT_RANGE	(195 * 16)
#define DMP_ACCEL_CAL_TEMPERATURE_SENSITIVITY	(194 * 16 + 4)
#define DMP_ACCEL_CAL_TEMPERATURE_OFFSET_TRIM	(194 * 16 + 12)

#define DMP_CPASS_BIAS_X            (126 * 16 +  4)
#define DMP_CPASS_BIAS_Y            (126 * 16 +  8)
#define DMP_CPASS_BIAS_Z            (126 * 16 + 12)
#define DMP_CPASS_ACCURACY			(37 * 16)
#define DMP_CPASS_BIAS_SET			(34 * 16 + 14)
#define DMP_MAR_MODE				(37 * 16 + 2)
#define DMP_CPASS_COVARIANCE		(115 * 16)
#define DMP_CPASS_COVARIANCE_CUR	(118 * 16 +  8)
#define DMP_CPASS_REF_MAG_3D		(122 * 16)
#define DMP_CPASS_CAL_INIT			(114 * 16)
#define DMP_CPASS_EST_FIRST_BIAS	(113 * 16)
#define DMP_MAG_DISTURB_STATE		(113 * 16 + 2)
#define DMP_CPASS_VAR_COUNT			(112 * 16 + 6)
#define DMP_CPASS_COUNT_7			( 87 * 16 + 2)
#define DMP_CPASS_MAX_INNO			(124 * 16)
#define DMP_CPASS_BIAS_OFFSET		(113 * 16 + 4)
#define DMP_CPASS_CUR_BIAS_OFFSET	(114 * 16 + 4)
#define DMP_CPASS_PRE_SENSOR_DATA	( 87 * 16 + 4)

// Compass Cal params to be adjusted according to sampling rate
#define DMP_CPASS_TIME_BUFFER		(112 * 16 + 14)
#define DMP_CPASS_RADIUS_3D_THRESH_ANOMALY	(112 * 16 + 8)

#define DMP_CPASS_STATUS_CHK		(25 * 16 + 12)

// 9-axis
#define DMP_MAGN_THR_9X				(80 * 16)
#define DMP_MAGN_LPF_THR_9X			(80 * 16 +  8)
#define DMP_QFB_THR_9X				(80 * 16 + 12)

// DMP running counter
#define DMP_DMPRATE_CNTR			(18 * 16 + 4)

// pedometer
#define DMP_PEDSTD_BP_B				(49 * 16 + 12)
#define DMP_PEDSTD_BP_A4			(52 * 16)
#define DMP_PEDSTD_BP_A3			(52 * 16 +  4)
#define DMP_PEDSTD_BP_A2			(52 * 16 +  8)
#define DMP_PEDSTD_BP_A1			(52 * 16 + 12)
#define DMP_PEDSTD_SB				(50 * 16 +  8)
#define DMP_PEDSTD_SB_TIME			(50 * 16 + 12)
#define DMP_PEDSTD_PEAKTHRSH		(57 * 16 +  8)
#define DMP_PEDSTD_TIML				(50 * 16 + 10)
#define DMP_PEDSTD_TIMH				(50 * 16 + 14)
#define DMP_PEDSTD_PEAK				(57 * 16 +  4)
#define DMP_PEDSTD_STEPCTR			(54 * 16)
#define DMP_PEDSTD_STEPCTR2			(58 * 16 +  8)
#define DMP_PEDSTD_TIMECTR			(60 * 16 +  4)
#define DMP_PEDSTD_DECI				(58 * 16)
#define DMP_PEDSTD_SB2				(60 * 16 + 14)
#define DMP_STPDET_TIMESTAMP		(18 * 16 +  8)
#define DMP_PEDSTEP_IND				(19 * 16 +  4)
#define DMP_PED_Y_RATIO				(17 * 16 +  0)

// SMD
#define DMP_SMD_VAR_TH              (141 * 16 + 12)
#define DMP_SMD_VAR_TH_DRIVE        (143 * 16 + 12)
#define DMP_SMD_DRIVE_TIMER_TH      (143 * 16 +  8)
#define DMP_SMD_TILT_ANGLE_TH       (179 * 16 + 12)
#define DMP_BAC_SMD_ST_TH           (179 * 16 +  8)
#define DMP_BAC_ST_ALPHA4           (180 * 16 + 12)
#define DMP_BAC_ST_ALPHA4A          (176 * 16 + 12)

// Wake on Motion
#define DMP_WOM_ENABLE              (64 * 16 + 14)
#define DMP_WOM_STATUS              (64 * 16 + 6)
#define DMP_WOM_THRESHOLD           (64 * 16)
#define DMP_WOM_CNTR_TH             (64 * 16 + 12)

// Activity Recognition
#define DMP_BAC_RATE                (48  * 16 + 10)
#define DMP_BAC_STATE               (179 * 16 +  0)
#define DMP_BAC_STATE_PREV          (179 * 16 +  4)
#define DMP_BAC_ACT_ON              (182 * 16 +  0)
#define DMP_BAC_ACT_OFF             (183 * 16 +  0)
#define DMP_BAC_STILL_S_F           (177 * 16 +  0)
#define DMP_BAC_RUN_S_F             (177 * 16 +  4)
#define DMP_BAC_DRIVE_S_F           (178 * 16 +  0)
#define DMP_BAC_WALK_S_F            (178 * 16 +  4)
#define DMP_BAC_SMD_S_F             (178 * 16 +  8)
#define DMP_BAC_BIKE_S_F            (178 * 16 + 12)
#define DMP_BAC_E1_SHORT            (146 * 16 +  0)
#define DMP_BAC_E2_SHORT            (146 * 16 +  4)
#define DMP_BAC_E3_SHORT            (146 * 16 +  8)
#define DMP_BAC_VAR_RUN             (148 * 16 + 12)
#define DMP_BAC_TILT_INIT           (181 * 16 +  0)
#define DMP_BAC_MAG_ON              (225 * 16 +  0)
#define DMP_BAC_PS_ON               (74  * 16 +  0)
#define DMP_BAC_BIKE_PREFERENCE     (173 * 16 +  8)
#define DMP_BAC_MAG_I2C_ADDR        (229 * 16 +  8)
#define DMP_BAC_PS_I2C_ADDR         (75  * 16 +  4)
#define DMP_BAC_DRIVE_CONFIDENCE    (144 * 16 +  0)
#define DMP_BAC_WALK_CONFIDENCE     (144 * 16 +  4)
#define DMP_BAC_SMD_CONFIDENCE      (144 * 16 +  8)
#define DMP_BAC_BIKE_CONFIDENCE     (144 * 16 + 12)
#define DMP_BAC_STILL_CONFIDENCE    (145 * 16 +  0)
#define DMP_BAC_RUN_CONFIDENCE      (145 * 16 +  4)
#define DMP_BAC_MODE_CNTR           (150 * 16)
#define DMP_BAC_STATE_T_PREV        (185 * 16 +  4)
#define DMP_BAC_ACT_T_ON            (184 * 16 +  0)
#define DMP_BAC_ACT_T_OFF           (184 * 16 +  4)
#define DMP_BAC_STATE_WRDBS_PREV    (185 * 16 +  8)
#define DMP_BAC_ACT_WRDBS_ON        (184 * 16 +  8)
#define DMP_BAC_ACT_WRDBS_OFF       (184 * 16 + 12)
#define DMP_BAC_ACT_ON_OFF          (190 * 16 +  2)
#define DMP_PREV_BAC_ACT_ON_OFF     (188 * 16 +  2)
#define DMP_BAC_CNTR                (48  * 16 +  2)

// Flip/Pick-up
#define DMP_FP_VAR_ALPHA            (245 * 16 +  8)
#define DMP_FP_STILL_TH             (246 * 16 +  4)
#define DMP_FP_MID_STILL_TH         (244 * 16 +  8)
#define DMP_FP_NOT_STILL_TH         (246 * 16 +  8)
#define DMP_FP_VIB_REJ_TH           (241 * 16 +  8)
#define DMP_FP_MAX_PICKUP_T_TH      (244 * 16 + 12)
#define DMP_FP_PICKUP_TIMEOUT_TH    (248 * 16 +  8)
#define DMP_FP_STILL_CONST_TH       (246 * 16 + 12)
#define DMP_FP_MOTION_CONST_TH      (240 * 16 +  8)
#define DMP_FP_VIB_COUNT_TH         (242 * 16 +  8)
#define DMP_FP_STEADY_TILT_TH       (247 * 16 +  8)
#define DMP_FP_STEADY_TILT_UP_TH    (242 * 16 + 12)
#define DMP_FP_Z_FLAT_TH_MINUS      (243 * 16 +  8)
#define DMP_FP_Z_FLAT_TH_PLUS       (243 * 16 + 12)
#define DMP_FP_DEV_IN_POCKET_TH     (76  * 16 + 12)
#define DMP_FP_PICKUP_CNTR          (247 * 16 +  4)
#define DMP_FP_RATE                 (240 * 16 + 12)

// Gyro FSR
#define DMP_GYRO_FULLSCALE          (72 * 16 + 12)

// Accel FSR
#define DMP_ACC_SCALE               (30 * 16 + 0)
#define DMP_ACC_SCALE2              (79 * 16 + 4)

// EIS authentication
#define DMP_EIS_AUTH_INPUT			(160 * 16 +   4)
#define DMP_EIS_AUTH_OUTPUT			(160 * 16 +   0)

// B2S
#define DMP_B2S_RATE                (48  * 16 +   8)
// mounting matrix
#define DMP_B2S_MTX_00              (208 * 16)
#define DMP_B2S_MTX_01              (208 * 16 + 4)
#define DMP_B2S_MTX_02              (208 * 16 + 8)
#define DMP_B2S_MTX_10              (208 * 16 + 12)
#define DMP_B2S_MTX_11              (209 * 16)
#define DMP_B2S_MTX_12              (209 * 16 + 4)
#define DMP_B2S_MTX_20              (209 * 16 + 8)
#define DMP_B2S_MTX_21              (209 * 16 + 12)
#define DMP_B2S_MTX_22              (210 * 16)

#define DMP_START_ADDRESS   ((unsigned short)0x1000)
#define DMP_MEM_BANK_SIZE   256
#define DMP_LOAD_START      0x90

#define DMP_CODE_SIZE 14301

// BAC states
#define DMP_BAC_DRIVE   0x01
#define DMP_BAC_WALK    0x02
#define DMP_BAC_RUN     0x04
#define DMP_BAC_BIKE    0x08
#define DMP_BAC_TILT    0x10
#define DMP_BAC_STILL   0x20
