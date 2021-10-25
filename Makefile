#
# Copyright (C) 2020 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

include $(APPDIR)/Make.defs

#CSRCS += src/btmanager/btm_a2dp_source.c 
#CSRCS += src/btmanager/btm_avrcp_target.c 
#CSRCS += src/btmanager/btm_gap.c 

CSRCS += btservice/btservice/bts_service.c 
CSRCS += btservice/gap/bts_gap.c
CSRCS += btservice/a2dp_source/bts_a2dp_source.c
CSRCS += btservice/avrcp_target/bts_avrcp_target.c

CSRCS +=btservice/state_machine/state_machine.c
CSRCS +=btservice/hfp_client/bts_hf_client.c
CSRCS +=btservice/hfp_client/bts_hf_client_state_machine.c
CSRCS +=btservice/spp/bts_spp.c
CSRCS +=utils/list.c

MAINSRC   = btmanager/btm_manager.c 

PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = CONFIG_DEFAULT_TASK_STACKSIZE
PROGNAME  = btmanager 

MODULE    = $(CONFIG_BLUETOOTH)
include $(APPDIR)/Application.mk

