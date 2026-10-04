/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


/**
 * File: trts_veh.cpp
 * Description: 
 *     This file implements the support of custom exception handling. 
 */

#include "sgx_trts_exception.h"
#include <stdlib.h>
#include <string.h>
#include "sgx_trts.h"
#include "xsave.h"
#include "arch.h"
#include "sgx_spinlock.h"
#include "thread_data.h"
#include "global_data.h"
#include "trts_internal.h"
#include "trts_mitigation.h"
#include "trts_inst.h"
#include "util.h"
#include "trts_util.h"
#include "trts_shared_constants.h"
#include "se_cdefs.h"
#include "emm_private.h"
#include "sgx_mm_rt_abstraction.h"
#include "sgx_trts_aex.h"
#include "ctd.h"
#include "elf_util.h"

#include "se_memcpy.h"
typedef struct _handler_node_t
{
    uintptr_t callback;
    struct _handler_node_t   *next;
} handler_node_t;

static handler_node_t *g_first_node = NULL;
static sgx_spinlock_t g_handler_lock = SGX_SPINLOCK_INITIALIZER;

static uintptr_t g_veh_cookie = 0;
sgx_mm_pfhandler_t g_mm_pfhandler = NULL;
#define ENC_VEH_POINTER(x)  (uintptr_t)(x) ^ g_veh_cookie
#define DEC_VEH_POINTER(x)  (sgx_exception_handler_t)((x) ^ g_veh_cookie)
extern int g_aexnotify_supported;
extern "C" sgx_status_t sgx_apply_mitigations(const sgx_exception_info_t *);
extern "C" uintptr_t cselect_mitigation_rip(const sgx_exception_info_t *);
extern "C" uintptr_t cselect_mitigation_regs(const sgx_exception_info_t *,
		                             uintptr_t saved_rip,
					     uintptr_t c3_byte_address);

extern uint16_t aex_notify_c3_cache[2048];
extern uint8_t *__ct_mitigation_ret;


// sgx_register_exception_handler()
//      register a custom exception handler
// Parameter
//      is_first_handler - the order in which the handler should be called.
// if the parameter is nonzero, the handler is the first handler to be called.
// if the parameter is zero, the handler is the last handler to be called.
//      exception_handler - a pointer to the handler to be called.
// Return Value
//      handler - success
void *sgx_register_exception_handler(int is_first_handler, sgx_exception_handler_t exception_handler)
{
    // initialize g_veh_cookie for the first time sgx_register_exception_handler is called.
    if(unlikely(g_veh_cookie == 0))
    {
        uintptr_t rand = 0;
        do
        {
            if(SGX_SUCCESS != sgx_read_rand((unsigned char *)&rand, sizeof(rand)))
            {
                return NULL;
            }
        } while(rand == 0);

        sgx_spin_lock(&g_handler_lock);
        if(g_veh_cookie == 0)
        {
            g_veh_cookie = rand;
        }
        sgx_spin_unlock(&g_handler_lock);
    }
    if(!sgx_is_within_enclave((const void*)exception_handler, 0))
    {
        return NULL;
    }
    handler_node_t *node = (handler_node_t *)malloc(sizeof(handler_node_t));
    if(!node)
    {
        return NULL;
    }
    node->callback = ENC_VEH_POINTER(exception_handler);

    // write lock
    sgx_spin_lock(&g_handler_lock);

    if((g_first_node == NULL) || is_first_handler)
    {
        node->next = g_first_node;
        g_first_node = node;
    }
    else
    {
        handler_node_t *tmp = g_first_node;
        while(tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        node->next = NULL;
        tmp->next = node;
    }
    // write unlock
    sgx_spin_unlock(&g_handler_lock);

    return node;
}
// sgx_unregister_exception_handler()
//      unregister a custom exception handler.
// Parameter
//      handler - a handler to the custom exception handler previously 
// registered using the sgx_register_exception_handler function.
// Return Value
//      none zero - success
//              0 - fail
int sgx_unregister_exception_handler(void *handler)
{
    if(!handler)
    {
        return 0;
    }

    int status = 0;

    // write lock
    sgx_spin_lock(&g_handler_lock);

    if(g_first_node)
    {
        handler_node_t *node = g_first_node;
        if(node == handler)
        {
            g_first_node = node->next;
            status = 1;
        }
        else
        {
            while(node->next != NULL)
            {
                if(node->next == handler)
                {
                    node->next = node->next->next;
                    status = 1;
                    break;
                }
                node = node->next;
            }
        }
    }
    // write unlock
    sgx_spin_unlock(&g_handler_lock);

    if(status) free(handler);
    return status;
}

// continue_execution(sgx_exception_info_t *info):
// try to restore the thread context saved in info to current execution context.
extern "C" __attribute__((regparm(1))) void continue_execution(sgx_exception_info_t *info);
extern "C" void restore_xregs(uint8_t *buf);

#ifndef SE_SIM
extern "C" __attribute__((regparm(1))) void second_phase(sgx_exception_info_t *info, 
    void *new_sp, void *second_phase_handler_addr);

extern "C" void constant_time_apply_sgxstep_mitigation_and_continue_execution(sgx_exception_info_t *info,
        uintptr_t ssa_aexnotify_addr, uintptr_t stack_tickle_pages, uintptr_t code_tickle_page, uintptr_t data_tickle_address, uintptr_t c3_byte_address);

// constant time select based on given condition
static inline uint64_t cselect64(uint64_t pred, const uint64_t expected, uint64_t old_val, uint64_t new_val)
{
    __asm__("cmp %3, %1\n\t"
            "cmove %2, %0"
            : "+r"(new_val)
            : "rm"(pred), "rm"(old_val), "ri"(expected));
    return new_val;
}

static uintptr_t c3_cache_lookup(uint64_t address)
{
    const uintptr_t page = address & ~static_cast<uintptr_t>(0xFFF);
    uintptr_t c3_byte_address =
        page + *(aex_notify_c3_cache + ((page >> 12) & 0x07FF));
    if (*(uint8_t *)c3_byte_address != 0xc3)
    {
        uint8_t *i = (uint8_t *)page;
        uint8_t *e = i + 4096;
        for (; i != e && *i != 0xc3; ++i) {}
        if (i == e)
        {
            c3_byte_address = (uintptr_t)&__ct_mitigation_ret;
        }
        else
        {
            c3_byte_address = (uintptr_t)i;
            *(aex_notify_c3_cache + ((page >> 12) & 0x07FF)) =
                (uint16_t)(c3_byte_address & 0xFFF);
        }
    }
    return c3_byte_address;
}

#define TLBLUR_PAM_SIZE_BYTES       0x1000000ULL
#define TLBLUR_PAM_ENTRY_COUNT      (TLBLUR_PAM_SIZE_BYTES / sizeof(uint64_t))
#define TLBLUR_PAM_PAGE_COUNT       (TLBLUR_PAM_SIZE_BYTES / 0x1000ULL)
#define TLBLUR_MAX_PWS_SIZE         4096ULL
#define TLBLUR_MANDATORY_PWS_SIZE   (TLBLUR_PAM_PAGE_COUNT + 2)
#define TLBLUR_PWS_CAPACITY         (TLBLUR_MAX_PWS_SIZE + TLBLUR_MANDATORY_PWS_SIZE)

extern "C" uint64_t __tlblur_pam[] __attribute__((weak));
extern "C" uint64_t __tlblur_counter __attribute__((weak));
extern "C" void tlblur_pam_update(void) __attribute__((weak));

extern "C" {
bool g_tlblur_enabled = false;
uint64_t g_tlblur_pws_size = 0;
uint64_t g_tlblur_pws_r_size = 0;
uint64_t g_tlblur_pws_x_size = 0;
uint64_t g_tlblur_pws_w_size = 0;
uint64_t g_tlblur_prefetch_r[TLBLUR_PWS_CAPACITY] = {0};
uint64_t g_tlblur_prefetch_w[TLBLUR_PWS_CAPACITY] = {0};
uint64_t g_tlblur_prefetch_x[TLBLUR_PWS_CAPACITY] = {0};
}
static uint64_t g_tlblur_prefetch_buffer[TLBLUR_PWS_CAPACITY] = {0};
/* __tlblur_pam_size is bytes in runtime; this length is PAM uint64_t entries. */
static uint64_t g_tlblur_pam_size = TLBLUR_PAM_ENTRY_COUNT;
static uint64_t g_tlblur_prefetch_count = 0;

static inline uint64_t tlblur_nonzero(uint64_t value)
{
    return (value | (0 - value)) >> 63;
}

static inline uint64_t tlblur_less(uint64_t lhs, uint64_t rhs)
{
    return lhs < rhs;
}

static bool tlblur_runtime_available(void)
{
    /* TLBLUR_PAM_SIZE_BYTES must agree with __tlblur_pam_size in the TLBlur
     * runtime (runtime/tlblur.S), which sizes the PAM and masks the index.
     * That is an assembly absolute symbol, so its value cannot be read from
     * C: taking its address under PIE yields __ImageBase + 0x1000000. The
     * agreement is therefore a build-time convention, not a runtime check. */
    return reinterpret_cast<uintptr_t>(__tlblur_pam) != 0 &&
           reinterpret_cast<uintptr_t>(&__tlblur_counter) != 0 &&
           reinterpret_cast<uintptr_t>(tlblur_pam_update) != 0;
}

/*
 * Page permissions come from the enclave image's own program headers, not from
 * linker-script boundary symbols: symbols such as _srx/_ero are neither page
 * aligned nor guaranteed to cover every allocated section, so a page-base
 * address derived from a PAM index can fall outside them and be misclassified.
 * The loader derives EPCM permissions from the same headers
 * (elf_parser.c:init_segment_emas), so the headers are authoritative.
 */
#define TLBLUR_MAX_SEGMENTS 16

struct tlblur_range
{
    uintptr_t start;
    uintptr_t end;
};

static tlblur_range g_tlblur_load[TLBLUR_MAX_SEGMENTS];
static uint32_t g_tlblur_load_flags[TLBLUR_MAX_SEGMENTS];
static size_t g_tlblur_load_count = 0;
static tlblur_range g_tlblur_relro[TLBLUR_MAX_SEGMENTS];
static size_t g_tlblur_relro_count = 0;

/* Page rounding identical to elf_parser.c change_protection/init_segment_emas. */
static void tlblur_segment_pages(uintptr_t base, const ElfW(Phdr) *phdr,
                                 tlblur_range *out)
{
    out->start = base + (phdr->p_vaddr & ~static_cast<uintptr_t>(SE_PAGE_SIZE - 1));
    out->end = base + ((phdr->p_vaddr + phdr->p_memsz + SE_PAGE_SIZE - 1) &
                       ~static_cast<uintptr_t>(SE_PAGE_SIZE - 1));
}

static bool tlblur_load_segments(void)
{
    const uintptr_t base = reinterpret_cast<uintptr_t>(get_enclave_base());
    const ElfW(Ehdr) *ehdr = reinterpret_cast<const ElfW(Ehdr) *>(base);
    const ElfW(Phdr) *phdr = reinterpret_cast<const ElfW(Phdr) *>(
        base + static_cast<uintptr_t>(ehdr->e_phoff));

    g_tlblur_load_count = 0;
    g_tlblur_relro_count = 0;

    for (ElfW(Half) i = 0; i < ehdr->e_phnum; ++i, ++phdr)
    {
        if (phdr->p_type == PT_LOAD)
        {
            if (g_tlblur_load_count >= TLBLUR_MAX_SEGMENTS)
                return false;
            tlblur_segment_pages(base, phdr,
                                 &g_tlblur_load[g_tlblur_load_count]);
            g_tlblur_load_flags[g_tlblur_load_count] = phdr->p_flags;
            ++g_tlblur_load_count;
        }
        else if (phdr->p_type == PT_GNU_RELRO)
        {
            if (g_tlblur_relro_count >= TLBLUR_MAX_SEGMENTS)
                return false;
            tlblur_segment_pages(base, phdr,
                                 &g_tlblur_relro[g_tlblur_relro_count]);
            ++g_tlblur_relro_count;
        }
    }

    return g_tlblur_load_count != 0;
}

static bool tlblur_in_range(const tlblur_range *range, uintptr_t address)
{
    return address >= range->start && address < range->end;
}

/*
 * Effective page permissions.  Addresses outside every PT_LOAD (heap, stack,
 * TCS, reserved layout entries) are read-write.  PT_GNU_RELRO pages lose PF_W
 * because change_protection() strips it after relocation, so writing them
 * would fault exactly like writing an RX page.
 */
static uint32_t tlblur_page_flags(uintptr_t address)
{
    uint32_t flags = PF_R | PF_W;

    for (size_t i = 0; i < g_tlblur_load_count; ++i)
    {
        if (tlblur_in_range(&g_tlblur_load[i], address))
        {
            flags = g_tlblur_load_flags[i];
            break;
        }
    }

    for (size_t i = 0; i < g_tlblur_relro_count; ++i)
    {
        if (tlblur_in_range(&g_tlblur_relro[i], address))
        {
            flags &= ~static_cast<uint32_t>(PF_W);
            break;
        }
    }

    return flags;
}

static bool tlblur_is_exec(uintptr_t address)
{
    return (tlblur_page_flags(address) & PF_X) != 0;
}

static bool tlblur_is_writable(uintptr_t address)
{
    return (tlblur_page_flags(address) & PF_W) != 0;
}

static bool tlblur_append(uint64_t *array, uint64_t *size, uint64_t value)
{
    if (*size >= TLBLUR_PWS_CAPACITY)
        return false;
    array[*size] = value;
    ++*size;
    return true;
}

static void tlblur_select_max_lt(uint64_t input[], size_t input_len,
                                  uint64_t limit, uint64_t *max,
                                  uint64_t *max_idx)
{
    *max = 0;
    *max_idx = 0;
    for (size_t i = 0; i < input_len; ++i)
    {
        const uint64_t current = input[i];
        const uint64_t current_lt_limit = current < limit;
        const uint64_t current_ge_max = current >= *max;
        const uint64_t replace = tlblur_nonzero(current) &
                                 current_lt_limit & current_ge_max;
        *max = cselect64(replace, 1, current, *max);
        *max_idx = cselect64(replace, 1, i, *max_idx);
    }
}

/*
 * Select the top-N PAM counters.  PAM values determine recency here, while
 * cselect64 keeps the scan schedule independent of their contents.
 */
static void tlblur_select_pws(uint64_t pam[], size_t pam_len, uint64_t pws[],
                              size_t pws_len)
{
    uint64_t limit = static_cast<uint64_t>(-1);
    for (size_t j = 0; j < pws_len; ++j)
    {
        uint64_t max = 0;
        uint64_t max_idx = 0;
        tlblur_select_max_lt(pam, pam_len, limit, &max, &max_idx);
        pws[j] = max_idx;
        limit = max;
    }
}

/*
 * Sort page indices in ascending order with a fixed compare/exchange schedule
 * after the recency selection, so the final access order does not expose PAM
 * freshness.
 */
static void tlblur_sort(uint64_t input[], uint64_t output[], size_t input_len,
                        size_t output_len)
{
    if (input_len == 0 || output_len == 0)
        return;

    for (size_t i = 0; i < output_len; ++i)
        output[i] = input[i];

    for (size_t i = 0; i < output_len; ++i)
    {
        for (size_t j = i + 1; j < output_len; ++j)
        {
            const uint64_t left = output[i];
            const uint64_t right = output[j];
            const uint64_t swap = tlblur_less(right, left);
            // cselect64(pred, 1, a, b) yields `a` when pred == 1.
            output[i] = cselect64(swap, 1, right, left);
            output[j] = cselect64(swap, 1, left, right);
        }
    }
}

static bool tlblur_prepare_prefetch(uintptr_t stack_page, uintptr_t c3_byte_address)
{
    const uintptr_t enclave_base = reinterpret_cast<uintptr_t>(get_enclave_base());
    const size_t candidate_count = static_cast<size_t>(g_tlblur_pws_size);
    const size_t fixed_count = static_cast<size_t>(g_tlblur_prefetch_count);
    size_t r_size = 0;
    size_t w_size = 0;
    size_t x_size = 0;

    for (size_t i = 0; i < fixed_count; ++i)
    {
        g_tlblur_prefetch_r[i] = c3_byte_address;
        g_tlblur_prefetch_w[i] = stack_page;
        g_tlblur_prefetch_x[i] = c3_byte_address;
    }

    tlblur_select_pws(__tlblur_pam, static_cast<size_t>(g_tlblur_pam_size),
                      g_tlblur_prefetch_buffer, candidate_count);
    tlblur_sort(g_tlblur_prefetch_buffer, g_tlblur_prefetch_buffer,
                candidate_count, candidate_count);

    for (size_t i = 0; i < candidate_count; ++i)
    {
        const uint64_t selected = g_tlblur_prefetch_buffer[i];
        const uintptr_t address = selected == 0 ?
            c3_byte_address : enclave_base + (selected << 12);

        if (!tlblur_append(g_tlblur_prefetch_r, &r_size, address))
            return false;

        if (tlblur_is_exec(address))
        {
            if (!tlblur_append(g_tlblur_prefetch_x, &x_size,
                               c3_cache_lookup(address)))
                return false;
        }
        else if (tlblur_is_writable(address) &&
                 !tlblur_append(g_tlblur_prefetch_w, &w_size, address))
        {
            return false;
        }
    }

    for (size_t offset = 0; offset < g_tlblur_pam_size * sizeof(uint64_t);
         offset += 0x1000)
    {
        const uintptr_t address = reinterpret_cast<uintptr_t>(__tlblur_pam) + offset;
        if (!tlblur_append(g_tlblur_prefetch_r, &r_size, address) ||
            !tlblur_append(g_tlblur_prefetch_w, &w_size, address))
            return false;
    }

    const uintptr_t counter_address = reinterpret_cast<uintptr_t>(&__tlblur_counter);
    if (!tlblur_append(g_tlblur_prefetch_r, &r_size, counter_address) ||
        !tlblur_append(g_tlblur_prefetch_w, &w_size, counter_address))
        return false;

    const uintptr_t update_address = c3_cache_lookup(
        reinterpret_cast<uintptr_t>(tlblur_pam_update));
    if (!tlblur_append(g_tlblur_prefetch_r, &r_size, update_address) ||
        !tlblur_append(g_tlblur_prefetch_x, &x_size, update_address))
        return false;

    if (r_size > fixed_count || w_size > fixed_count || x_size > fixed_count)
        return false;

    g_tlblur_pws_r_size = fixed_count;
    g_tlblur_pws_w_size = fixed_count;
    g_tlblur_pws_x_size = fixed_count;
    return true;
}

extern "C" sgx_status_t tlblur_enable(uint64_t vtlb_size)
{
    if (vtlb_size > TLBLUR_MAX_PWS_SIZE || !tlblur_runtime_available())
        return SGX_ERROR_UNEXPECTED;

    /* Cache the image segment permissions once; the AEX path must not walk
     * the program headers or take any lock. */
    if (!tlblur_load_segments())
        return SGX_ERROR_UNEXPECTED;

    const uint64_t enclave_pages = get_enclave_size() >> 12;
    g_tlblur_pam_size = enclave_pages < TLBLUR_PAM_ENTRY_COUNT ?
        enclave_pages : TLBLUR_PAM_ENTRY_COUNT;
    g_tlblur_pws_size = vtlb_size;
    if (g_tlblur_pws_size > g_tlblur_pam_size)
        g_tlblur_pws_size = g_tlblur_pam_size;
    const uint64_t pam_page_count =
        (g_tlblur_pam_size * sizeof(uint64_t) + 0xFFF) >> 12;
    if (pam_page_count + 2 > TLBLUR_PWS_CAPACITY ||
        g_tlblur_pws_size > TLBLUR_PWS_CAPACITY - pam_page_count - 2)
        return SGX_ERROR_INVALID_PARAMETER;

    g_tlblur_prefetch_count = g_tlblur_pws_size + pam_page_count + 2;
    g_tlblur_pws_r_size = 0;
    g_tlblur_pws_w_size = 0;
    g_tlblur_pws_x_size = 0;
    g_tlblur_enabled = true;
    return SGX_SUCCESS;
}

extern "C" sgx_status_t tlblur_disable(void)
{
    g_tlblur_enabled = false;
    g_tlblur_pws_size = 0;
    g_tlblur_prefetch_count = 0;
    g_tlblur_pws_r_size = 0;
    g_tlblur_pws_w_size = 0;
    g_tlblur_pws_x_size = 0;
    return SGX_SUCCESS;
}

// apply the constant time mitigation handler
static void apply_constant_time_sgxstep_mitigation_and_continue_execution(sgx_exception_info_t *info)
{
    thread_data_t *thread_data = get_thread_data();
    int ct_result;
    uint64_t data_address;
    uintptr_t saved_rip;
    uintptr_t code_tickle_page, c3_byte_address, stack_tickle_pages, data_tickle_address,
              stack_base_page = ((thread_data->stack_base_addr & ~0xFFF) == 0) ?
                  (thread_data->stack_base_addr) - 0x1000 :
                  (thread_data->stack_base_addr & ~0xFFF),
              stack_limit_page = thread_data->stack_limit_addr & ~0xFFF;
    int data_tickle_address_is_within_enclave;

    // NOTE: use cselect_mitigation_rip to ensure we only ever dereference
    // the interrupted application code page, even if previous interrupt
    // was in the atomic mitigation stub (i.e., zero-step)
    saved_rip = cselect_mitigation_rip(info);

    // Determine which stack pages can be tickled
    if (((uintptr_t)info & ~0xFFF) == stack_base_page) {
        if (stack_base_page == stack_limit_page) {
            // The stack is only a single page, so we tickle that page
            stack_tickle_pages = stack_base_page;
        } else {
            // The current stack page is the base page, but there are more
            // pages so we tickle the next one as well.
            stack_tickle_pages = (stack_base_page - 0x1000) | 1;
        }
    } else {
        // If the current stack page is not the base page, then it's generally
        // better to also tickle the previous page. For example, the mitigation
        // code and the interrupted code may have separate but adjacent stack
        // pages (in this case, the interrupted code's stack frame must be on
        // the page with a higher address).
        stack_tickle_pages = ((uintptr_t)info & ~0xFFF) | 1;
    }

    // Look up the code page in the c3 cache
    code_tickle_page = saved_rip & ~0xFFF;
    c3_byte_address = c3_cache_lookup(code_tickle_page);

    // NOTE: in case the previous interrupt was in the atomic mitigation
    // stub, first restore clobbered application registers in the info
    // struct before determining tickle addresses
    cselect_mitigation_regs(info, saved_rip, c3_byte_address);

    ct_result = ct_decode(&info->cpu_context, &data_address);

    data_tickle_address = stack_tickle_pages & ~0x1;
    data_tickle_address = cselect64(ct_result, 1, data_address, data_tickle_address);
    data_tickle_address = cselect64(ct_result, 2, data_address, data_tickle_address);
    data_tickle_address_is_within_enclave =
		sgx_is_within_enclave((void*) data_tickle_address, sizeof(uint8_t));

    /*
     * Ensure the tickle page dereferenced by the mitigation lies _inside_ the enclave.
     *
     * NOTE:
     *  - Unguarded user memory accesses can leak through MMIO stale data.
     *  - User memory accesses are detectable and single-steppable anyway.
     *  - Below non-cst time check can only ever be false when the next enclave
     *    instruction will dereference user memory (trivially known to attacker).
     */
    data_tickle_address = data_tickle_address_is_within_enclave ?
                          data_tickle_address : stack_tickle_pages & ~0x1;

    code_tickle_page = cselect64(ct_result, 2, code_tickle_page | 0x1, code_tickle_page);
    code_tickle_page = cselect64(data_tickle_address_is_within_enclave, 1, code_tickle_page, code_tickle_page & ~0x1);

    // Pop an entropy byte from the entropy cache
    if (--thread_data->aex_notify_entropy_remaining < 0) {
        if (0 == do_rdrand(&thread_data->aex_notify_entropy_cache))
        {
            thread_data->exception_flag = -1;
            abort();
        }
        thread_data->aex_notify_entropy_remaining = 31;
    }
    code_tickle_page |= (thread_data->aex_notify_entropy_cache & 1) << 4;
    thread_data->aex_notify_entropy_cache >>= 1;

    if (g_tlblur_enabled &&
        !tlblur_prepare_prefetch(stack_tickle_pages & ~static_cast<uintptr_t>(1),
                                 c3_byte_address))
    {
        // Every append is checked; disable the optional prefetcher rather than
        // entering the mitigation with a partially populated working set.
        g_tlblur_enabled = false;
        g_tlblur_pws_size = 0;
        g_tlblur_prefetch_count = 0;
        g_tlblur_pws_r_size = 0;
        g_tlblur_pws_w_size = 0;
        g_tlblur_pws_x_size = 0;
    }

    // There are three additional "implicit" parameters to this function:
    // 1. The low-order bit of `stack_tickle_pages` is 1 if a second stack
    //    page should be tickled (specifically, the stack page immediately
    //    below the page specified in the upper bits)
    // 2. Bit 0 of `code_tickle_page` is 1 if `data_tickle_address`
    //    is writable, and therefore should be tested for write permissions
    //    by the mitigation
    // 3. Bit 4 of `code_tickle_page` is 1 if the cycle delay
    //    should be added to the mitigation
    constant_time_apply_sgxstep_mitigation_and_continue_execution(
                    info, thread_data->first_ssa_gpr + offsetof(ssa_gpr_t, aex_notify),
                    stack_tickle_pages, code_tickle_page,
                    data_tickle_address, c3_byte_address);
}
#else

// Simulation mode has no AEX-Notify mitigation to hook, so TLBlur is a no-op.
// The symbols must still exist so SIM enclaves link against the same API.
sgx_status_t tlblur_enable(uint64_t vtlb_size)
{
    (void)vtlb_size;
    return SGX_SUCCESS;
}

sgx_status_t tlblur_disable(void)
{
    return SGX_SUCCESS;
}

#endif

//      the 2nd phrase exception handing, which traverse registered exception handlers.
//      if the exception can be handled, then continue execution
//      otherwise, throw abortion, go back to 1st phrase, and call the default handler.
extern "C" __attribute__((regparm(1))) void internal_handle_exception(sgx_exception_info_t *info)
{
    int status = EXCEPTION_CONTINUE_SEARCH;
    handler_node_t *node = NULL;
    thread_data_t *thread_data = get_thread_data();
    size_t size = 0;
    uintptr_t *nhead = NULL;
    uintptr_t *ntmp = NULL;
    uintptr_t xsp = 0;
    uint8_t *xsave_in_ssa = (uint8_t*)ROUND_TO_PAGE(thread_data->first_ssa_gpr) - ROUND_TO_PAGE(get_xsave_size() + sizeof(ssa_gpr_t));

    // AEX Notify allows this handler to handle interrupts
    if (info == NULL) {
        goto failed_end;
    }

    memcpy_s(info->xsave_area, info->xsave_size, xsave_in_ssa, info->xsave_size);

    if (info->exception_valid == 0) {
        goto exception_handling_end;
    }

    if (thread_data->exception_flag < 0)
        goto failed_end;
    thread_data->exception_flag++;

    if(info->exception_vector == SGX_EXCEPTION_VECTOR_PF &&
        (g_mm_pfhandler != NULL))
    {
        thread_data->exception_flag--;
        sgx_pfinfo* pfinfo = (sgx_pfinfo*)(&info->exinfo);
        if(SGX_MM_EXCEPTION_CONTINUE_EXECUTION == g_mm_pfhandler(pfinfo))
        {
            //instruction triggering the exception will be executed again.
           goto exception_handling_end;
        }
        //restore old flag, and fall thru
        thread_data->exception_flag++;
    }
    // read lock
    sgx_spin_lock(&g_handler_lock);

    node = g_first_node;
    while(node != NULL)
    {
        size += sizeof(uintptr_t);
        node = node->next;
    }

    // There's no exception handler registered
    if (size == 0)
    {
        sgx_spin_unlock(&g_handler_lock);

        //exception cannot be handled
        thread_data->exception_flag = -1;

        goto exception_handling_end;
    }
    // The customer handler may never return, use alloca instead of malloc
    if ((nhead = (uintptr_t *)alloca(size)) == NULL)
    {
        sgx_spin_unlock(&g_handler_lock);
        goto failed_end;
    }
    ntmp = nhead;
    node = g_first_node;
    while(node != NULL)
    {
        *ntmp = node->callback;
        ntmp++;
        node = node->next;
    }

    // read unlock
    sgx_spin_unlock(&g_handler_lock);

    // decrease the nested exception count before the customer
    // handler execution, becasue the handler may never return
    thread_data->exception_flag--;

    // call exception handler until EXCEPTION_CONTINUE_EXECUTION is returned
    ntmp = nhead;
    while(size > 0)
    {
        sgx_exception_handler_t handler = DEC_VEH_POINTER(*ntmp);
        status = handler(info);
        if(EXCEPTION_CONTINUE_EXECUTION == status)
        {
            break;
        }
        ntmp++;
        size -= sizeof(sgx_exception_handler_t);
    }

    // call default handler
    // ignore invalid return value, treat to EXCEPTION_CONTINUE_SEARCH
    // check SP to be written on SSA is pointing to the trusted stack
    xsp = info->cpu_context.REG(sp);
    if (!is_valid_sp(xsp))
    {
        goto failed_end;
    }

    if(EXCEPTION_CONTINUE_EXECUTION != status)
    {
        //exception cannot be handled
        thread_data->exception_flag = -1;
    }

exception_handling_end:
#ifndef SE_SIM
    //instruction triggering the exception will be executed again.
    if(info->do_aex_mitigation == 1)
    {
        if (info->exception_vector == SGX_EXCEPTION_VECTOR_PF &&
            thread_data->exception_flag == -1)
        {
            // The #PF wasn't handled by EDMM or a custom #PF handler, but
            // since do_aex_mitigation == 1 here (AEX-Notify enabled), the #PF
            // will still be "handled" by the AEX-Notify mitigation.
            thread_data->exception_flag = 0;
        }

        // apply customized mitigation handlers
        // Note that we don't enable AEX-notify for customized mitigation handler
        sgx_apply_mitigations(info);
        restore_xregs(info->xsave_area);
        apply_constant_time_sgxstep_mitigation_and_continue_execution(info);
    }
    else
#endif
    {
        //instruction triggering the exception will be executed again.
        restore_xregs(info->xsave_area);
        continue_execution(info);
    }
failed_end:
    thread_data->exception_flag = -1; // mark the current exception cannot be handled
    abort();    // throw abortion
}

static int expand_stack_by_pages(void *start_addr, size_t page_count)
{
    int ret = -1;

    if ((start_addr == NULL) || (page_count == 0))
        return -1;

    ret = mm_commit(start_addr, page_count << SE_PAGE_SHIFT);
    return ret;
}

extern "C" const char Lereport_inst;
extern "C" const char Leverifyreport2_inst;

// trts_handle_exception(void *tcs)
//      the entry point for the exceptoin handling
// Parameter
//      the pointer of TCS
// Return Value
//      none zero - success
extern "C" sgx_status_t trts_handle_exception(void *tcs)
{
    thread_data_t *thread_data = get_thread_data();
    ssa_gpr_t *ssa_gpr = NULL;
    sgx_exception_info_t *info = NULL;
    uintptr_t sp_u, sp, *new_sp = NULL;
    size_t size = 0;
    bool is_exception_handled = false;

    if ((thread_data == NULL) || (tcs == NULL)) goto default_handler;
    if (check_static_stack_canary(tcs) != 0)
        goto default_handler;
 
    if(get_enclave_state() != ENCLAVE_INIT_DONE)
    {
        goto default_handler;
    }
    
    // check if the exception is raised from 2nd phrase
    if(thread_data->exception_flag == -1) {
        goto default_handler;
    }
 
    if ((TD2TCS(thread_data) != tcs) 
            || (((thread_data->first_ssa_gpr)&(~0xfff)) - ROUND_TO_PAGE(get_xsave_size() + sizeof(ssa_gpr_t))) != (uintptr_t)tcs) {
        goto default_handler;
    }

    // no need to check the result of ssa_gpr because thread_data is always trusted
    ssa_gpr = reinterpret_cast<ssa_gpr_t *>(thread_data->first_ssa_gpr);

    // The unstrusted RSP should never point inside the enclave
    sp_u = ssa_gpr->REG(sp_u);
    if (!sgx_is_outside_enclave((void *)sp_u, sizeof(sp_u)))
    {
        set_enclave_state(ENCLAVE_CRASHED);
        return SGX_ERROR_STACK_OVERRUN;
    }

    // The untrusted and trusted RSPs cannot be the same, unless
    // an exception happened before the enclave setup the trusted stack
    sp = ssa_gpr->REG(sp);
    if (sp_u == sp)
    {
        set_enclave_state(ENCLAVE_CRASHED);
        return SGX_ERROR_STACK_OVERRUN;
    }

    if(!is_stack_addr((void*)sp, 0))  // check stack overrun only, alignment will be checked after exception handled
    {
        set_enclave_state(ENCLAVE_CRASHED);
        return SGX_ERROR_STACK_OVERRUN;
    }

    size = 0;
    // x86_64 requires a 128-bytes red zone, which begins directly
    // after the return addr and includes func's arguments
    size += RED_ZONE_SIZE;

    // Add space for reserved slot for GPRs that will be used by mitigation
    // assembly code RIP, RAX, RBX, RCX, RDX, RBP, RSI, RDI, 1st
    // QWORD of red zone
    size += RSVD_SIZE_OF_MITIGATION_STACK_AREA;

    // decrease the stack to give space for info
    size += sizeof(sgx_exception_info_t);
    size += thread_data->xsave_size;
    sp -= size;
    sp = sp & ~0x3F;

    // check the decreased sp to make sure it is in the trusted stack range
    if(!is_stack_addr((void *)sp, size))
    {
        set_enclave_state(ENCLAVE_CRASHED);
        return SGX_ERROR_STACK_OVERRUN;
    }

    info = (sgx_exception_info_t *)sp;
    // decrease the stack to save the SSA[0]->ip
    size = sizeof(uintptr_t);
    sp -= size;
    if(!is_stack_addr((void *)sp, size))
    {
        set_enclave_state(ENCLAVE_CRASHED);
        return SGX_ERROR_STACK_OVERRUN;
    }

    /* try to allocate memory dynamically */
    if((size_t)sp < thread_data->stack_commit_addr)
    {
        int ret = -1;
        size_t page_aligned_delta = 0;
        /* try to allocate memory dynamically */
        page_aligned_delta = ROUND_TO(thread_data->stack_commit_addr - (size_t)sp, SE_PAGE_SIZE);
        if ((thread_data->stack_commit_addr > page_aligned_delta)
                && ((thread_data->stack_commit_addr - page_aligned_delta) >= thread_data->stack_limit_addr))
        {
            ret = expand_stack_by_pages((void *)(thread_data->stack_commit_addr - page_aligned_delta),
                                        (page_aligned_delta >> SE_PAGE_SHIFT));
        }
        if (ret == 0)
        {
            thread_data->stack_commit_addr -= page_aligned_delta;
            is_exception_handled = true; // The exception has been handled in the 1st phase exception handler
            goto handler_end;
        }
        else
        {
            set_enclave_state(ENCLAVE_CRASHED);
            return SGX_ERROR_STACK_OVERRUN;
        }
    }

    if (size_t(&Lereport_inst) == ssa_gpr->REG(ip) && SE_EREPORT == ssa_gpr->REG(ax))
    {
        // Handle the exception raised by EREPORT instruction
        ssa_gpr->REG(ip) += 3;     // Skip ENCLU, which is always a 3-byte instruction
        ssa_gpr->REG(flags) |= 1;  // Set CF to indicate error condition, see implementation of do_report()
        is_exception_handled = true; // The exception has been handled in the 1st phase exception handler.
        goto handler_end;
    }
    if (size_t(&Leverifyreport2_inst) == ssa_gpr->REG(ip) && SE_EVERIFYREPORT2 == ssa_gpr->REG(ax))
    {
        // Handle the exception raised by everifyreport2 instruction
        ssa_gpr->REG(ip) += 3;     // Skip ENCLU, which is always a 3-byte instruction
        ssa_gpr->REG(flags) |= 64;  // Set ZF to indicate error condition, see implementation of do_everifyreport2()
        ssa_gpr->REG(ax) = EVERIFYREPORT2_INVALID_LEAF;
        is_exception_handled = true; // The exception has been handled in the 1st phase exception handler.
        goto handler_end;
    }

    if(g_aexnotify_supported == 0 && ssa_gpr->exit_info.valid != 1)
    {
        // exception handlers are not allowed to call in a non-exception state
        // add aexnotify check here to skip the case of interrupts
        goto default_handler;
    }
handler_end:
    // initialize the info with SSA[0]
    info->exception_valid = is_exception_handled ? 0 : ssa_gpr->exit_info.valid;
    info->exception_vector = (sgx_exception_vector_t)ssa_gpr->exit_info.vector;
    info->exception_type = (sgx_exception_type_t)ssa_gpr->exit_info.exit_type;
    info->xsave_size = thread_data->xsave_size;

    info->cpu_context.REG(ax) = ssa_gpr->REG(ax);
    info->cpu_context.REG(cx) = ssa_gpr->REG(cx);
    info->cpu_context.REG(dx) = ssa_gpr->REG(dx);
    info->cpu_context.REG(bx) = ssa_gpr->REG(bx);
    info->cpu_context.REG(sp) = ssa_gpr->REG(sp);
    info->cpu_context.REG(bp) = ssa_gpr->REG(bp);
    info->cpu_context.REG(si) = ssa_gpr->REG(si);
    info->cpu_context.REG(di) = ssa_gpr->REG(di);
    info->cpu_context.REG(flags) = ssa_gpr->REG(flags);
    info->cpu_context.REG(ip) = ssa_gpr->REG(ip);
#ifdef SE_64
    info->cpu_context.r8  = ssa_gpr->r8;
    info->cpu_context.r9  = ssa_gpr->r9;
    info->cpu_context.r10 = ssa_gpr->r10;
    info->cpu_context.r11 = ssa_gpr->r11;
    info->cpu_context.r12 = ssa_gpr->r12;
    info->cpu_context.r13 = ssa_gpr->r13;
    info->cpu_context.r14 = ssa_gpr->r14;
    info->cpu_context.r15 = ssa_gpr->r15;
#endif
    if ((info->exception_vector == SGX_EXCEPTION_VECTOR_PF)
            || (info->exception_vector == SGX_EXCEPTION_VECTOR_GP))
    {
        misc_exinfo_t* exinfo =
            (misc_exinfo_t*)((uint64_t)ssa_gpr - (uint64_t)MISC_BYTE_SIZE);
        info->exinfo.faulting_address = exinfo->maddr;
        info->exinfo.error_code = exinfo->errcd;
    }
    new_sp = (uintptr_t *)sp;
    if(!(g_aexnotify_supported || is_exception_handled == true))
    {
        // Two cases that we don't need to run below code:
        //  1. AEXNotify is enabled
        //  2. stack expansion or EREPORT exception. We have handled it 
        //  in the first phase and we should not change anything in the ssa_gpr
        //
        ssa_gpr->REG(ip) = (size_t)internal_handle_exception; // prepare the ip for 2nd phrase handling
        ssa_gpr->REG(sp) = (size_t)new_sp;      // new stack for internal_handle_exception
        ssa_gpr->REG(ax) = (size_t)info;        // 1st parameter (info) for LINUX32
        ssa_gpr->REG(di) = (size_t)info;        // 1st parameter (info) for LINUX64, LINUX32 also uses it while restoring the context
    }
    *new_sp = info->cpu_context.REG(ip);    // for debugger to get call trace
#ifndef SE_SIM
    if(g_aexnotify_supported)
    {
        info->do_aex_mitigation = get_ssa_aexnotify();
        void *first_ssa_xsave = reinterpret_cast<void *>(thread_data->first_ssa_xsave);
        restore_xregs((uint8_t*)first_ssa_xsave);
        // With AEX Notify, we don't need to do a return here (phase-1 handler). 
        // Instead, we jump to internal_handle_exception (phase-2 handler).
        // We should not make a function call either, because ideally the return at 
        // the end of phase-2 handler should directly return to the interrupted enclave code.
        // Disable aexnotify before EDCSSA
        if(info->do_aex_mitigation == 1)
        {
            sgx_set_ssa_aexnotify(0);
        }
        second_phase(info, new_sp, (void *)internal_handle_exception);
    }
    else
#endif
    {
        return SGX_SUCCESS;
    }
 
default_handler:
    set_enclave_state(ENCLAVE_CRASHED);
    return SGX_ERROR_ENCLAVE_CRASHED;
}
