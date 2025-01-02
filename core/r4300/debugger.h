#pragma once
#include <cstdint>

namespace Debugger
{
    typedef struct CpuState {
        uint32_t opcode;
        uint32_t address;
    } t_cpu_state;

    /**
     * \brief Gets whether execution is resumed
     */
    bool get_resumed();

    /**
     * \brief Sets execution resumed status
     */
    void set_is_resumed(bool);

    /**
     * Steps execution by one instruction
     */
    void step();

    /**
     * \brief Gets whether DMA reads are allowed. If false, reads should return 0xFF
     */
    bool get_dma_read_enabled();

    /**
     * \brief Sets whether DMA reads are allowed
     */
    void set_dma_read_enabled(bool);

    /**
     * \brief Gets whether the RSP is enabled.
     */
    bool get_rsp_enabled();

    /**
     * \brief Sets whether the RSP is enabled.
     */
    void set_rsp_enabled(bool);

    /**
     * \brief Notifies the debugger of a processor cycle ending
     * \param opcode The processor's opcode
     * \param address The processor's address
     */
    void on_late_cycle(uint32_t opcode, uint32_t address);
} // namespace Debugger
