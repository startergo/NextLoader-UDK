/************************************************************************
 *
 * Copyright (c) 2013-2015 Intel Corporation.
 *
* This program and the accompanying materials
* are licensed and made available under the terms and conditions of the BSD License
* which accompanies this distribution.  The full text of the license may be found at
* http://opensource.org/licenses/bsd-license.php
*
* THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
* WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 *
 ************************************************************************/
#ifndef _MEMINIT_H_
#define _MEMINIT_H_

// function prototypes
void MemInit(MRCParams_t *mrc_params);

typedef void (*MemInitFn_t)(MRCParams_t *mrc_params);

typedef struct MemInit_s {
  uint16_t    post_code;
  uint16_t    boot_path;
  MemInitFn_t init_fn;
} MemInit_t;

#endif // _MEMINIT_H_
