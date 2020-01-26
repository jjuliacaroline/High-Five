 /******************************************************************************
 *  Filename:       aon_batmon.h
 *  Revised:        2015-07-16 12:12:04 +0200 (Thu, 16 Jul 2015)
 *  Revision:       44151
 *
 *  Description:    Defines and prototypes for the AON Battery and Temperature
 *                  Monitor
 *
 *  Copyright (c) 2015, Texas Instruments Incorporated
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1) Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2) Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *  3) Neither the name of the ORGANIZATION nor the names of its contributors may
 *     be used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

 //*****************************************************************************
 //
 //
 //*****************************************************************************

 #ifndef __AON_BATMON_H__
 #define __AON_BATMON_H__

 //*****************************************************************************
 //
 // If building with a C++ compiler, make all of the definitions in this header
 // have a C binding.
 //
 //*****************************************************************************
 #ifdef __cplusplus
 extern "C"
 {
 #endif

 #include <stdbool.h>
 #include <stdint.h>
 #include <inc/hw_types.h>
 #include <inc/hw_memmap.h>
 #include <inc/hw_aon_batmon.h>
 #include <driverlib/debug.h>


 //*****************************************************************************
 //
 // API Functions and prototypes
 //
 //*****************************************************************************

 //*****************************************************************************
 //
 //
 //*****************************************************************************
 __STATIC_INLINE void
 AONBatMonEnable(void)
 {
     //
     // Enable the measurements.
     //
     HWREG(AON_BATMON_BASE + AON_BATMON_O_CTL) =
         AON_BATMON_CTL_CALC_EN |
         AON_BATMON_CTL_MEAS_EN;
 }

 //*****************************************************************************
 //
 //
 //*****************************************************************************
 __STATIC_INLINE void
 AONBatMonDisable(void)
 {
     //
     // Disable the measurements.
     //
     HWREG(AON_BATMON_BASE + AON_BATMON_O_CTL) = 0;
 }


 //*****************************************************************************
 //
 //
 //*****************************************************************************
 int32_t
 AONBatMonTemperatureGetDegC( void );

 //*****************************************************************************
 //
 //
 //*****************************************************************************
 __STATIC_INLINE uint32_t
 AONBatMonBatteryVoltageGet(void)
 {
     uint32_t ui32CurrentBattery;

     ui32CurrentBattery = HWREG(AON_BATMON_BASE + AON_BATMON_O_BAT);

     //
     // Return the current battery voltage measurement.
     //
     return (ui32CurrentBattery >> AON_BATMON_BAT_FRAC_S);
 }

 //*****************************************************************************
 //
 //
 //*****************************************************************************
 __STATIC_INLINE bool
 AONBatMonNewBatteryMeasureReady(void)
 {
     bool bStatus;

     //
     // Check the status bit.
     //
     bStatus = HWREG(AON_BATMON_BASE + AON_BATMON_O_BATUPD) &
               AON_BATMON_BATUPD_STAT ? true : false;

     //
     // Clear status bit if set.
     //
     if(bStatus)
     {
         HWREG(AON_BATMON_BASE + AON_BATMON_O_BATUPD) = 1;
     }

     //
     // Return status.
     //
     return (bStatus);
 }

 //*****************************************************************************
 //
 //
 //*****************************************************************************
 __STATIC_INLINE bool
 AONBatMonNewTempMeasureReady(void)
 {
     bool bStatus;

     //
     // Check the status bit.
     //
     bStatus = HWREG(AON_BATMON_BASE + AON_BATMON_O_TEMPUPD) &
               AON_BATMON_TEMPUPD_STAT ? true : false;

     //
     // Clear status bit if set.
     //
     if(bStatus)
     {
         HWREG(AON_BATMON_BASE + AON_BATMON_O_TEMPUPD) = 1;
     }

     //
     // Return status.
     //
     return (bStatus);
 }

 //*****************************************************************************
 //
 // Mark the end of the C bindings section for C++ compilers.
 //
 //*****************************************************************************
 #ifdef __cplusplus
 }
 #endif

 #endif //  __AON_BATMON_H__

 //*****************************************************************************
 //
 //
 //*****************************************************************************
