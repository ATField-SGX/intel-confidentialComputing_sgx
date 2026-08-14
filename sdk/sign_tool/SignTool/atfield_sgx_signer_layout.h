/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef ATFIELD_SGX_SIGNER_LAYOUT_H
#define ATFIELD_SGX_SIGNER_LAYOUT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATFIELD_SGX_SIGNER_LAYOUT_MAX_RECORDS 1024u

/* Byte-for-byte representation of one SGX DIR_LAYOUT record. */
typedef struct atfield_sgx_signer_layout_record {
    uint16_t id;
    uint16_t second;
    uint32_t u32;
    uint64_t u64;
    uint32_t words[4];
} atfield_sgx_signer_layout_record;

typedef struct atfield_sgx_signer_layout_result {
    uint64_t metadata_version;
    uint64_t enclave_size;
    uint64_t ordinary_image_end_rva;
    uint32_t layout_count;
    atfield_sgx_signer_layout_record records[ATFIELD_SGX_SIGNER_LAYOUT_MAX_RECORDS];
} atfield_sgx_signer_layout_result;

/* Build metadata/layout only; no loading, hashing, signing, or file update. */
int atfield_sgx_build_signer_layout(const uint8_t *elf, size_t elf_size,
                                    const char *config_path,
                                    atfield_sgx_signer_layout_result *out);

#ifdef __cplusplus
}
#endif

#endif
