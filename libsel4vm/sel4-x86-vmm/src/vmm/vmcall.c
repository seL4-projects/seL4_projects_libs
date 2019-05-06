/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <sel4vm/guest_vm.h>
#include "sel4vm/vmm.h"
#include "sel4vm/debug.h"
#include "sel4vm/vmcall.h"

static vmcall_handler_t *get_handle(vm_t *vm, int token);

static vmcall_handler_t *get_handle(vm_t *vm, int token) {
    int i;
    for(i = 0; i < vm->arch.vmcall_num_handlers; i++) {
        if(vm->arch.vmcall_handlers[i].token == token) {
            return &vm->arch.vmcall_handlers[i];
        }
    }

    return NULL;
}

int reg_new_handler(vm_t *vm, vmcall_handler func, int token) {
    unsigned int *hnum = &(vm->arch.vmcall_num_handlers);
    if(get_handle(vm, token) != NULL) {
        return -1;
    }

    vm->arch.vmcall_handlers = realloc(vm->arch.vmcall_handlers, sizeof(vmcall_handler_t) * (*hnum + 1));
    if(vm->arch.vmcall_handlers == NULL) {
        return -1;
    }

    vm->arch.vmcall_handlers[*hnum].func = func;
    vm->arch.vmcall_handlers[*hnum].token = token;
    vm->arch.vmcall_num_handlers++;

    DPRINTF(4, "Reg. handler %u for vmm, total = %u\n", *hnum - 1, *hnum);
    return 0;
}

int vmm_vmcall_handler(vm_vcpu_t *vcpu) {
    int res;
    vmcall_handler_t *h;
    int token = vmm_read_user_context(&vcpu->vcpu_arch.guest_state, USER_CONTEXT_EAX);
    h = get_handle(vcpu->vm, token);
    if(h == NULL) {
        DPRINTF(2, "Failed to find handler for token:%x\n", token);
        vmm_guest_exit_next_instruction(&vcpu->vcpu_arch.guest_state, vcpu->vcpu.cptr);
        return 0;
    }

    res = h->func(vcpu);
    if(res == 0) {
        vmm_guest_exit_next_instruction(&vcpu->vcpu_arch.guest_state, vcpu->vcpu.cptr);
    }

    return res;
}