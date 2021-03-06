//
//  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

#include <AsmMacroIoLib.h>

ASM_FUNC(_ModuleEntryPoint)
  // Do early platform specific actions
  bl    ASM_PFX(ArmPlatformPeiBootAction)

  // Identify CPU ID
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   r5, r0

  // Is it the Primary Core ?
  bl    ASM_PFX(ArmPlatformIsPrimaryCore)

  // Get the top of the primary stacks (and the base of the secondary stacks)
  MOV32 (r1, FixedPcdGet64(PcdCPUCoresStackBase) + FixedPcdGet32(PcdCPUCorePrimaryStackSize))

  // r0 is equal to 1 if I am the primary core
  cmp   r0, #1
  beq   _SetupPrimaryCoreStack

_SetupSecondaryCoreStack:
  // r1 contains the base of the secondary stacks

  // Get the Core Position
  mov   r6, r1      // Save base of the secondary stacks
  mov   r0, r5
  bl    ASM_PFX(ArmPlatformGetCorePosition)
  // The stack starts at the top of the stack region. Add '1' to the Core Position to get the top of the stack
  add   r0, r0, #1

  // StackOffset = CorePos * StackSize
  MOV32 (r2, FixedPcdGet32(PcdCPUCoreSecondaryStackSize))
  mul   r0, r0, r2
  // SP = StackBase + StackOffset
  add   sp, r6, r0

_PrepareArguments:
  // The PEI Core Entry Point has been computed by GenFV and stored in the second entry of the Reset Vector
  MOV32 (r2, FixedPcdGet32(PcdFvBaseAddress))
  ldr   r1, [r2, #4]

  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r3, =ASM_PFX(CEntryPoint)

  // Jump to PrePeiCore C code
  //    r0 = mp_id
  //    r1 = pei_core_address
  mov   r0, r5
  blx   r3

_SetupPrimaryCoreStack:
  mov   sp, r1
  MOV32 (r8, FixedPcdGet64 (PcdCPUCoresStackBase))
  MOV32 (r9, FixedPcdGet32 (PcdInitValueInTempStack))
  mov   r10, r9
  mov   r11, r9
  mov   r12, r9
0:stm   r8!, {r9-r12}
  cmp   r8, r1
  blt   0b
  b     _PrepareArguments

_NeverReturn:
  b _NeverReturn
