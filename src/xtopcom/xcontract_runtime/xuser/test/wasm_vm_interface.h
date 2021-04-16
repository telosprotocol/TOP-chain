#ifndef WASM_VM_INTERFACE
#define WASM_VM_INTERFACE

#ifdef __cplusplus
extern "C" {
#endif

bool validate_wasm_with_content(uint8_t *s, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* WASM_VM_INTERFACE */